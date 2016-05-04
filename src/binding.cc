/*
 * binding.cc
 *
 *  Created on: Mar 15, 2015
 *      Author: ed
 * (c) 2015, WigWag Inc.
 */

#define BUILDING_NODE_EXTENSION

#include <v8.h>
#include <node.h>
#include <uv.h>
#include <node_buffer.h>


#include "TW/tw_utils.h"
#include "grease_client.h"
#include "nan.h"

#ifdef NODE_DHCLIENT_DEBUG_BUILD
#warning "*** Debug build."
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "dhclient-cfuncs.h"
#include "error-common.h"

#include <TW/tw_fifo.h>
#include <TW/tw_circular.h>
#include <TW/tw_khash.h>
#include <assert.h>
typedef Allocator<Alloc_Std> DhclientAllocator;

using namespace v8;



#define V8_IFEXIST_TO_DYN_CSTR(v8field,cfield,val,v8obj) { val = Nan::Get(v8obj, Nan::New(v8field).ToLocalChecked()).ToLocalChecked();\
if(!val->IsUndefined() && val->IsString()) {\
	if(cfield) free(cfield);\
	v8::String::Utf8Value v8str(val);\
	cfield = strdup(v8str.operator *()); } }
#define V8_IFEXIST_SPRINT_CSTR(val, v8obj, v8field, fmt, cstr) { val = Nan::Get(v8obj, Nan::New(v8field).ToLocalChecked()).ToLocalChecked();\
if(!val->IsUndefined() && val->IsString()) {\
	v8::String::Utf8Value v8str(val);\
	cstr += sprintf(cstr, fmt, v8str.operator *());\
	} }
#define V8_IFEXIST_TO_INT_CAST(v8field,cfield,val,v8obj,typ) { val = Nan::Get(v8obj, Nan::New(v8field).ToLocalChecked()).ToLocalChecked();\
		if(!val->IsUndefined() && val->IsNumber()) cfield = (typ) val->ToInteger()->IntegerValue(); }

#define V8_IFEXIST_TO_INT32(v8field,cfield,val,v8obj) { val = Nan::Get(v8obj, Nan::New(v8field).ToLocalChecked()).ToLocalChecked();\
		if(!val->IsUndefined() && val->IsNumber()) cfield = val->ToInteger()->Int32Value(); }
#define INT32_TO_V8(v8field,cfield,v8obj) Nan::Set(v8obj, Nan::New(v8field).ToLocalChecked(),Nan::New<Int32>(cfield))
#define CSTR_TO_V8STR(v8field,cstr,v8obj) { if(cstr) Nan::Set(v8obj,Nan::New(v8field).ToLocalChecked(),Nan::New(cstr,strlen(cstr)).ToLocalChecked()); }

#define MAX_COMMAND_QUEUE 100


class NodeDhclient : public Nan::ObjectWrap {
public:
	enum work_code {
		DISCOVER_REQUEST,
		RELEASE,
		HIBERNATE,
		AWAKEN,
		SHUTDOWN,
		NO_WORK
	};
	enum v8_event_code {
		UNDEFINED,
		NEWADDRESS,     // a new address was obtained
		EXPIRED,        // the lease has expired
		RENEWAL_NOTIFY, // half the expire time is up, its time to renew

		// a lease was acquired, but not necessarily a new address
		// (correspond to what would go in /var/lib/dhcp/dhclient.leases)
		RECORD_NEW_LEASE,
		HIBERNATE_COMPLETE,
		AWAKEN_COMPLETE,
		RELEASE_COMPLETE,
		SHUTDOWN_COMPLETE,
		BAD_CONFIG,
		GENERAL_ERROR
	};

	static void do_workReq(uv_work_t *req);
	static void post_workReq(uv_work_t *req, int status);

	class workReq {
	protected:
//		uv_async_t async;  // used to tell node.js event loop we are done when work is done
//		uv_work_t work;
//		bool ref;
		bool freeBacking;  // free the backing on delete?

	public:
		work_code cmdcode;
		v8_event_code v8code;
		_errcmn::err_ev err;
		bool originV8; // are callbacks init-ed?
		Nan::Callback* onCompleteCB;
		Nan::Callback* onFulfillCB;
		Nan::Persistent<Object> buffer; // Buffer object passed in
		uint32_t uint32t_data;
		char *_backing;    // backing of the passed in Buffer
		int len; // amount read or written
		NodeDhclient *self;
		int _reqSize;
		int retries;
		int timeout;


