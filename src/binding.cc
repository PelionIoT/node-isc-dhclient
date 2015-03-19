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

using namespace v8;



/**
 * LICENSE_IMPORT_BEGIN 9/7/14
 *
 * Macros below pulled from this project:
 *
 * https://github.com/bnoordhuis/node-buffertools/blob/master/buffertools.cc
 *
 * and include additions by WigWag.
 *
 * original license:

 * Copyright (c) 2010, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if NODE_MAJOR_VERSION > 0 || NODE_MINOR_VERSION > 10
# define UNI_BOOLEAN_NEW(value)                                               \
    v8::Boolean::New(args.GetIsolate(), value)
# define UNI_BUFFER_NEW(size)                                                 \
    node::Buffer::New(args.GetIsolate(), size)
# define UNI_CONST_ARGUMENTS(name)                                            \
    const v8::FunctionCallbackInfo<v8::Value>& name
# define UNI_ESCAPE(value)                                                    \
    return handle_scope.Escape(value)
# define UNI_ESCAPABLE_HANDLESCOPE()                                          \
    v8::EscapableHandleScope handle_scope(args.GetIsolate())
# define UNI_FUNCTION_CALLBACK(name)                                          \
    void name(const v8::FunctionCallbackInfo<v8::Value>& args)
# define UNI_HANDLESCOPE()                                                    \
    v8::HandleScope handle_scope(args.GetIsolate())
# define UNI_INTEGER_NEW(value)                                               \
    v8::Integer::New(args.GetIsolate(), value)
# define UNI_RETURN(value)                                                    \
    args.GetReturnValue().Set(value)
# define UNI_STRING_EMPTY()                                                   \
    v8::String::Empty(args.GetIsolate())
# define UNI_STRING_NEW(string, size)                                         \
    v8::String::NewFromUtf8(args.GetIsolate(),                                \
                            string,                                           \
                            v8::String::kNormalString,                        \
                            size)
# define UNI_THROW_AND_RETURN(type, message)                                  \
    do {                                                                      \
      args.GetIsolate()->ThrowException(                                      \
          type(v8::String::NewFromUtf8(args.GetIsolate(), message)));         \
      return;                                                                 \
    } while (0)
# define UNI_THROW_EXCEPTION(type, message)                                   \
    args.GetIsolate()->ThrowException(                                        \
        type(v8::String::NewFromUtf8(args.GetIsolate(), message)));
#else  // NODE_MAJOR_VERSION > 0 || NODE_MINOR_VERSION > 10
# define UNI_BOOLEAN_NEW(value)                                               \
    v8::Local<v8::Boolean>::New(v8::Boolean::New(value))
# define UNI_BUFFER_NEW(size)                                                 \
    v8::Local<v8::Object>::New(node::Buffer::New(size)->handle_)
# define UNI_CONST_ARGUMENTS(name)                                            \
    const v8::Arguments& name
# define UNI_ESCAPE(value)                                                    \
    return handle_scope.Close(value)
# define UNI_ESCAPABLE_HANDLESCOPE()                                          \
    v8::HandleScope handle_scope
# define UNI_FUNCTION_CALLBACK(name)                                          \
    v8::Handle<v8::Value> name(const v8::Arguments& args)
# define UNI_HANDLESCOPE()                                                    \
    v8::HandleScope handle_scope
# define UNI_INTEGER_NEW(value)                                               \
    v8::Integer::New(value)
# define UNI_RETURN(value)                                                    \
    return handle_scope.Close(value)
# define UNI_STRING_EMPTY()                                                   \
    v8::String::Empty()
# define UNI_STRING_NEW(string, size)                                         \
    v8::String::New(string, size)
# define UNI_THROW_AND_RETURN(type, message)                                  \
    return v8::ThrowException(v8::String::New(message))
# define UNI_THROW_EXCEPTION(type, message)                                   \
    v8::ThrowException(v8::String::New(message))
#endif  // NODE_MAJOR_VERSION > 0 || NODE_MINOR_VERSION > 10

// LICENSE_IMPORT_END

#define V8_IFEXIST_TO_INT32(v8field,cfield,val,v8obj) { val = v8obj->Get(String::New(v8field));\
		if(!val->IsUndefined() && val->IsNumber()) cfield = val->ToInteger()->Int32Value(); }
#define V8_IFEXIST_TO_DYN_CSTR(v8field,cfield,val,v8obj) { val = v8obj->Get(String::New(v8field));\
if(!val->IsUndefined() && val->IsString()) {\
	if(cfield) free(cfield);\
	v8::String::Utf8Value v8str(val);\
	cfield = strdup(v8str.operator *()); } }
#define V8_IFEXIST_TO_INT_CAST(v8field,cfield,val,v8obj,typ) { val = v8obj->Get(String::New(v8field));\
		if(!val->IsUndefined() && val->IsNumber()) cfield = (typ) val->ToInteger()->IntegerValue(); }
#define V8_IFEXIST_TO_INT_CAST_THROWBOUNDS(v8field,cfield,val,v8obj,typ,lowbound,highbound) { val = v8obj->Get(String::New(v8field));\
if(!val->IsUndefined() && val->IsNumber()) { \
int64_t cval = val->ToInteger()->IntegerValue();\
if(cval < ((int64_t) ((typ) lowbound)) || cval > ((int64_t) ((typ) highbound))) return ThrowException(Exception::TypeError(String::New(v8field" is out of bounds.")));\
else cfield = (typ) val->ToInteger()->IntegerValue(); } }

#define INT32_TO_V8(v8field,cfield,v8obj) v8obj->Set(String::New(v8field),Int32::New(cfield))
#define UINT32_TO_V8(v8field,cfield,v8obj) v8obj->Set(String::New(v8field),Integer::NewFromUnsigned(cfield))
#define INT_TO_V8(v8field,cfield,v8obj) v8obj->Set(String::New(v8field),Integer::New(cfield))
#define CSTR_TO_V8STR(v8field,cstr,v8obj) { if(cstr) v8obj->Set(String::New(v8field),String::New(cstr,strlen(cstr))); }

#define BOOLEAN_TO_V8(v8field,cfield) {   const char *_k = v8field;\
if(cfield) o->Set(String::New(_k),Boolean::New(true)); else o->Set(String::New(_k),Boolean::New(false)); }

#define MEMBLK_TO_NODE_BUFFER(v8field,cfield,strct,size,v8obj) \
{\
	Local<Value> _o = v8obj->Get(String::New(v8field));\
	char *m = NULL;\
	if(!_o->IsUndefined() && _o->IsObject()) {\
		m = node::Buffer::Data(_o->ToObject());\
	} else {\
		Handle<Object> buf = UNI_BUFFER_NEW(size);\
		m = node::Buffer::Data(buf);\
		v8obj->Set(String::New(v8field),buf);\
	}\
	memcpy(m,cfield,size);\
}

#define NODE_BUFFER_TO_MEMBLK(v8field,cfield,size,v8obj) \
{\
	Local<Value> _o = v8obj->Get(String::New(v8field)); \
	if(!_o->IsUndefined() && _o->IsObject()) { \
		char *m = node::Buffer::Data(_o->ToObject());\
		memcpy(cfield,m,size);\
	}\
}










class NodeDhclient : public node::ObjectWrap {
protected:
	dhclient_config _config;
public:
	enum work_code {
		DISCOVER_REQUEST,
		RELEASE,
		RELEASE_RE_REQUEST,
		RENEW
	};
	enum control_event_code {
		UNDEFINED,
		NEWADDRESS,     // a new address was obtained
		EXPIRED,        // the lease has expired
		RENEWAL_NOTIFY, // half the expire time is up, its time to renew
		NEW_LEASE       // a lease was acquired, but not necessarily a new address
	};

	static void do_workReq(uv_work_t *req);
	static void post_workReq(uv_work_t *req, int status);

	class workReq {
	protected:
//		uv_async_t async;  // used to tell node.js event loop we are done when work is done
		uv_work_t work;
		bool ref;
	public:
		work_code cmdcode;
		int _errno;     // the errno that happened on read if an error occurred.
		v8::Persistent<Function> onCompleteCB;
		v8::Persistent<Object> buffer; // Buffer object passed in
		uint32_t uint32t_data;
		char *_backing;    // backing of the passed in Buffer
		bool freeBacking;  // free the backing on delete?
		int len; // amount read or written
		NodeDhclient *self;
		int _reqSize;
		int retries;
		int timeout;

		workReq(NodeDhclient *i, work_code c) : ref(false), cmdcode(c),_errno(0),
				onCompleteCB(), buffer(),
				_backing(NULL), freeBacking(false), len(0), self(i), _reqSize(0),
				retries(DEFAULT_RETRIES), timeout(TIMEOUT_FOR_RETRY) {
			work.data = this;
			switch(c) {
			// special setup for certain requests?
			}
		}
		// complete() to be called from worker (6lbr) thread. Alerts v8 thread that this work is done.
		void complete() {
//			uv_async_send(&async);
		};
		workReq() = delete;
		// NOTE: workReq should be deleted from the v8 thread!!
		~workReq() {
			if(freeBacking) ::free(_backing);
//			uv_close((uv_handle_t *) &async,NULL);
		}
		void execute(uv_loop_t *l) {
			uv_queue_work(l,&work,do_workReq,post_workReq);
		}
	};



    static Persistent<Function> constructor;
    static Persistent<ObjectTemplate> prototype;

	static Handle<Value> SetConfig(const Arguments& args);
	static Handle<Value> GetConfig(const Arguments& args);

	static Handle<Value> RequestLease(const Arguments& args);
	static Handle<Value> NewClient(const Arguments& args);



	static Handle<Value> NewInstance(const Arguments& args) {
		HandleScope scope;
		int n = args.Length();
		Local<Object> instance;

		if(args.Length() > 0) {
			Handle<Value> argv[n];
			for(int x=0;x<n;x++)
				argv[x] = args[x];
			instance = NodeDhclient::constructor->NewInstance(n, argv);
		} else {
			instance = NodeDhclient::constructor->NewInstance();
		}

		return scope.Close(instance);
	}


	static Handle<Value> New(const Arguments& args) {
		HandleScope scope;

		NodeDhclient* obj = NULL;

		if (args.IsConstructCall()) {
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
			obj->Wrap(args.This());


			return args.This();
		} else {
		    // Invoked as plain function `MyObject(...)`, turn into construct call.
		    const int argc = 1;
		    Local<Value> argv[argc] = { args[0] };
		    return scope.Close(constructor->NewInstance(argc, argv));
		  }
	}




	static void Init() { //Handle<Object> exports) {
		// Prepare constructor template
		Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
		tpl->SetClassName(String::NewSymbol("IscDhclient"));
		tpl->InstanceTemplate()->SetInternalFieldCount(1);
		tpl->PrototypeTemplate()->SetInternalFieldCount(2);

		// Prototype
//		tpl->PrototypeTemplate()->Set(String::NewSymbol("start"), FunctionTemplate::New(Start)->GetFunction());
//		tpl->PrototypeTemplate()->Set(String::NewSymbol("stop"), FunctionTemplate::New(Stop)->GetFunction());
		tpl->PrototypeTemplate()->Set(String::NewSymbol("requestLease"), FunctionTemplate::New(RequestLease)->GetFunction());
		tpl->PrototypeTemplate()->Set(String::NewSymbol("setConfig"), FunctionTemplate::New(SetConfig)->GetFunction());
		tpl->PrototypeTemplate()->Set(String::NewSymbol("getConfig"), FunctionTemplate::New(GetConfig)->GetFunction());


		constructor = Persistent<Function>::New(tpl->GetFunction());
//		exports->Set(String::NewSymbol("IscDhclient"), constructor);
	}


	NodeDhclient() {
		init_defaults_config(&_config);
	}


};


Handle<Value> NodeDhclient::NewClient(const Arguments& args) {
	HandleScope scope;

	return scope.Close(NodeDhclient::NewInstance(args));

}

Persistent<Function> NodeDhclient::constructor;
Persistent<ObjectTemplate> NodeDhclient::prototype;

Handle<Value> NodeDhclient::SetConfig(const Arguments& args) {

}

Handle<Value> NodeDhclient::GetConfig(const Arguments& args) {

}



Handle<Value> NodeDhclient::RequestLease(const Arguments& args) {
	HandleScope scope;

	NodeDhclient* self = ObjectWrap::Unwrap<NodeDhclient>(args.This());

	NodeDhclient::workReq *req = new NodeDhclient::workReq(self,NodeDhclient::work_code::DISCOVER_REQUEST);

	if(args.Length() > 0) {
		 if(args[0]->IsFunction())
				req->onCompleteCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
		 else
			 return ThrowException(Exception::TypeError(String::New("bad param: requestLease([function]) -> Need [function]")));
	}

	req->execute(uv_default_loop());

	return scope.Close(Undefined());
}


void NodeDhclient::do_workReq(uv_work_t *req) {
	NodeDhclient::workReq *work = (NodeDhclient::workReq *) req->data;

	switch(work->cmdcode) {
	case DISCOVER_REQUEST:
	    {
	    	char *errstr = NULL;
	    	int ret = do_dhclient_request(&errstr, &work->self->_config);
	    	if(ret != 0) {
	    		DBG_OUT("Got error: do_dhclient_request() = %d\n",ret);
	    	}
	    }
		break;

	default:
		DBG_OUT("Don't know how to do work: %d\n", work->cmdcode);
	}
}

void NodeDhclient::post_workReq(uv_work_t *req, int status) {

}






//Persistent<Function> SixLBR::constructor;
//
//namespace TWlib {
//template<>   // allows us to use uip_ipaddr_t types in TW_KHash_32 table
//struct tw_hash<uip_ipaddr_t *> {
//	inline size_t operator()(const uip_ipaddr_t *s) const {
//		return (size_t) TWlib::data_hash_Hsieh((char *) s,sizeof(uip_ipaddr_t));
//	}
//};
//
//template<>   // allows us to use uip_ipaddr_t types in TW_KHash_32 table
//struct tw_hash<MAC_ADDR_6LBR *> {
//	inline size_t operator()(const MAC_ADDR_6LBR *s) const {
//		return (size_t) TWlib::data_hash_Hsieh((char *) s->val,sizeof(MAC_ADDR_6LBR_LEN));
//	}
//};

//#define CAT_STR(d,n,...)  if (n>0) { \
//    int r = snprintf(d,n,##__VA_ARGS__); \
//	n = n-r;                         \
//    d += r;                          \
//}

//char *str_uip_ipaddr(const uip_ipaddr_t *addr) {
//	int N = 40;
//	char *ret = (char *) malloc(N+1);
//	char *walk = ret;
//	memset(ret,0,N+1);
//	if (addr == NULL || addr->u8 == NULL) {
//		return ret;
//	}
////#if UIP_CONF_IPV6
//	uint16_t a;
//	unsigned int i;
//	int f;
//	for (i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
//		a = (addr->u8[i] << 8) + addr->u8[i + 1];
//		if (a == 0 && f >= 0) {
//			if (f++ == 0) {
//				CAT_STR(walk,N,"::");
//			}
//		} else {
//			if (f > 0) {
//				f = -1;
//			} else if (i > 0) {
//				CAT_STR(walk,N,":");
//			}
//			CAT_STR(walk,N,"%x", a);
//		}
//		if(N<1) break;
//	}
//	return ret;
////#else /* UIP_CONF_IPV6 */
////  PRINTA("%u.%u.%u.%u", addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3]);
////#endif /* UIP_CONF_IPV6 */
//}
//
//bool str_to_EUI48(char *str, uint8_t *bytes) {
//	bool ret = true;
//	int values[6];
//	int i;
//	char c;
//
//	if( 6 == sscanf( str, "%x:%x:%x:%x:%x:%x%c",
//		&values[0], &values[1], &values[2],
//		&values[3], &values[4], &values[5], &c ) )
//	{
//		/* convert to uint8_t */
//		for( i = 0; i < 6; ++i )
//			bytes[i] = (uint8_t) values[i];
//	}
//
//	else
//	{
//		ret = false;
//	}
//	return ret;
//}
//
//bool str_to_EUI64(char *str, uint8_t *bytes) {
//	bool ret = true;
//	int values[8];
//	int i;
//	char c;
//
//	if( 8 == sscanf( str, "%x:%x:%x:%x:%x:%x:%x:%x%c",
//		&values[0], &values[1], &values[2],
//		&values[3], &values[4], &values[5], &values[6], &values[7], &c ) )
//	{
//		/* convert to uint8_t */
//		for( i = 0; i < 8; ++i )
//			bytes[i] = (uint8_t) values[i];
//	}
//	else
//	{
//		ret = false;
//	}
//	return ret;
//}