		workReq(NodeDhclient *i, work_code c) : freeBacking(false),
				cmdcode(c), v8code(UNDEFINED),
				err(),
				originV8(true),
				onCompleteCB(NULL), onFulfillCB(NULL), buffer(),
				_backing(NULL),  len(0), self(i), _reqSize(0),
				retries(DEFAULT_RETRIES), timeout(TIMEOUT_FOR_RETRY) {
			switch(c) {
			// special setup for certain requests?
			}
		}
		workReq() = delete;
		// NOTE: workReq should be deleted from the v8 thread!!
		~workReq() {
			if(freeBacking) ::free(_backing);
		}
	protected:
		// used to create work object that originates from dhcp thread (not v8)
		workReq(NodeDhclient *i, v8_event_code c) : freeBacking(false),
				cmdcode(NO_WORK), v8code(c),
				err(),
				originV8(false),
				_backing(NULL),  len(0), self(i), _reqSize(0),
				retries(DEFAULT_RETRIES), timeout(TIMEOUT_FOR_RETRY) {
			};
	public:
		static workReq *createDhcpOriginatedWork(NodeDhclient *i, v8_event_code c) {
			return new workReq(i,c);
		};

	};
protected:
	dhclient_config _config;
	TWlib::tw_safeCircular<workReq *, DhclientAllocator > dhcp_thread_queue;
	TWlib::tw_safeCircular<workReq *, DhclientAllocator > v8_cmd_queue;
	uv_mutex_t _control;
	bool _shutdown;
	uv_cond_t _start_cond;
	uv_thread_t _dhcp_thread;
	static void dhcp_thread(void *d);
	static void dhcp_worker_thread(void *d);
	bool threadUp;
	Nan::Callback* leaseCallback;

	uv_async_t _toV8_async;       // used when me need to make a callback / take action in v8 thread
	static void toV8_control(uv_async_t *handle, int status /*UNUSED*/);


	bool setupThread() {
		uv_mutex_lock(&_control);
		bool ret = threadUp;
		uv_mutex_unlock(&_control);
		if(!ret) {
			//Yash: Fix for relay-kernel 4.2 - First, lock the mutex and then start the thread
			//otherwise the signal will be lost and the main thread will hang indefinitely
			uv_mutex_lock(&_control);
#if UV_VERSION_MAJOR > 0
			uv_thread_create(&this->_dhcp_thread,dhcp_thread,this); // start thread
			// FIXME in newer versions libuv returns something different
#else
			int r = uv_thread_create(&this->_dhcp_thread,dhcp_thread,this); // start thread
			if (r < 0) {  // old libuv returns -1 on failure
				ERROR_OUT("NON-RECOVERABLE: failed to create dhcp thread.\n");
				return false;
			}
#endif
			uv_cond_wait(&_start_cond, &_control); // wait for thread's start...
			ret = threadUp;
			uv_ref((uv_handle_t *) &_toV8_async);
			uv_mutex_unlock(&_control);
		}
		return ret;
	}

	void sigThreadUp() {
		uv_mutex_lock(&_control);
		threadUp = true;
		uv_cond_signal(&_start_cond);
		uv_mutex_unlock(&_control);
	}

	void sigThreadDown() {
		uv_mutex_lock(&_control);
		threadUp = false;
		uv_thread_join(&this->_dhcp_thread);
		uv_cond_signal(&_start_cond);
		uv_mutex_unlock(&_control);
		uv_unref((uv_handle_t *) &_toV8_async);
	}

	bool isThreadUp() {
		bool ret = false;
		uv_mutex_lock(&_control);
		ret = threadUp;
		uv_mutex_unlock(&_control);
		return ret;
	}


public:



    static Nan::Persistent<Function> constructor;
    static Nan::Persistent<ObjectTemplate> prototype;

	static NAN_METHOD(SetConfig);
	static NAN_METHOD(SetCurrentLease);
	static NAN_METHOD(GetConfig);
	static NAN_METHOD(Start);
	static NAN_METHOD(Shutdown);

	static NAN_METHOD(RequestLease);
	static NAN_METHOD(Hibernate);
	static NAN_METHOD(Awaken);
	static NAN_METHOD(Release);
	static NAN_METHOD(SetLeaseCallback);
	static NAN_METHOD(NewClient);

	// NAN_METHOD(NewInstance) {
	// 	HandleScope scope;
	// 	int n = args.Length();
	// 	Local<Object> instance;
	//
	// 	if(args.Length() > 0) {
	// 		Handle<Value> argv[n];
	// 		for(int x=0;x<n;x++)
	// 			argv[x] = args[x];
	// 		instance = NodeDhclient::constructor->NewInstance(n, argv);
	// 	} else {
	// 		instance = NodeDhclient::constructor->NewInstance();
	// 	}
	//
	// 	return scope.Close(instance);
	// }
	//

	static NAN_METHOD(New) {

		NodeDhclient* obj = NULL;

		if (info.IsConstructCall()) {
		    // Invoked as constructor: `new MyObject(...)`
	//	    double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
	//		if(args.Length() > 0) {
	//			if(!args[0]->IsString()) {
	//				return ThrowException(Exception::TypeError(String::New("Improper first arg to ProcFs cstor. Must be a string.")));
	//			}
	//			Local<Value> ifname = args[0]->ToObject()->Get(String::New(""));

	//			v8::String::Utf8Value v8str(args[0]);
				//obj->setIfName(v8str.operator *(),v8str.length());
				obj = new NodeDhclient();
	//		} else {
	//			return ThrowException(Exception::TypeError(String::New("First arg must be a string path.")));
	//		}
			obj->Wrap(info.This());


			info.GetReturnValue().Set(info.This());
		} else {
		    // Invoked as plain function `MyObject(...)`, turn into construct call.
		    const int argc = 1;
		    Local<Value> argv[argc] = { info[0] };
			v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
	    	info.GetReturnValue().Set(cons->NewInstance(argc,argv));
		}
	}




	static void Init()
	{
		// Prepare constructor template
		Local<FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
		tpl->SetClassName(Nan::New("IscDhclient").ToLocalChecked());
		tpl->InstanceTemplate()->SetInternalFieldCount(1);
		tpl->PrototypeTemplate()->SetInternalFieldCount(2);

		// Prototype
		Nan::SetPrototypeMethod(tpl,"hibernate",Hibernate);
		Nan::SetPrototypeMethod(tpl,"awaken",Awaken);
		Nan::SetPrototypeMethod(tpl,"release",Release);
		Nan::SetPrototypeMethod(tpl,"requestLease",RequestLease);
		Nan::SetPrototypeMethod(tpl,"_setLeaseCallback",SetLeaseCallback);

		Nan::SetPrototypeMethod(tpl,"setCurrentLease",SetCurrentLease);
		Nan::SetPrototypeMethod(tpl,"setConfig",SetConfig);
		Nan::SetPrototypeMethod(tpl,"getConfig",GetConfig);
		Nan::SetPrototypeMethod(tpl,"start",Start);
		Nan::SetPrototypeMethod(tpl,"shutdown",Shutdown);


		constructor.Reset(tpl->GetFunction());
	}


	NodeDhclient() : _config(),
			dhcp_thread_queue(MAX_COMMAND_QUEUE, false), v8_cmd_queue(MAX_COMMAND_QUEUE, false),
			_control(), _shutdown(false), _start_cond(), _dhcp_thread(), threadUp(false),
			leaseCallback(NULL),
			_toV8_async()
	{
		init_defaults_config(&_config);

		uv_cond_init(&_start_cond);
		uv_mutex_init(&_control);
		uv_async_init(uv_default_loop(), &_toV8_async, reinterpret_cast<uv_async_cb>(toV8_control));
		// unref it - we don't want this to hold up an exit of node
		// when the thread is running, it ->Ref() NodeDhclient, and this *should* prevent
		// node from exiting until the thread shutsdown anyway
		// we also re-ref this handle as well then - b/c ->Ref() alone does not seem to work (?)
		uv_unref((uv_handle_t *) &_toV8_async);
	}

	~NodeDhclient()	{ if(_config.initial_leases) free(_config.initial_leases); }


	// external friend functions - interconnect to regular dhclient C code
	friend int submit_lease_to_v8(char *json);
	friend int submit_hibernate_complete_to_v8(void);
	friend int submit_awaken_complete_to_v8(void);
	friend int submit_release_complete_to_v8(void);

protected:

	static bool submitToV8ControlCommand(workReq *ev);

	bool submitToV8(workReq *ev) {
		bool ret = v8_cmd_queue.addMvIfRoom(ev);
		_toV8_async.data = this;
		uv_async_send(&_toV8_async); // let v8 know - calls callback...
		return ret;
	}
};



// TLS var for use with static functions called from
// standard ISC DHCP code
__thread NodeDhclient *nodeClientTLS; // init is handled by dhcp_thread()
__thread NodeDhclient *nodeClientWorkerTLS; // init is handled by dhcp_worker_thread()

bool NodeDhclient::submitToV8ControlCommand(NodeDhclient::workReq *ev) {
	assert(nodeClientTLS);
	return nodeClientTLS->submitToV8(ev);
}