//// this function: http://www.kegel.com/dkftpbench/nonblocking.html
//int setNonblocking(int fd)
//{
//    int flags;
//
//    /* If they have O_NONBLOCK, use the Posix way to do it */
//#if defined(O_NONBLOCK)
//    /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
//    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
//        flags = 0;
//    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
//#else
//    /* Otherwise, use the old way of doing it */
//    flags = 1;
//    return ioctl(fd, FIOBIO, &flags);
//#endif
//}
















//
//
//
//SixLBR::SixLBR() : ObjectWrap(), _6lbr_thread(), _async_6mac_arrival(), _async_control(), _mutex_opts(), _control(), _start_cond(),
//		_mac_set_cond(), running(false), nvm_data_p(NULL),
//		onSrcAddrChangeCB(), onFailureCB(), onInboundRawFrameCB(), onRoutingUpdateCB(), onIfUpCB(), onIfDownCB(),
//		work_queue(),
//		cmd_queue_6lbr_thread(),
//		v8_cmd_queue(MAX_QUEUED_V8IN_CMD, true),
//		magicNumTable(),
//		rplTable(),
//		macKeyTable(),
//		networkKey(),
//		threadUp(false), ifUp(false), ifDevName(NULL)
//	{
//		uv_async_init(uv_default_loop(), &_async_control, control_update);  // this needs fix, see DVC-48
//		uv_async_init(uv_default_loop(), &_async_6mac_arrival, six_mac_arrival);
//		uv_mutex_init(&_mutex_opts);
//		uv_mutex_init(&_mutex_networkKey);
//		uv_mutex_init(&_mutex_keepalive);
//		_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX] = uv_hrtime();
//		_last_keepalive[SIXLBR_LAST_TIMER_INDEX] =	_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX];
//		uv_cond_init(&_start_cond);
//		uv_cond_init(&_mac_set_cond);
//		uv_mutex_init(&_control);
//		slip_config_init(config);
//		memset(&tunInfo,0,sizeof(_6LBR_tun_info));
//		if(pipe(wakeup_pipe)) {
//			perror("ERROR: pipe() failed.\n");
//		} else {
//			setNonblocking(wakeup_pipe[0]);
//		}
//	}
//
//SixLBR::~SixLBR() {
//	close(wakeup_pipe[0]); close(wakeup_pipe[1]);
//	if(ifDevName) free(ifDevName);
//}