// static - thread runner
void NodeDhclient::dhcp_thread(void *d) {
	NodeDhclient::workReq *work = NULL;
	NodeDhclient *self = (NodeDhclient *) d;
	nodeClientTLS = (NodeDhclient *) d;  // assign object value to TLS var also

	self->sigThreadUp();
	bool shutdown = false;

	NodeDhclient::workReq *shutdown_cmd = NULL;
	NodeDhclient::workReq *control_cmd = NULL;
	char *errstr = NULL;
	int ret;
	uv_thread_t _dhcp_worker_thread;

	while(!shutdown) {
		if(self->dhcp_thread_queue.removeOrBlock(work)) {
			switch(work->cmdcode) {
			case SHUTDOWN:
				DBG_OUT("have work: %p code: SHUTDOWN\n",work, work->cmdcode);
				shutdown_cmd = work;
				shutdown_cmd->v8code = SHUTDOWN_COMPLETE;
				ret = do_dhclient_shutdown(&errstr, &self->_config);
				if(!ret) shutdown = true;
				break;
			case DISCOVER_REQUEST:
				DBG_OUT("have work: %p code: DISCOVER_REQUEST\n",work, work->cmdcode);
				{
					char *errstr = NULL;
					//int ret = do_dhclient_request(&errstr, &self->_config);

					ret = uv_thread_create(&_dhcp_worker_thread,dhcp_worker_thread,d); // start thread
					if(ret != 0) {
						DBG_OUT("Got error: do_dhclient_request() = %d\n",ret);

						if(ret == DHCLIENT_INVALID_CONFIG)
							work->v8code = BAD_CONFIG;
						else
							work->v8code = GENERAL_ERROR;
						work->err.setError(ret, errstr);
						if(errstr) ::free(errstr);
					}
				}
				self->submitToV8(work);
				break;

			case HIBERNATE:
				DBG_OUT("have work: %p code: HIBERNATE\n",work, work->cmdcode);
				control_cmd = work;
				control_cmd->v8code = HIBERNATE_COMPLETE;
				ret = do_dhclient_hibernate(&errstr, &self->_config);
				if(!ret) self->submitToV8(control_cmd);
				break;

			case AWAKEN:
				DBG_OUT("have work: %p code: AWAKEN\n",work, work->cmdcode);
				control_cmd = work;
				control_cmd->v8code = AWAKEN_COMPLETE;
				ret = do_dhclient_awaken(&errstr, &self->_config);
				if(!ret) self->submitToV8(control_cmd);
				break;

			case RELEASE:
				DBG_OUT("have work: %p code: RELEASE\n",work, work->cmdcode);
				control_cmd = work;
				control_cmd->v8code = RELEASE_COMPLETE;
				ret = do_dhclient_release(&errstr, &self->_config);
				if(!ret) self->submitToV8(control_cmd);
				break;

			default:
				DBG_OUT("Don't know how to do work: %d\n", work->cmdcode);
			}
		} else {
			DBG_OUT("dhcp thread wakeup. failed removal.\n");
			// wokeup for shutdown?
			bool shutdown = false;
			uv_mutex_lock(&self->_control);
			shutdown = self->_shutdown;
			uv_mutex_unlock(&self->_control);
			if(shutdown)
				break;
		}

	}
	DBG_OUT("dhcp thread shutting down.\n");
	self->sigThreadDown();
	if(shutdown_cmd)
		self->submitToV8(shutdown_cmd);
}

// static - thread runner
void NodeDhclient::dhcp_worker_thread(void *d) {

	NodeDhclient::workReq *shutdown_cmd = NULL;
	NodeDhclient *self = (NodeDhclient *) d;
	nodeClientWorkerTLS = (NodeDhclient *) d;  // assign object value to TLS var also

	char *errstr = NULL;

	int ret = do_dhclient_request(&errstr, &self->_config);
	DBG_OUT("do_dhclient_request returned with rc = %d", ret);
	if(ret != 0) {
		DBG_OUT("Got error: do_dhclient_request() = %d\n",ret);

		// if(ret == DHCLIENT_INVALID_CONFIG)
		// 	work->v8code = BAD_CONFIG;
		// else
		// 	work->v8code = GENERAL_ERROR;
		// work->err.setError(ret, errstr);
		if(errstr) ::free(errstr);
	}
}


// execute in the V8 thread
// allows callbacks, etc. to happen
void NodeDhclient::toV8_control(uv_async_t *handle, int status /*UNUSED*/) {
 //   SixLBR::control_event *ev = (SixLBR::control_event *) handle->data;

	NodeDhclient::workReq *work;

	NodeDhclient *dhclient = (NodeDhclient *) handle->data;

	auto isolate = Isolate::GetCurrent();
	HandleScope scope(isolate);
	auto context = isolate->GetCurrentContext();
	auto global = context->Global();

	const unsigned argc = 2;
	Local<Value> argv[argc];

	while(dhclient->v8_cmd_queue.remove(work)) {
		switch(work->v8code) {
		case RECORD_NEW_LEASE:
			DBG_OUT("completing work code = RECORD_NEW_LEASE\n");
			{
				// call leaseLog callback
				if(!dhclient->leaseCallback->IsEmpty()) {
					argv[0] = Nan::New<v8::String>(work->_backing,work->len).ToLocalChecked();
					dhclient->leaseCallback->Call(Nan::GetCurrentContext()->Global(),1,argv);
				} else {
					ERROR_OUT("No leaseCallback set! dump: %s\n", work->_backing);
				}
				delete work;
			}
			break;
		case BAD_CONFIG:
		case GENERAL_ERROR:
			DBG_OUT("completing work code = GENERAL_ERROR\n");
			if(work->err.hasErr()) {
				argv[0] = _errcmn::err_ev_to_JS(work->err)->ToObject();
				if(!work->onCompleteCB->IsEmpty())
					work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),1,argv);
			} else {
				ERROR_OUT("toV8_control saw error - but no error info.");
			}
		case HIBERNATE_COMPLETE:
		case AWAKEN_COMPLETE:
		case RELEASE_COMPLETE:
			DBG_OUT("complete\n");
			if(work->err.hasErr()) {
				if(!work->onCompleteCB->IsEmpty())
					work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),1,NULL);
			} else {
				if(!work->onCompleteCB->IsEmpty())
					work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),0,NULL);
			}
			break;
		case SHUTDOWN_COMPLETE:
			DBG_OUT("completing work code = SHUTDOWN_COMPLETE\n");
			if(work->err.hasErr()) {
				argv[0] = _errcmn::err_ev_to_JS(work->err)->ToObject();
				if(!work->onCompleteCB->IsEmpty())
					work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),1,argv);
			} else {
				if(!work->onCompleteCB->IsEmpty())
					work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),0,NULL);
			}
			break;
		case NEWADDRESS:
			DBG_OUT("completing work code = NEWADDRESS\n");
			if(!work->onCompleteCB->IsEmpty()) {
				if(work->err.hasErr()) {
//					argv[1] =  Local<Value>::New(Null());
					argv[0] = _errcmn::err_ev_to_JS(work->err)->ToObject();
					if(!work->onCompleteCB->IsEmpty())
						work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),1,argv);
				} else {
					// TODO make object to pass in...
					if(!work->onCompleteCB->IsEmpty())
						work->onCompleteCB->Call(Nan::GetCurrentContext()->Global(),0,NULL);
				}
			}
			break;
		default:
			DBG_OUT("Unhandled v8code: 0x%x\n", work->v8code);
		}
	}
}


int submit_lease_to_v8(char *json) {
	if(json) {
		assert(nodeClientWorkerTLS);

		NodeDhclient::workReq *work = NodeDhclient::workReq::createDhcpOriginatedWork(nodeClientWorkerTLS,NodeDhclient::RECORD_NEW_LEASE);
		work->_backing = json;
		work->len = strlen(json)+1;
		if(nodeClientWorkerTLS->submitToV8(work)) {
			return 1;
		} else {
			ERROR_OUT("Failed to add event to queue: v8_cmd_queue\n");
			return 0;
		}
	}
	return 0;
}


int submit_hibernate_complete_to_v8(void) {
	assert(nodeClientTLS);

	NodeDhclient::workReq *work = NodeDhclient::workReq::createDhcpOriginatedWork(nodeClientTLS,NodeDhclient::HIBERNATE_COMPLETE);
	if(nodeClientTLS->submitToV8(work)) {
		return 1;
	} else {
		ERROR_OUT("Failed to add event to queue: v8_cmd_queue\n");
		return 0;
	}
}

int submit_awaken_complete_to_v8(void) {
	assert(nodeClientTLS);

	NodeDhclient::workReq *work = NodeDhclient::workReq::createDhcpOriginatedWork(nodeClientTLS,NodeDhclient::AWAKEN_COMPLETE);
	if(nodeClientTLS->submitToV8(work)) {
		return 1;
	} else {
		ERROR_OUT("Failed to add event to queue: v8_cmd_queue\n");
		return 0;
	}
}

int submit_release_complete_to_v8(void) {
	assert(nodeClientTLS);

	NodeDhclient::workReq *work = NodeDhclient::workReq::createDhcpOriginatedWork(nodeClientTLS,NodeDhclient::RELEASE_COMPLETE);
	if(nodeClientTLS->submitToV8(work)) {
		return 1;
	} else {
		ERROR_OUT("Failed to add event to queue: v8_cmd_queue\n");
		return 0;
	}
}

NAN_METHOD(NodeDhclient::NewClient) {

	NodeDhclient::New(info);
}

Nan::Persistent<Function> NodeDhclient::constructor;
Nan::Persistent<ObjectTemplate> NodeDhclient::prototype;