/** SixLBR(opts)
 * opts {
 * 	    ifname: "tun77"
 * }
 * @param args
 * @return
 **/
//Handle<Value> SixLBR::New(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = NULL;
//
//	if (args.IsConstructCall()) {
//	    // Invoked as constructor: `new MyObject(...)`
////	    double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
////		if(args.Length() > 0) {
////			if(!args[0]->IsString()) {
////				return ThrowException(Exception::TypeError(String::New("Improper first arg to ProcFs cstor. Must be a string.")));
////			}
////			Local<Value> ifname = args[0]->ToObject()->Get(String::New(""));
//
////			v8::String::Utf8Value v8str(args[0]);
//			//obj->setIfName(v8str.operator *(),v8str.length());
//			obj = new SixLBR();
//			my6LBR = obj; // assign the TLS variable too..
////		} else {
////			return ThrowException(Exception::TypeError(String::New("First arg must be a string path.")));
////		}
//		obj->Wrap(args.This());
//
//
//		return args.This();
//	} else {
//	    // Invoked as plain function `MyObject(...)`, turn into construct call.
//	    const int argc = 1;
//	    Local<Value> argv[argc] = { args[0] };
//	    return scope.Close(constructor->NewInstance(argc, argv));
//	  }
//
//}



//void SixLBR::Init(Handle<Object> exports) {
//	// Prepare constructor template
//	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
//	tpl->SetClassName(String::NewSymbol("SixLBR"));
//	tpl->InstanceTemplate()->SetInternalFieldCount(1);
//
//	// Prototype
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("start"), FunctionTemplate::New(Start)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("stop"), FunctionTemplate::New(Stop)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("setConfig"), FunctionTemplate::New(SetConfig)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("getConfig"), FunctionTemplate::New(GetConfig)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onSrcAddrChange"), FunctionTemplate::New(OnSrcAddrChange)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onFailure"), FunctionTemplate::New(OnFailure)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onInboundRawFrame"), FunctionTemplate::New(OnInboundRawFrame)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onRoutingUpdate"), FunctionTemplate::New(OnRoutingUpdate)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onIfUp"), FunctionTemplate::New(OnIfUp)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onIfDown"), FunctionTemplate::New(OnIfDown)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("onInboundChannelStudy"), FunctionTemplate::New(OnInboundChannelStudy)->GetFunction());
//
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("send802154Frame"), FunctionTemplate::New(Send802154Frame)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("sendMulticastPkt"), FunctionTemplate::New(SendMulticastPkt)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("setSlipChannel"), FunctionTemplate::New(SetSlipChannel)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("setRPLOptions"), FunctionTemplate::New(SetRPLOptions)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("channelStudy"), FunctionTemplate::New(ChannelStudy)->GetFunction());
//
////	static Handle<Value> GetRoutes(const Arguments& args);
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("getRoutes"), FunctionTemplate::New(GetRoutes)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("addSymmetricDeviceKey"), FunctionTemplate::New(AddSymmetricDeviceKey)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("clearSymmetricDeviceKey"), FunctionTemplate::New(ClearSymmetricDeviceKey)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("getSymmetricDeviceKey"), FunctionTemplate::New(GetSymmetricDeviceKey)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("setSymmetricNetworkKey"), FunctionTemplate::New(SetSymmetricNetworkKey)->GetFunction());
//	tpl->PrototypeTemplate()->Set(String::NewSymbol("getSymmetricNetworkKey"), FunctionTemplate::New(GetSymmetricNetworkKey)->GetFunction());
//
//
//	constructor = Persistent<Function>::New(tpl->GetFunction());
//	exports->Set(String::NewSymbol("SixLBR"), constructor);
//}

/**
 * @method onSrcAddrChange
 * @param cb A callback which is called when the source address changes. The format of the callback is:<br>
 * <pre>
 * callback(addr) {}
 * </pre>
 * where addr is a node.js Buffer object with the new address information...
 */