//typedef struct dhclient_config_t {
//	int local_family;  // = AF_INET  or  AF_INET6
//	int local_port;    // = 0
//	char *server;      // = NULL   the DHCP server address to use (instead of broadcast)
//	char *interfaces[DHCLIENT_MAX_INTERFACES];
//	// IPv6 stuff:
//	int wanted_ia_na;  // = -1   see dhclient option: -T and -P  and -N
//	int wanted_ia_ta;  // = 0    see dhclient option: -T
//	int wanted_ia_pd;  // = 0    see dhclient option: -P
//	int stateless;     // = 0    see dhclient option: -S
//	int duid_type;     // = 0    see dhclient option: -D
//	int quiet;         // = 0    if 1 be chatty on stdout, only shoudl be used for debugging
//
//	int exit_mode;     // = 0 these are for dhclient process related stuff - probably can remove them
//	int release_mode;  // = 0
//} dhclient_config;

NAN_METHOD(NodeDhclient::Shutdown) {

	NodeDhclient* self = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::SHUTDOWN);


	if(!self->isThreadUp()) {
		if(info.Length() > 0) {
			 if(info[0]->IsFunction()) {
					Nan::Callback(Local<Function>::Cast(info[0])).Call(Nan::GetCurrentContext()->Global(),0,NULL);
			 } else
				 return Nan::ThrowTypeError("bad param: shutdown([function]) -> Need [function]");
		}
	} else if(info.Length() > 0) {
		 if(info[0]->IsFunction())
				req->onCompleteCB = new Nan::Callback(info[0].As<Function>());
		 else
			 return Nan::ThrowTypeError("bad param: shutdown([function]) -> Need [function]");
	}
	self->dhcp_thread_queue.add(req);
}

/**
 * Blocks until thread is really up.
 */
NAN_METHOD(NodeDhclient::Start) {

	NodeDhclient* obj = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	info.GetReturnValue().Set(Nan::New<v8::Boolean>(obj->setupThread()));
}

NAN_METHOD(NodeDhclient::SetLeaseCallback) {

	NodeDhclient* obj = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	if(info.Length() > 0 && info[0]->IsFunction()) {
		obj->leaseCallback = new Nan::Callback(info[0].As<Function>());
	} else {
		 return Nan::ThrowTypeError("bad param: setLeaseCallback([function])");
	}
}


NAN_METHOD(NodeDhclient::SetConfig) {

	NodeDhclient* obj = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	if(info.Length() > 0 && info[0]->IsObject()) {
		Local<Object> o = info[0]->ToObject();
		Local<Value> v;
		V8_IFEXIST_TO_INT_CAST("local_family",obj->_config.local_family,v,o,int);
		V8_IFEXIST_TO_INT_CAST("local_port",obj->_config.local_port,v,o,int);
		V8_IFEXIST_TO_DYN_CSTR("server",obj->_config.server,v,o);
		{ v = Nan::Get(o, Nan::New("interfaces").ToLocalChecked()).ToLocalChecked();
			if(!v->IsUndefined() && v->IsArray()) {
				Handle<Array> a =  v8::Handle<v8::Array>::Cast(v);;
				int n = a->Length();
				if(n > DHCLIENT_MAX_INTERFACES) {
					return Nan::ThrowTypeError("Too many interfaces listed.");
				}
				for(int x=0;x<DHCLIENT_MAX_INTERFACES;x++) { // clear out old interface list
					if(obj->_config.interfaces[x]) {
						free(obj->_config.interfaces[x]);
						obj->_config.interfaces[x] = NULL;
					}
				}
				for(int x=0;x<n;x++) {
					{ 	Local<Value> s;
						s = a->Get(x);
						if(!s->IsUndefined() && s->IsString()) {
							v8::String::Utf8Value v8str(s);
							obj->_config.interfaces[x] = strdup(v8str.operator *());
						}
					}
				}
			} else {
				DBG_OUT("Warning - no interfaces configured.");
			}
		}

		V8_IFEXIST_TO_INT_CAST("wanted_ia_na",obj->_config.wanted_ia_na,v,o,int);
		V8_IFEXIST_TO_INT_CAST("wanted_ia_ta",obj->_config.wanted_ia_ta,v,o,int);
		V8_IFEXIST_TO_INT_CAST("wanted_ia_pd",obj->_config.wanted_ia_pd,v,o,int);
		V8_IFEXIST_TO_INT_CAST("stateless",obj->_config.stateless,v,o,int);
		V8_IFEXIST_TO_INT_CAST("duid_type",obj->_config.duid_type,v,o,int);
		V8_IFEXIST_TO_INT_CAST("quiet",obj->_config.quiet,v,o,int);
		V8_IFEXIST_TO_INT_CAST("exit_mode",obj->_config.exit_mode,v,o,int);
		V8_IFEXIST_TO_INT_CAST("release_mode",obj->_config.release_mode,v,o,int);

		V8_IFEXIST_TO_DYN_CSTR("config_options",obj->_config.config_options,v,o);
		if(obj->_config.config_options) obj->_config.config_options_len = strlen(obj->_config.config_options);

	} else {
		return Nan::ThrowTypeError("bad param: setConfig([object])");
	}
}

NAN_METHOD(NodeDhclient::SetCurrentLease) {

	NodeDhclient* obj = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	if(info.Length() > 0 && info[0]->IsObject()) {
		char lease[MAX_LEASE_STR_SIZE];
		memset(&lease,'\0', sizeof(lease));
		Local<Object> o = info[0]->ToObject();
		Local<Value> v;
		char *ls = lease;

		ls += sprintf(ls, "lease {\n");
		V8_IFEXIST_SPRINT_CSTR(v, o, "interface", "  interface \"%s\";\n", ls)
		V8_IFEXIST_SPRINT_CSTR(v, o, "fixed_address", "  fixed-address %s;\n", ls)


		Local<Value> opts_val = Nan::Get(o, Nan::New("options").ToLocalChecked()).ToLocalChecked();
		if(!opts_val->IsUndefined() && opts_val->IsObject()) {
			Local<Object> opts = opts_val->ToObject();
			V8_IFEXIST_SPRINT_CSTR(v, opts, "subnet-mask", "  option subnet-mask %s;\n", ls)
			V8_IFEXIST_SPRINT_CSTR(v, opts, "dhcp-lease-time", "  option dhcp-lease-time %s;\n", ls)
			V8_IFEXIST_SPRINT_CSTR(v, opts, "routers", "  option routers %s;\n", ls)
			V8_IFEXIST_SPRINT_CSTR(v, opts, "dhcp-message-type", "  option dhcp-message-type %s;\n", ls)
			V8_IFEXIST_SPRINT_CSTR(v, opts, "domain-name-servers", "  option domain-name-servers %s;\n", ls)
			V8_IFEXIST_SPRINT_CSTR(v, opts, "dhcp-server-identifier", "  option dhcp-server-identifier %s;\n", ls)
		}

		V8_IFEXIST_SPRINT_CSTR(v, o, "renew", "  renew %s\n", ls)
		V8_IFEXIST_SPRINT_CSTR(v, o, "rebind", "  rebind %s\n", ls)
		V8_IFEXIST_SPRINT_CSTR(v, o, "expire", "  expire %s\n", ls)
		sprintf(ls, "}\n");

		if(obj->_config.initial_leases) free(obj->_config.initial_leases);
		obj->_config.initial_leases = strdup(lease); // get's freed by
	} else {
		return Nan::ThrowTypeError("bad param: SetCurrentLease([object])");
	}
}




NAN_METHOD(NodeDhclient::GetConfig) {

	NodeDhclient* obj = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	Local<Object> o = Nan::New<Object>();

	INT32_TO_V8("local_family",obj->_config.local_family,o);
	INT32_TO_V8("local_port",obj->_config.local_port,o);
	CSTR_TO_V8STR("server",obj->_config.server,o);
	{
		int n = 0;
		while(n < DHCLIENT_MAX_INTERFACES) {
			if(!obj->_config.interfaces[n])
				break;
			n++;
		}
		if(n > 0) {
			Local<Array> ar = Nan::New<v8::Array>(n);
			for(int x=0;x<n;x++) {
				Nan::Set(ar, x, Nan::New(obj->_config.interfaces[x],strlen(obj->_config.interfaces[x])).ToLocalChecked());			}
			Nan::Set(o, Nan::New("interfaces").ToLocalChecked(),ar);
		} else {
			Nan::Set(o, Nan::New("interfaces").ToLocalChecked(),Nan::Undefined());
		}
	}

	INT32_TO_V8("wanted_ia_na",obj->_config.wanted_ia_na,o);
	INT32_TO_V8("wanted_ia_ta",obj->_config.wanted_ia_ta,o);
	INT32_TO_V8("wanted_ia_pd",obj->_config.wanted_ia_pd,o);
	INT32_TO_V8("stateless",obj->_config.stateless,o);
	INT32_TO_V8("duid_type",obj->_config.duid_type,o);
	INT32_TO_V8("quiet",obj->_config.quiet,o);
	INT32_TO_V8("exit_mode",obj->_config.exit_mode,o);
	INT32_TO_V8("release_mode",obj->_config.release_mode,o);

	CSTR_TO_V8STR("config_options",obj->_config.config_options,o);

	info.GetReturnValue().Set(o);
}