//Handle<Value> SixLBR::OnSrcAddrChange(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onSrcAddrChangeCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
//
//Handle<Value> SixLBR::GetRoutes(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	Local<Object> ret = Object::New();
//
//	uip6Table::HashIterator iter(obj->rplTable);
//	do {
//		if(iter.data()) {
//			char *k = str_uip_ipaddr(&(*iter.data())->ipaddr);
//			if(k) {
//				if(&(*iter.data())->nexthop) {
//	 				char *v = str_uip_ipaddr(&(*iter.data())->nexthop);
//					ret->Set(String::New(k),String::New(v));
//					if(v) free(v);
//				} else {
//					ret->Set(String::New(k),Null());
//				}
//			} else {
//	    		LOG6LBR_ERROR("NULL address entry in rplTable. This is wrong.\n");
//			}
//			if(k) free(k);
//		} else {
//    		LOG6LBR_ERROR("NULL entry in rplTable. This is wrong.\n");
//		}
////		ret->Set()
////		iter.getNext();
//	} while(iter.getNext());
//	iter.release();
////		if(args[0]->IsFunction()) {
////			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
////			// TODO get routes
////		} else {
////			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
////		}
//	return scope.Close(ret);
//}
//
//Handle<Value> SixLBR::SetSymmetricNetworkKey(const Arguments& args) {
//	HandleScope scope;
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//	if(args.Length() > 0 && args[0]->IsObject()) {
//		char *key = node::Buffer::Data(args[0]->ToObject());
//		int keylen = node::Buffer::Length(args[0]->ToObject());
//		if(keylen < AES_KEY_6LBR_LEN) {
//			return ThrowException(Exception::TypeError(String::New("setSymmetricNetworkKey(Buffer) requires 16-byte Buffer. Buffer too small.")));
//		}
//		uv_mutex_lock(&obj->_mutex_networkKey);
//		memcpy(obj->networkKey.val,key,AES_KEY_6LBR_LEN);
//		uv_mutex_unlock(&obj->_mutex_networkKey);
//	} else {
//		return ThrowException(Exception::TypeError(String::New("setSymmetricNetworkKey(Buffer) requires 16-byte Buffer")));
//	}
//	return scope.Close(Undefined());
//}
//
//Handle<Value> SixLBR::GetSymmetricNetworkKey(const Arguments& args) {
//	HandleScope scope;
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//	Local<Object> ret = UNI_BUFFER_NEW(AES_KEY_6LBR_LEN);
//	char *b = node::Buffer::Data(ret);
//	uv_mutex_lock(&obj->_mutex_networkKey);
//	memcpy(b,obj->networkKey.val,AES_KEY_6LBR_LEN);
//	uv_mutex_unlock(&obj->_mutex_networkKey);
//	return scope.Close(ret);
//}
//
//Handle<Value> SixLBR::AddSymmetricDeviceKey(const Arguments& args) {
//	HandleScope scope;
//
//	if(args.Length() > 1 && args[0]->IsString() && args[1]->IsObject()) {
//		SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//		MAC_ADDR_6LBR macaddr;
//		AES_KEY_6LBR aeskey;
//		char *key = node::Buffer::Data(args[1]->ToObject());
//		int keylen = node::Buffer::Length(args[1]->ToObject());
//		if(key && keylen > 15) {
//			v8::String::AsciiValue v8str(args[0]);
//			if(v8str.operator *() && v8str.length() > 1) {
//				if(str_to_EUI64(v8str.operator *(), macaddr.val)) {
//					memcpy(aeskey.val,key,AES_KEY_6LBR_LEN);
//					obj->macKeyTable.addReplace(macaddr,aeskey);
//				} else {
//					return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) requires valid EUI-64 MAC address")));
//				}
//			} else
//				return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) could not convert string to ASCII. Invalid chars?")));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) requires 16-byte Buffer")));
//		}
//	} else {
//		return ThrowException(Exception::TypeError(String::New("bad parameters, call convention is: addSymmetricKey(string,Buffer)")));
//	}
//	return scope.Close(Undefined());
//}
//
//Handle<Value> SixLBR::ClearSymmetricDeviceKey(const Arguments& args) {
//	HandleScope scope;
//	bool ret = false;
//	if(args.Length() > 0 && args[0]->IsString()) {
//		SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//		MAC_ADDR_6LBR macaddr;
//		AES_KEY_6LBR aeskey;
//		v8::String::AsciiValue v8str(args[0]);
//		if(v8str.operator *() && v8str.length() > 1) {
//			if(str_to_EUI64(v8str.operator *(), macaddr.val)) {
//				ret = obj->macKeyTable.remove(macaddr);
//			} else {
//				return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) requires valid EUI-64 MAC address")));
//			}
//		} else
//			return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) could not convert string to ASCII. Invalid chars?")));
//	} else {
//		return ThrowException(Exception::TypeError(String::New("bad parameters, call convention is: clearSymmetricKey(string)")));
//	}
//
//	return scope.Close(Boolean::New(ret));
//}
//
//Handle<Value> SixLBR::GetSymmetricDeviceKey(const Arguments& args) {
//	HandleScope scope;
//	Local<Object> ret;
//	if(args.Length() > 0 && args[0]->IsString()) {
//		SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//		MAC_ADDR_6LBR macaddr;
////		AES_KEY_6LBR aeskey;
//		v8::String::AsciiValue v8str(args[0]);
//		if(v8str.operator *() && v8str.length() > 1) {
//			if(str_to_EUI64(v8str.operator *(), macaddr.val)) {
//				uint8_t *aes = NULL;
//				uint8_t *mac = macaddr.val;
//				if(lookup_symmetric_device_key_6LBR(mac, &aes)) {
//					ret = UNI_BUFFER_NEW(AES_KEY_6LBR_LEN);
//					char *b = node::Buffer::Data(ret);
//					memcpy(b,aes,AES_KEY_6LBR_LEN);
//				}
////				if(obj->macKeyTable.find(macaddr,aeskey)) {
////					ret = UNI_BUFFER_NEW(AES_KEY_6LBR_LEN);
////					char *b = node::Buffer::Data(ret);
////					memcpy(b,aeskey.val,AES_KEY_6LBR_LEN);
////				}
//			} else {
//				return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) requires valid EUI-64 MAC address")));
//			}
//		} else
//			return ThrowException(Exception::TypeError(String::New("addSymmetricKey(string,Buffer) could not convert string to ASCII. Invalid chars?")));
//	} else {
//		return ThrowException(Exception::TypeError(String::New("bad parameters, call convention is: clearSymmetricKey(string)")));
//	}
//
//	if(ret.IsEmpty())
//		return scope.Close(Undefined());
//	else
//		return scope.Close(ret);
//}
//
//
///**
// * @method onFailure()
// * @param cb A callback which is called when the 6LBR thread shuts down because of a critical failure. This is not called
// * if a controlled shutdown is requested.
// * <pre>
// * callback(err) {}
// * </pre>
// * where err is an Error object explaining the failure
// */
//Handle<Value> SixLBR::OnFailure(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onFailureCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
///**
// * @method onInboundRawFrameCB()
// * @param cb A callback which is called when the 6LBR thread gets a raw MAC frame which does not belong to the 6LoWPAN
// * protocol layer, and which would otherwise be dropped by 6LBR.
// * <pre>
// * callback(err) {}
// * </pre>
// * where err is an Error object explaining the failure
// */
//Handle<Value> SixLBR::OnInboundRawFrame(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onInboundRawFrameCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
///**
// * @method onRoutingUpdateCB()
// * @param cb A callback which is called when the 6LBR Routing table is updated
// * <pre>
// * callback(err) {}
// * </pre>
// * where err is an Error object explaining the failure
// */
//Handle<Value> SixLBR::OnRoutingUpdate(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onRoutingUpdateCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
//
///**
// * @method onIfUp
// * @param cb A callback which is called when a given interface should be brought up. (6LBR itself does not do this).
// * The format of the callback is:<br>
// * <pre>
// * callback(ifname) { // where ifname is a String
// * }
// * </pre>
// * where addr is a node.js Buffer object with the new address information...
// */
//Handle<Value> SixLBR::OnIfUp(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onIfUpCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
///**
// * @method onIfDown
// * @param cb A callback which is called when a given interface should be brought down. (6LBR itself does not do this).
// * The format of the callback is:<br>
// * <pre>
// * callback(ifname) { // where ifname is a String
// * }
// * </pre>
// * where addr is a node.js Buffer object with the new address information...
// */
//Handle<Value> SixLBR::OnIfDown(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onIfDownCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
///**
// * @method onInboundChannelStudyCB()
// * @param cb A callback which is called Slip radio analyze the channel traffic
// * and reply to 6lbr which contains channel readings
// * <pre>
// * callback(err) {}
// * </pre>
// * where err is an Error object explaining the failure
// */
//Handle<Value> SixLBR::OnInboundChannelStudy(const Arguments& args) {
//	HandleScope scope;
//	if(args.Length() > 0) {
//		if(args[0]->IsFunction()) {
//			SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//			// install callback...
//			obj->onInboundChannelStudyCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		} else {
//			return ThrowException(Exception::TypeError(String::New("Passed in argument must be a Function.")));
//		}
//	}
//	return scope.Close(Undefined());
//}
//
//void SixLBR::control_update(uv_async_t *handle, int status /*UNUSED*/) {
// //   SixLBR::control_event *ev = (SixLBR::control_event *) handle->data;
//
//	SixLBR::control_event ev;
//
//	SixLBR *lbr = (SixLBR *) handle->data;
//
//	while(lbr->v8_cmd_queue.removeMv(ev)) {
//
//    switch(ev.code) {
//    	case STARTED:
////    		printf("Got STARTED.\n");
//    		if(!ev.self->ifUp) {
//        	    { // same as IFUP.. doing this b/c in rare cases for some reason the uv_async_t _control will
//        	      // will not call IFUP when the event is called in the thread at early startup.
//        	    	if(!lbr->ifDevName) {
//        	    		LOG6LBR_ERROR(" Uhoh. No ifDevName, and IFUP event not responded to by control_update.\n");
//        	    	} else {
//						const unsigned argc = 1;
//						Local<Value> argv[argc];
//						if(!ev.self->onIfUpCB.IsEmpty()) {
//							Local<Object> buf = UNI_BUFFER_NEW(sizeof(uip_ipaddr_t));
//							argv[0] = String::New((char *) lbr->ifDevName);
//							lbr->onIfUpCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//							lbr->ifUp = true; // FIXME makes big assumption that the interface did indeed come up. Need to fix this.
//						}
//        	    	}
//        	    }
//    		}
//    		if(ev.success) {
//    			if(!ev.onSuccessCB.IsEmpty())
//    				ev.onSuccessCB->Call(Context::GetCurrent()->Global(),0,NULL); // call success callback
//    		} else {
//    			if(!ev.onFailCB.IsEmpty())
//    				ev.onFailCB->Call(Context::GetCurrent()->Global(),0,NULL); // call success callback
//    		}
//    		break;
//    	case STOPPED:
////    		printf("Got STOPPED.\n");
////    		printf("STOPPED >>>>> HANDLE COUNT: %d\n", ((uv_handle_t *) &ev->self->_async_control)->loop->active_handles);
//    		if(ev.success) {
//    			if(!ev.onSuccessCB.IsEmpty())
//    				ev.onSuccessCB->Call(Context::GetCurrent()->Global(),0,NULL); // call success callback
//    		} else {
//    			if(!ev.onFailCB.IsEmpty())
//    				ev.onFailCB->Call(Context::GetCurrent()->Global(),0,NULL); // call success callback
//    		}
//    		break;
//    	case NEWSRCADDR:
//        	{
////        		printf("NEWSRCADDR >>>>> HANDLE COUNT: %d\n", ((uv_handle_t *) &ev->self->_async_control)->loop->active_handles);
//
//        		const unsigned argc = 1;
//        		Local<Value> argv[argc];
//
//        		if(!ev.self->onSrcAddrChangeCB.IsEmpty()) {
//        			Local<Object> buf = UNI_BUFFER_NEW(sizeof(uip_ipaddr_t));
//        			memcpy(node::Buffer::Data(buf),ev.managed_aux,sizeof(uip_ipaddr_t));
//        			argv[0] = buf; // the Buffer has the address in it...
//        			ev.self->onSrcAddrChangeCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//        		}
//        	}
//    		break;
//
//    	case IFUP:
//    	    {
//        		const unsigned argc = 1;
//        		Local<Value> argv[argc];
//        		if(!ev.self->onIfUpCB.IsEmpty()) {
//        			Local<Object> buf = UNI_BUFFER_NEW(sizeof(uip_ipaddr_t));
//        			argv[0] = String::New((char *) ev.managed_aux);
//        			ev.self->onIfUpCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//        			ev.self->ifUp = true; // FIXME makes big assumption that the interface did indeed come up. Need to fix this.
//        		}
//        		ev.self->sigIfChange();
//    	    }
//    	    break;
//    	case IFDOWN:
//    	    {
//        		const unsigned argc = 1;
//        		Local<Value> argv[argc];
//        		if(!ev.self->onIfDownCB.IsEmpty()) {
//        			Local<Object> buf = UNI_BUFFER_NEW(sizeof(uip_ipaddr_t));
//        			argv[0] = String::New((char *) ev.managed_aux);
//        			ev.self->onIfDownCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//        		}
//        		ev.self->sigIfChange();
//    	    }
//    	    break;
//    	case CRITICAL_FAILURE:
//			{
//				const unsigned argc = 1;
//				Local<Value> argv[argc];
//				if(!ev.self->onFailureCB.IsEmpty()) {
//					int _errno = *((int *) ev.managed_aux);
//					argv[0] = _errcmn::errno_to_JSError(_errno, "node-6lbr critical failure: ");
//					ev.self->onFailureCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//				}
//			}
//			break;
//    	case INBOUND_RAW_FRAME:
//			{
//				const unsigned argc = 2;
//				Local<Value> argv[argc];
//				if(!ev.self->onInboundRawFrameCB.IsEmpty()) {
//					SixLBR::macFrame *frame = static_cast<SixLBR::macFrame *>(ev.managed_aux);
//					argv[0] = Integer::New(frame->getMagic());
//					argv[1] = frame->toBuffer()->ToObject();
////					argv[0] = _errcmn::errno_to_JSError(_errno, "node-6lbr critical failure: ");
//					ev.self->onInboundRawFrameCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//				}
//			}
//			break;
//    	case ROUTING_UPDATE:
//			{
//				if(!ev.self->onRoutingUpdateCB.IsEmpty()) {
//					ev.self->onRoutingUpdateCB->Call(Context::GetCurrent()->Global(),0,NULL); // call success callback
//				}
//			}
//			break;
//    	case CHANNEL_STUDY:
//			{
//				const unsigned argc = 1;
//				Local<Value> argv[argc];
//				if(!ev.self->onInboundChannelStudyCB.IsEmpty()) {
//					SixLBR::channelStudyData *data = static_cast<SixLBR::channelStudyData *>(ev.managed_aux);
//					argv[0] = data->toBuffer()->ToObject();
//					ev.self->onInboundChannelStudyCB->Call(Context::GetCurrent()->Global(),argc,argv); // call success callback
//				}
//			}
//			break;
//
//    }
//
//	} // end while
//
//}
//
//void SixLBR::six_mac_arrival(uv_async_t *handle, int status /*UNUSED*/) {
//    SixLBR::control_event *ev = (SixLBR::control_event *) handle->data;
//
//    // TODO
//}
//
//bool SixLBR::setupThread() {
//	uv_mutex_lock(&_control);
//	bool ret = threadUp;
//	uv_mutex_unlock(&_control);
//	if(!ret) {
////		printf("(pre-up) HANDLE COUNT: %d\n", ((uv_handle_t *) &this->_start_cond)->loop->active_handles);
//		this->Ref();
////		uv_ref((uv_handle_t *)&this->_start_cond); // don't uv_ref uv_default_loop anymore - see this: https://groups.google.com/forum/#!topic/nodejs/530YS0RB42w
//		uv_thread_create(&this->_6lbr_thread,contiki_main_thread,this); // start thread
//		uv_mutex_lock(&_control);
//		uv_cond_wait(&_start_cond, &_control); // wait for thread's start...
//		ret = threadUp;
//		uv_mutex_unlock(&_control);
//	}
//	return ret;
//}
//
//void SixLBR::sigThreadUp() {
//	uv_mutex_lock(&_control);
//	threadUp = true;
//	uv_cond_signal(&_start_cond);
//	uv_mutex_unlock(&_control);
//}
//
//void SixLBR::sigThreadDown() {
//	uv_mutex_lock(&_control);
//	threadUp = false;
//	uv_cond_signal(&_start_cond);
//	uv_mutex_unlock(&_control);
//}
//
//void SixLBR::sigIfChange() {
//	uv_mutex_lock(&_control);
//	uv_cond_signal(&_mac_set_cond);
//	uv_mutex_unlock(&_control);
//}
//
//
///**
// * Usage: sixlbr.start(function(){ console.log("success"); }, function() { console.log("failure") });
// */
//Handle<Value> SixLBR::Start(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	// start thread... if not started
//	bool ret = obj->setupThread();
//
//	SixLBR::control_event ev(DOSTART,obj);
//
//	if(ret) {
//		if(args.Length() > 0 && args[0]->IsFunction()) {
//			ev.onSuccessCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//		}
//		if(args.Length() > 1 && args[1]->IsFunction()) {
//			ev.onFailCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//		}
//	} else if (args.Length() > 1 && args[1]->IsFunction()){ // immediate failure
//		Local<Function> failcb = Local<Function>::Cast(args[1]);
//		failcb->Call(Context::GetCurrent()->Global(),0,NULL); // call failure callback
//	}
//
//	obj->cmd_queue_6lbr_thread.add(ev); // will awake thread... if 6LBR is not running...
//
//	return scope.Close(Undefined());
//}
//
//Handle<Value> SixLBR::Stop(const Arguments& args) {
//	// will have to signal a process in contiki to start clean shutdown...
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	SixLBR::control_event ev(DOSTOP,obj);
//
//	if(args.Length() > 0 && args[0]->IsFunction()) {
//		ev.onSuccessCB = Persistent<Function>::New(Local<Function>::Cast(args[0]));
//	}
//	if(args.Length() > 1 && args[1]->IsFunction()) {
//		ev.onFailCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//	}
//
//	obj->cmd_queue_6lbr_thread.add(ev); // will awake thread... if 6LBR is not running...
//	obj->set_wakeup(); // wakeup thread if in select()...
//
//	return scope.Close(Undefined());
//}
//
///**
// * @method send802154Frame
// * @param data (Buffer} buffer with entire raw MAC frame
// * @param callback {function} cb(err)
// * @param options {object}
// * options: {
// *    magic: 65535, // magic number use to find reply (2nd and 3rd byte of frame)
// *    replyCB : function(data) {}
// * }
// */
//Handle<Value> SixLBR::Send802154Frame(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	SixLBR::workReq *req = new SixLBR::workReq(obj,SixLBR::DORAWWRITE);
//
//	if(args.Length() > 0 && args[0]->IsObject() && Buffer::HasInstance(args[0])) {
//		req->len = node::Buffer::Length(args[0]->ToObject());
//		if(req->len < 3) {
//			return ThrowException(Exception::TypeError(String::New("node-6lbr -> Buffer must have length >= 3: send6MACFrame(data {Buffer}, callback)")));
//		}
//		req->buffer = Persistent<Object>::New(args[0]->ToObject()); // keep the Buffer persistent until the write is done... (will be removed when req is deleted)
//		req->_backing = node::Buffer::Data(args[0]->ToObject());
//
//
//		if(args.Length() > 1 && args[1]->IsFunction()) {
//			req->onSendCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//		}
//
//		if(args.Length() > 2 && args[2]->IsObject()) {
//			Local<Object> o = args[2]->ToObject();
//			if(o->Get(String::New("replyCB"))->IsFunction()) {
//				req->onReplyCB = Persistent<Function>::New(Local<Function>::Cast(o->Get(String::New("replyCB")->IsFunction())));
//			}
//			if(o->Get(String::New("magic"))->IsInt32()) {
//				// TODO
//			}
//		}
//
//		obj->work_queue.add(req);
//		obj->set_wakeup(); // wakeup 6lbr thread if blocked in select()...
//	} else {
//		return ThrowException(Exception::TypeError(String::New("node-6lbr -> Need at least two params: send6MACFrame(data {Buffer}, callback)")));
//	}
//
//	return scope.Close(Undefined());
//}
//
//
//
//void SixLBR::post_stdCommand(uv_async_t *handle, int status /* unused apparently */) {
//	workReq *req = (workReq *) handle->data;
//	req->buffer.Clear(); // don't need reference anymore
//	// TODO - use callbacks to tell v8 data is sent
//	if(!req->onReplyCB.IsEmpty()) {
//
//	} else if(!req->onSendCB.IsEmpty()) {
//		if(req->_errno == 0) {
//			req->onSendCB->Call(Context::GetCurrent()->Global(),0,NULL);
//		} else {
//			// TODO
//		}
//	}
//};
//
//
///**
// * @method sendMulticastPkt
// * @param data (MAC_ADDR)
// * @param callback {function} cb(err)
// * @param options {object}
// * options: {
// *    replyCB : function(data) {}
// * }
// */
//Handle<Value> SixLBR::SendMulticastPkt(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	SixLBR::workReq *req = new SixLBR::workReq(obj,SixLBR::DOMULTICAST);
//
//	if(args.Length() > 0 && args[0]->IsObject() && Buffer::HasInstance(args[0])) {
//		req->len = node::Buffer::Length(args[0]->ToObject());
//		if(req->len < 3) {
//			return ThrowException(Exception::TypeError(String::New("node-6lbr -> Buffer must have length >= 3: sendMulticastPkt(data {MAC_ADDR}, callback)")));
//		}
//		req->buffer = Persistent<Object>::New(args[0]->ToObject()); // keep the Buffer persistent until the write is done... (will be removed when req is deleted)
//		req->_backing = node::Buffer::Data(args[0]->ToObject());
//
//		// dont need a callback on this one...
//		// if(args.Length() > 1 && args[1]->IsFunction()) {
//		// 	req->onSendCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//		// }
//
//		// if(args.Length() > 2 && args[2]->IsObject()) {
//		// 	Local<Object> o = args[2]->ToObject();
//		// 	if(o->Get(String::New("replyCB"))->IsFunction()) {
//		// 		req->onReplyCB = Persistent<Function>::New(Local<Function>::Cast(o->Get(String::New("replyCB")->IsFunction())));
//		// 	}
//		// }
//
//		obj->work_queue.add(req);
//		obj->set_wakeup(); // wakeup 6lbr thread if blocked in select()...
//	} else {
//		return ThrowException(Exception::TypeError(String::New("node-6lbr -> Need at least two params: sendMulticastPkt(data {Buffer}, callback)")));
//	}
//
//	return scope.Close(Undefined());
//}
//
///**
// * @method setSlipChannel
// * @param channel (11-26)
// * }
// */
//Handle<Value> SixLBR::SetSlipChannel(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	if(args.Length() > 0 && args[0]->IsUint32()) {
//		SixLBR::workReq *req = new SixLBR::workReq(obj,SixLBR::DOSWITCHCHANNEL);
//		req->uint32t_data = args[0]->Uint32Value();
//		obj->work_queue.add(req);
//		obj->set_wakeup(); // wakeup 6lbr thread if blocked in select()...
//	} else {
//		return ThrowException(Exception::TypeError(String::New("node-6lbr -> Need at least one params: setSlipChannel(channel)")));
//	}
//
//	return scope.Close(Undefined());
//}
//
///**
// * @method setRPLOptions
// * @param Array of 2 Bytes - control message interval, route lifetime
// * }
// */
//Handle<Value> SixLBR::SetRPLOptions(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//	SixLBR::workReq *req = new SixLBR::workReq(obj,SixLBR::DORPLOPTIONS);
//	if(args.Length() > 0 && args[0]->IsObject() && Buffer::HasInstance(args[0])) {
//		req->len = node::Buffer::Length(args[0]->ToObject());
//		if(req->len < 2) {
//			return ThrowException(Exception::TypeError(String::New("node-6lbr -> Buffer must have length >= 2: setRPLOptions(data {interval, lifetime})")));
//		}
//		req->buffer = Persistent<Object>::New(args[0]->ToObject()); // keep the Buffer persistent until the write is done... (will be removed when req is deleted)
//		req->_backing = node::Buffer::Data(args[0]->ToObject());
//
//		obj->work_queue.add(req);
//		obj->set_wakeup(); // wakeup 6lbr thread if blocked in select()...
//	} else {
//		return ThrowException(Exception::TypeError(String::New("node-6lbr -> Need at least one params: setRPLOptions()")));
//	}
//
//	return scope.Close(Undefined());
//}
//
//
///**
// * @method channelStudy
// * @param data (Buffer} buffer with interval and default channel
// * @param callback {function} cb(err)
// * @param options {object}
// * options: {
// *    replyCB : function(data) {}
// * }
// */
//Handle<Value> SixLBR::ChannelStudy(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//
//	SixLBR::workReq *req = new SixLBR::workReq(obj,SixLBR::DOCHANNELSTUDY);
//
//	if(args.Length() > 0 && args[0]->IsObject() && Buffer::HasInstance(args[0])) {
//		req->len = node::Buffer::Length(args[0]->ToObject());
//		if(req->len < 2) {
//			return ThrowException(Exception::TypeError(String::New("node-6lbr -> Buffer must have length >= 2: channelStudy(data {Buffer}, callback)")));
//		}
//		req->buffer = Persistent<Object>::New(args[0]->ToObject()); // keep the Buffer persistent until the write is done... (will be removed when req is deleted)
//		req->_backing = node::Buffer::Data(args[0]->ToObject());
//
//
//		// if(args.Length() > 1 && args[1]->IsFunction()) {
//		// 	req->onSendCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//		// }
//
//		// if(args.Length() > 2 && args[2]->IsObject()) {
//		// 	Local<Object> o = args[2]->ToObject();
//		// 	if(o->Get(String::New("replyCB"))->IsFunction()) {
//		// 		req->onReplyCB = Persistent<Function>::New(Local<Function>::Cast(o->Get(String::New("replyCB")->IsFunction())));
//		// 	}
//		// 	if(o->Get(String::New("magic"))->IsInt32()) {
//		// 		// TODO
//		// 	}
//		// }
//
//		obj->work_queue.add(req);
//		obj->set_wakeup(); // wakeup 6lbr thread if blocked in select()...
//	} else {
//		return ThrowException(Exception::TypeError(String::New("node-6lbr -> Need at least two params: channelStudy(data {Buffer}, callback)")));
//	}
//
//	return scope.Close(Undefined());
//}
//
//
///**
// *
// * @method SetConfig
// * Usage: sixlbr.setConfig(
// * { // config data
// * },
// * function(){ console.log("success"); },
// * function() { console.log("failure") });
// *
// */
//Handle<Value> SixLBR::SetConfig(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//	obj->setupThread();
//
////	obj->config.
//
//	if(args.Length() > 0 && args[0]->IsObject()) {
////		Local<Array> keys = base->GetPropertyNames();
////		for(int n=0;n<keys->Length();n++) {
////			Local<String> keyname = keys->Get(n)->ToString();
////			tpl->InstanceTemplate()->Set(keyname, base->Get(keyname));
////		}
//		Local<Object> o = args[0]->ToObject();
//		Local<Value> v;
//		V8_IFEXIST_TO_INT_CAST("baudrate",obj->config.baudrate,v,o,int);
//
//		V8_IFEXIST_TO_INT_CAST_THROWBOUNDS("log_level",obj->config.Log6lbr_level,v,o,int8_t,0,127);
//		V8_IFEXIST_TO_INT_CAST("log_services",obj->config.Log6lbr_services,v,o,uint32_t);
//
//
//		V8_IFEXIST_TO_DYN_CSTR("host",obj->config.slip_config_host,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("port",obj->config.slip_config_port,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("siodev",obj->config.slip_config_siodev,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("tundev",obj->config.slip_config_tundev,v,o);
//
//		V8_IFEXIST_TO_INT_CAST("slip_basedelay",obj->config.slip_config_basedelay,v,o,uint16_t);
//
//		v = o->Get(String::New("use_raw_ethernet"));
//		if(!v->IsUndefined() && v->IsBoolean()) { if(v->ToBoolean()->IsTrue()) obj->config.use_raw_ethernet = 1; else obj->config.use_raw_ethernet = 0; }
//		v = o->Get(String::New("ethernet_has_fcs"));
//		if(!v->IsUndefined() && v->IsBoolean()) { if(v->ToBoolean()->IsTrue()) obj->config.ethernet_has_fcs = 1; else obj->config.ethernet_has_fcs = 0; }
//		v = o->Get(String::New("flowcontrol"));
//		if(!v->IsUndefined() && v->IsBoolean()) { if(v->ToBoolean()->IsTrue()) obj->config.slip_config_flowcontrol = 1; else obj->config.slip_config_flowcontrol = 0; }
//
//		V8_IFEXIST_TO_DYN_CSTR("ifup_script",obj->config.slip_config_ifup_script,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("ifdown_script",obj->config.slip_config_ifdown_script,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("www_root",obj->config.slip_config_www_root,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("plugins",obj->config.slip_config_plugins,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("watchdog_file_name",obj->config.watchdog_file_name,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("ip_config_file_name",obj->config.ip_config_file_name,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("node_config_file_name",obj->config.node_config_file_name,v,o);
//
//#ifdef SIXLBR_FOR_WW_RELAY_V1
//		V8_IFEXIST_TO_DYN_CSTR("reset_GPIO_path_firmware_mc1322",obj->config.reset_GPIO_path_firmware_mc1322,v,o);
//		V8_IFEXIST_TO_DYN_CSTR("firmware_path_mc1322",obj->config.firmware_path_mc1322,v,o);
//		V8_IFEXIST_TO_INT_CAST_THROWBOUNDS("firmware_load_delay",obj->config.firmware_load_delay,v,o,uint16_t,0,(0-1));  // in useconds
//		V8_IFEXIST_TO_INT_CAST("programming_baudrate",obj->config.programming_baudrate,v,o,int);
//#endif
//
//		V8_IFEXIST_TO_INT_CAST("watchdog_interval",obj->config.watchdog_interval,v,o,int);
//
//		V8_IFEXIST_TO_INT_CAST_THROWBOUNDS("slip_basedelay",obj->config.slip_config_basedelay,v,o,uint16_t,0,(0-1));
//
//		V8_IFEXIST_TO_INT_CAST_THROWBOUNDS("slip_delay",obj->config.slip_delay,v,o,clock_time_t,0,65536);
//
//
//		Local<Value> v8nvm = o->Get(String::New("nvm_data"));
//		if(!v8nvm->IsUndefined() && v8nvm->IsObject())
//			JStoNvmData( *(obj->nvm_data_p), v8nvm->ToObject());
//
//	}
//
//
//
//
////	 int slip_config_flowcontrol;
////	 int slip_config_timestamp;
////	 const char *slip_config_siodev;
////	 const char *slip_config_host;
////	 const char *slip_config_port;
////	 char slip_config_tundev[32];
////	 uint16_t slip_config_basedelay;
////	 char const *default_nvm_file;
////	 uint8_t use_raw_ethernet;
////	 uint8_t ethernet_has_fcs;
////	 int baudrate;
//////	 speed_t slip_config_b_rate;
////	 char const *slip_config_ifup_script;
////	 char const *slip_config_ifdown_script;
////	 char const *slip_config_www_root;
////	 char const *slip_config_plugins;
////	 int watchdog_interval;
////	 char const * watchdog_file_name;
////	 char const * ip_config_file_name;
////	 char const *  node_config_file_name;
////
////	 int8_t Log6lbr_level;
////	 uint32_t Log6lbr_services;
//
//
//	// TODO check if running
//	SixLBR::control_event ev(DOCONFIG,obj);
//
//	if(args.Length() > 1 && args[1]->IsFunction()) {
//		ev.onSuccessCB = Persistent<Function>::New(Local<Function>::Cast(args[1]));
//	}
//	if(args.Length() > 2 && args[2]->IsFunction()) {
//		ev.onFailCB = Persistent<Function>::New(Local<Function>::Cast(args[2]));
//	}
//
//	obj->cmd_queue_6lbr_thread.add(ev); // will awake thread... if 6LBR is not running...
//
//	return scope.Close(Undefined());
//}
//
//
//Handle<Value> SixLBR::GetConfig(const Arguments& args) {
//	HandleScope scope;
//
//	SixLBR* obj = ObjectWrap::Unwrap<SixLBR>(args.This());
//	obj->setupThread();
//
////	obj->config.
//
////	if(args.Length() > 0 && args[0]->IsObject()) {
////		Local<Array> keys = base->GetPropertyNames();
////		for(int n=0;n<keys->Length();n++) {
////			Local<String> keyname = keys->Get(n)->ToString();
////			tpl->InstanceTemplate()->Set(keyname, base->Get(keyname));
////		}
//		Local<Object> o = Object::New();
//
//
////		INT32_TO_V8(v8field,cfield,v8obj) v8obj->Set(String::New(v8field),Int32::New(cfield))
////		#define UINT32_TO_V8(v8field,cfield,v8obj) v8obj->Set(String::New(v8field),Integer::NewFromUnsigned(cfield))
////		#define MEMBLK_TO_NODE_BUFFER(v8field,cfield,strct,size,v8obj)
//
//		INT32_TO_V8("baudrate",obj->config.baudrate,o);
////		V8_IFEXIST_TO_INT_CAST("baudrate",obj->config.baudrate,v,o,int);
//
//		INT32_TO_V8("log_level",obj->config.Log6lbr_level,o);
//		UINT32_TO_V8("log_services",obj->config.Log6lbr_services,o);
//
//
//		CSTR_TO_V8STR("host",obj->config.slip_config_host,o);
//
//		CSTR_TO_V8STR("port",obj->config.slip_config_port,o);
//		CSTR_TO_V8STR("siodev",obj->config.slip_config_siodev,o);
//		CSTR_TO_V8STR("tundev",obj->config.slip_config_tundev,o);
//
//		INT32_TO_V8("slip_basedelay",obj->config.slip_config_basedelay,o);
//
//		BOOLEAN_TO_V8("use_raw_ethernet",obj->config.use_raw_ethernet);
//		BOOLEAN_TO_V8("ethernet_has_fcs",obj->config.ethernet_has_fcs);
//		BOOLEAN_TO_V8("flowcontrol",obj->config.slip_config_flowcontrol);
//
////		if(obj->config.ethernet_has_fcs) o->Set(String::New("ethernet_has_fcs"),Boolean::New(true)); else o->Set(String::New("ethernet_has_fcs"),Boolean::New(false));
////		if(obj->config.flowcontrol) o->Set(String::New("flowcontrol"),Boolean::New(true)); else o->Set(String::New("flowcontrol"),Boolean::New(false));
//
//
//		CSTR_TO_V8STR("ifup_script",obj->config.slip_config_ifup_script,o);
//		CSTR_TO_V8STR("ifdown_script",obj->config.slip_config_ifdown_script,o);
//		CSTR_TO_V8STR("www_root",obj->config.slip_config_www_root,o);
//		CSTR_TO_V8STR("plugins",obj->config.slip_config_plugins,o);
//		CSTR_TO_V8STR("watchdog_file_name",obj->config.watchdog_file_name,o);
//		CSTR_TO_V8STR("ip_config_file_name",obj->config.ip_config_file_name,o);
//		CSTR_TO_V8STR("node_config_file_name",obj->config.node_config_file_name,o);
//
//		INT_TO_V8("watchdog_interval",obj->config.watchdog_interval,o);
//		INT32_TO_V8("slip_basedelay",obj->config.slip_config_basedelay,o);
//
//		INT32_TO_V8("slip_delay",obj->config.slip_delay,o);
//
//
//		Local<Object> v8nvm = Object::New();
//		nvmDataToJS( *(obj->nvm_data_p), v8nvm);
//		o->Set(String::New("nvm_data"),v8nvm);
//
//		Local<Value> flags;
//		JSstringify_nvm_flags( *(obj->nvm_data_p), flags);
//
//		o->Set(String::New("FLAGS_read_only"),flags);
//
////	}
//
//
//
//
////	 int slip_config_flowcontrol;
////	 int slip_config_timestamp;
////	 const char *slip_config_siodev;
////	 const char *slip_config_host;
////	 const char *slip_config_port;
////	 char slip_config_tundev[32];
////	 uint16_t slip_config_basedelay;
////	 char const *default_nvm_file;
////	 uint8_t use_raw_ethernet;
////	 uint8_t ethernet_has_fcs;
////	 int baudrate;
//////	 speed_t slip_config_b_rate;
////	 char const *slip_config_ifup_script;
////	 char const *slip_config_ifdown_script;
////	 char const *slip_config_www_root;
////	 char const *slip_config_plugins;
////	 int watchdog_interval;
////	 char const * watchdog_file_name;
////	 char const * ip_config_file_name;
////	 char const *  node_config_file_name;
////
////	 int8_t Log6lbr_level;
////	 uint32_t Log6lbr_services;
//
//	return scope.Close(o);
//}
//
//// thread local storage global pointing to the v8 side C++ wrapped object for this current thread. Initialized when the thread starts. see: void SixLBR::contiki_main_thread(void *d)
//_6LBR_GLOBAL SixLBR *my6LBR = NULL;
//
//_6LBR_tun_info *get_tun_info() {
//	_6LBR_tun_info *ret = NULL;
//	uv_mutex_lock(&my6LBR->_control);
//	ret = &my6LBR->tunInfo;
//	uv_mutex_unlock(&my6LBR->_control);
//	return ret;
//}
//
//bool SixLBR::submitToV8ControlCommand(SixLBR::control_event &ev) {
//	bool ret = this->v8_cmd_queue.addMvIfRoom(ev);
////	if(ret) {
//		this->_async_control.data = my6LBR;
//		uv_async_send(&my6LBR->_async_control); // let v8 know - calls callback...
////	}
//	return ret;
//}
//
//void register6LBR_add_route_change(uip_ds6_route_t *route, uip_ipaddr_t *nexthop) {
//	SixLBR::sixlbr_uip_ds6_route *n6lbr_route = NULL;    // the node.js 'version' of the route info for a neighbor
//	uip_ds6_route_neighbor_routes *walk = NULL;
//
//	// For more info, reference: uip-ds6-route.c
//
//	if(!my6LBR->rplTable.find(route->ipaddr,n6lbr_route)) {
//		n6lbr_route = new SixLBR::sixlbr_uip_ds6_route(&route->ipaddr);
//		gettimeofday(&n6lbr_route->timestamp, NULL);
//        localtime_r(&n6lbr_route->timestamp.tv_sec, &n6lbr_route->date);
//        /* Yash: Nexthop, if the nexthop is the same then it is directly connected */
//        //TODO: nexthop should be null if the node is directly connected
//        // if(nexthop != NULL) {
//        	memcpy(&n6lbr_route->nexthop, nexthop, sizeof(uip_ipaddr_t));
//        // } else {
//        // 	n6lbr_route->nexthop = NULL;
//        // }
//
//		my6LBR->rplTable.addNoreplace(route->ipaddr,n6lbr_route);
//		// LOG6LBR_INFO("***** Adding new route at: ");
//  // 		printf("%d-%02d-%02d %d:%02d:%02d.%"PRIu32": ",
//  // 				n6lbr_route->date.tm_year+1900, n6lbr_route->date.tm_mon, n6lbr_route->date.tm_mday,
//  // 				n6lbr_route->date.tm_hour, n6lbr_route->date.tm_min, n6lbr_route->date.tm_sec,
//  // 				n6lbr_route->timestamp.tv_usec);
//		LOG6LBR_6ADDR(INFO, &route->ipaddr, "Route added, IP address: ");
//		LOG6LBR_6ADDR(INFO, nexthop, "via: ");
//	} else {
//		/* YASH: 6lbr deletes the routes before it adds the found route */
//		/* It should never enter here */
//		LOG6LBR_6ADDR(INFO, &route->ipaddr, "Found the old route");
//	}
//
//	SixLBR::control_event out_ev(SixLBR::ROUTING_UPDATE,my6LBR);
//
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping register6LBR_add_route_change\n");
//	}
//}
//
//void register6LBR_del_route_change(uip_ds6_route_t *route) {
//	SixLBR::sixlbr_uip_ds6_route *n6lbr_route = NULL;
//	uip_ds6_route_neighbor_routes *walk = NULL;
//
//	// For more info, reference: uip-ds6-route.c
//
//	if(my6LBR->rplTable.remove(route->ipaddr,n6lbr_route)) {
//		LOG6LBR_6ADDR(INFO, &route->ipaddr, "Removed IP address: ");
//	}
//
//	SixLBR::control_event out_ev(SixLBR::ROUTING_UPDATE,my6LBR);
//
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping register6LBR_del_route_change\n");
//	}
//}
//
//int lookup_symmetric_device_key_6LBR(uint8_t *mac, uint8_t **key) {
//	int ret = 0;
//	MAC_ADDR_6LBR macaddr(mac,false);  // temp-only cstor
//	AES_KEY_6LBR *aeskey = NULL;
//
//	if((aeskey = my6LBR->macKeyTable.find(macaddr)) != NULL) {
//		*key=aeskey->val;
//		ret = 1;
//	}
//
//	return ret;
//}
//
//void get_symmetric_network_key_6LBR(uint8_t **key) {
//	uv_mutex_lock(&my6LBR->_mutex_networkKey);
//	*key = my6LBR->networkKey.val;
//	uv_mutex_unlock(&my6LBR->_mutex_networkKey);
//}
//
//
//int submitIn_6LBR_rawLLPacket(char *d, int l) {
//	assert(my6LBR != NULL);
//	SixLBR::control_event *out_ev;
//	int dummy;
//	uint16_t magic = GET_MAGIC_NUMBER_CHARP(d);
//	if(my6LBR->magicNumTable.find(magic,dummy)) {
//		  LOG6LBR_TRACE("found corresponding magic num: %d\n", magic);
//		SixLBR::control_event out_ev(SixLBR::INBOUND_RAW_FRAME,my6LBR);
//		out_ev.managed_aux = new SixLBR::macFrame(magic,d,l);
//		if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//			LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping submitIn_6LBR_rawLLPacket\n");
//		}
//		return 1;
//	} else
//		return 0;
//}
//
//void update6LBRMagicNumTable(char *raw) {
//	assert(my6LBR != NULL);
//	  int dummy = 0; // nothing really stored in table right now
//	  int old = -1;
//	  uint16_t magic;
//	  magic = GET_MAGIC_NUMBER_CHARP(raw);
//	  if(my6LBR->magicNumTable.addReplace(magic,dummy,old) && old == -1) {
//		  LOG6LBR_TRACE("new magic number: %d\n", magic);
//	  }
//}
//
//void inform_6LBR_addSrcAddr(uip_ipaddr_t *addr) {
//	// inform v8 we are running...
//	SixLBR::control_event out_ev(SixLBR::NEWSRCADDR,my6LBR);
//	out_ev.managed_aux = malloc(sizeof(uip_ipaddr_t));
//	memcpy(out_ev.managed_aux, addr, sizeof(uip_ipaddr_t));
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping inform_6LBR_addSrcAddr\n");
//	}
//};
//
//
//void inform_6LBR_ifUp(const char *name) {
//	if(!my6LBR->ifDevName) {
//		my6LBR->ifDevName = strdup(name);
//	}
//	// inform v8 we are running...
//	SixLBR::control_event out_ev(SixLBR::IFUP,my6LBR);
//	out_ev.managed_aux = strdup(name);
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping inform_6LBR_ifUp\n");
//	}
//};
//
//// makes thread wait for a ifUp/ifDown to happen
//void wait_for_ifChange() {
//	uv_mutex_lock(&my6LBR->_control);
//	uv_cond_wait(&my6LBR->_mac_set_cond, &my6LBR->_control); // wait for thread's start...
//	uv_mutex_unlock(&my6LBR->_control);
//}
//
//void inform_6LBR_ifDown(const char *name) {
//	// inform v8 we are running...
//	SixLBR::control_event out_ev(SixLBR::IFDOWN,my6LBR);
//	out_ev.managed_aux = strdup(name);
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping inform_6LBR_ifDown\n");
//	}
//};
//
//// to be called from 6lbr thread...
//// returns the time elapsed since the last time the function was called.
//uint64_t increment_alive_timer() {
//	uint64_t ret = 0;
//	uv_mutex_lock(&my6LBR->_mutex_keepalive);
//	my6LBR->_last_keepalive[SIXLBR_LAST_TIMER_INDEX] = my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX];
//	my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX] = uv_hrtime();
//	ret = my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX] - my6LBR->_last_keepalive[SIXLBR_LAST_TIMER_INDEX];
//	uv_mutex_unlock(&my6LBR->_mutex_keepalive);
//	return ret;
//}
//
//uint64_t get_oldest_alive_timer() {
//	uint64_t ret = 0;
//	uv_mutex_lock(&my6LBR->_mutex_keepalive);
//	ret = my6LBR->_last_keepalive[SIXLBR_LAST_TIMER_INDEX];
//	uv_mutex_unlock(&my6LBR->_mutex_keepalive);
//	return ret;
//}
//
//uint64_t get_last_alive_interval() {
//	uint64_t ret = 0;
//	uv_mutex_lock(&my6LBR->_mutex_keepalive);
//	ret = my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX] - my6LBR->_last_keepalive[SIXLBR_LAST_TIMER_INDEX];
//	uv_mutex_unlock(&my6LBR->_mutex_keepalive);
//	return ret;
//}
//
//void reset_alive_timer() {
//	uv_mutex_lock(&my6LBR->_mutex_keepalive);
//	my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX] = uv_hrtime();
//	my6LBR->_last_keepalive[SIXLBR_LAST_TIMER_INDEX] = my6LBR->_last_keepalive[SIXLBR_CURRENT_TIMER_INDEX];
//	uv_mutex_unlock(&my6LBR->_mutex_keepalive);
//}
//
//void channel_study_callback(char *data, int len) {
//	assert(my6LBR != NULL);
//	LOG6LBR_TRACE("Got the channel study data\n");
//	SixLBR::control_event out_ev(SixLBR::CHANNEL_STUDY,my6LBR);
//	out_ev.managed_aux = new SixLBR::channelStudyData(data, len);
//	if(!my6LBR->submitToV8ControlCommand(out_ev)) {
//		LOG6LBR_ERROR(" Inbound v8 Queue is full!!! Dropping channel_study_callback_data\n");
//	}
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////