NAN_METHOD(NodeDhclient::RequestLease) {

	NodeDhclient* self = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::DISCOVER_REQUEST);
	DBG_OUT("created work: %p\n",req);


	if(!self->isThreadUp()) {
		const unsigned argc = 2;
		Local<Value> argv[argc];
		_errcmn::err_ev err;
		err.setError(DHCLIENT_EXEC_ERROR,"DHCP Thread not started.");
		argv[0] =  Nan::Null();
		argv[1] = _errcmn::err_ev_to_JS(err)->ToObject();
		if(info[0]->IsFunction())
			Nan::Callback(Local<Function>::Cast(info[0])).Call(Nan::GetCurrentContext()->Global(),2,argv);
		else
			ERROR_OUT("DHCP Thread not started. No callback provided.");
	} else {
		if(info.Length() > 0) {
			 if(info[0]->IsFunction())
					req->onCompleteCB = new Nan::Callback(info[0].As<Function>());
			 else
				 return Nan::ThrowTypeError("bad param: requestLease([function]) -> Need [function]");
		}
		self->dhcp_thread_queue.add(req);
	}
}

NAN_METHOD(NodeDhclient::Hibernate) {

	NodeDhclient* self = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::HIBERNATE);
	DBG_OUT("created work: %p\n",req);


	if(!self->isThreadUp()) {
		const unsigned argc = 2;
		Local<Value> argv[argc];
		_errcmn::err_ev err;
		err.setError(DHCLIENT_EXEC_ERROR,"DHCP Thread not started.");
		argv[0] =  Nan::Null();
		argv[1] = _errcmn::err_ev_to_JS(err)->ToObject();
		if(info[0]->IsFunction())
			Nan::Callback(Local<Function>::Cast(info[0])).Call(Nan::GetCurrentContext()->Global(),2,argv);
		else
			ERROR_OUT("DHCP Thread not started. No callback provided.");
	} else {
		if(info.Length() > 0) {
			 if(info[0]->IsFunction())
					req->onCompleteCB = new Nan::Callback(info[0].As<Function>());
			 else
				 return Nan::ThrowTypeError("bad param: Hibernate([function]) -> Need [function]");
		}
		self->dhcp_thread_queue.add(req);
	}
}

NAN_METHOD(NodeDhclient::Awaken) {

	NodeDhclient* self = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::AWAKEN);
	DBG_OUT("created work: %p\n",req);


	if(!self->isThreadUp()) {
		const unsigned argc = 2;
		Local<Value> argv[argc];
		_errcmn::err_ev err;
		err.setError(DHCLIENT_EXEC_ERROR,"DHCP Thread not started.");
		argv[0] =  Nan::Undefined();
		argv[1] = _errcmn::err_ev_to_JS(err)->ToObject();
		if(info[0]->IsFunction())
			Nan::Callback(Local<Function>::Cast(info[0])).Call(Nan::GetCurrentContext()->Global(),2,argv);
		else
			ERROR_OUT("DHCP Thread not started. No callback provided.");
	} else {
		if(info.Length() > 0) {
			 if(info[0]->IsFunction())
			 	req->onCompleteCB = new Nan::Callback(info[0].As<Function>());
			 else
				 return Nan::ThrowTypeError("bad param: awaken([function]) -> Need [function]");
		}
		self->dhcp_thread_queue.add(req);
	}
}

NAN_METHOD(NodeDhclient::Release) {
	NodeDhclient* self = Nan::ObjectWrap::Unwrap<NodeDhclient>(info.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::RELEASE);
	DBG_OUT("created work: %p\n",req);

	if(!self->isThreadUp()) {
		const unsigned argc = 2;
		Local<Value> argv[argc];
		_errcmn::err_ev err;
		err.setError(DHCLIENT_EXEC_ERROR,"DHCP Thread not started.");
		argv[0] =  Nan::Null();
		argv[1] = _errcmn::err_ev_to_JS(err)->ToObject();
		if(info[0]->IsFunction())
			Nan::Callback(Local<Function>::Cast(info[0])).Call(Nan::GetCurrentContext()->Global(),2,argv);
		else
			ERROR_OUT("DHCP Thread not started. No callback provided.");
	} else {
		if(info.Length() > 0) {
			 if(info[0]->IsFunction())
			 	req->onCompleteCB = new Nan::Callback(info[0].As<Function>());
			 else
				 return Nan::ThrowTypeError("bad param: release([function]) -> Need [function]");
		}
		self->dhcp_thread_queue.add(req);
	}
}


void InitAll(Handle<Object> exports, Handle<Object> module) {
	NodeDhclient::Init();

	Nan::Set(exports, Nan::New("_newClient").ToLocalChecked(),
			Nan::GetFunction(Nan::New<FunctionTemplate>(NodeDhclient::NewClient)).ToLocalChecked());

	// Handle<Object> consts = Object::New();
	// _errcmn::DefineConstants(consts);
	// exports->Set(String::NewSymbol("CONSTS"), consts);
}

NODE_MODULE(isc_dhclient, InitAll)