void InitAll(Handle<Object> exports, Handle<Object> module) {
//	NodeTransactionWrapper::Init();
//	NodeClientWrapper::Init();
//	exports->Set(String::NewSymbol("cloneRepo"), FunctionTemplate::New(CreateClient)->GetFunction());


//	TunInterface::Init();
//	exports->Set(String::NewSymbol("init"), FunctionTemplate::New(SixLBR::Init)->GetFunction());
	NodeDhclient::Init();

//	exports->Set(String::NewSymbol("IscDhclient"), FunctionTemplate::New(NodeDhclient::New)->GetFunction());
	exports->Set(String::NewSymbol("newClient"), FunctionTemplate::New(NodeDhclient::NewClient)->GetFunction());
//	exports->Set(String::NewSymbol("readPseudo"), FunctionTemplate::New(PseudoFs::ReadPseudofile)->GetFunction());
//	exports->Set(String::NewSymbol("writePseudo"), FunctionTemplate::New(PseudoFs::WritePseudofile)->GetFunction());

	Handle<Object> consts = Object::New();
	_errcmn::DefineConstants(consts);
	exports->Set(String::NewSymbol("CONSTS"), consts);


//	exports->Set(String::NewSymbol("read"), FunctionTemplate::New(Read)->GetFunction());
//	exports->Set(String::NewSymbol("write"), FunctionTemplate::New(Write)->GetFunction());
//	exports->Set(String::NewSymbol("close"), FunctionTemplate::New(Close)->GetFunction());

//	exports->Set(String::NewSymbol("assignRoute"), FunctionTemplate::New(AssignRoute)->GetFunction());
//	exports->Set(String::NewSymbol("setIfFlags"), FunctionTemplate::New(SetIfFlags)->GetFunction());
//	exports->Set(String::NewSymbol("unsetIfFlags"), FunctionTemplate::New(UnsetIfFlags)->GetFunction());

//	exports->Set(String::NewSymbol("_TunInterface_cstor"), TunInterface::constructor);

	//	exports->Set(String::NewSymbol("_TunInterface_proto"), TunInterface::prototype);

//	exports->Set(String::NewSymbol("shutdownTunInteface"), FunctionTemplate::New(ShutdownTunInterface)->GetFunction());

}

NODE_MODULE(isc_dhclient, InitAll)


