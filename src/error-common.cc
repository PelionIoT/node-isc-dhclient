/*
 * error-common.cc
 *
 *  Created on: Sep 3, 2014
 *      Author: ed
 * (c) 2014, WigWag Inc.
 */


#include "error-common.h"

#include <string.h>

extern "C" {
	extern char *strdup(const char *s);
}

namespace _errcmn {

typedef struct {
	const char *label;
	const int code;
} custom_errno;


#ifdef _ERRCMN_ADD_CONSTS
#include "errcmn_inl.h"
#else
	custom_errno custom_errs[] = { };
#endif


void DefineConstants(v8::Handle<v8::Object> target) {



#ifdef E2BIG
_ERRCMN_DEFINE_CONSTANT_WREV(target, E2BIG);
#endif

#ifdef EACCES
_ERRCMN_DEFINE_CONSTANT_WREV(target, EACCES);
#endif

#ifdef EADDRINUSE
_ERRCMN_DEFINE_CONSTANT_WREV(target, EADDRINUSE);
#endif

#ifdef EADDRNOTAVAIL
_ERRCMN_DEFINE_CONSTANT_WREV(target, EADDRNOTAVAIL);
#endif

#ifdef EAFNOSUPPORT
_ERRCMN_DEFINE_CONSTANT_WREV(target, EAFNOSUPPORT);
#endif

#ifdef EAGAIN
_ERRCMN_DEFINE_CONSTANT_WREV(target, EAGAIN);
#endif

#ifdef EALREADY
_ERRCMN_DEFINE_CONSTANT_WREV(target, EALREADY);
#endif

#ifdef EBADF
_ERRCMN_DEFINE_CONSTANT_WREV(target, EBADF);
#endif

#ifdef EBADMSG
_ERRCMN_DEFINE_CONSTANT_WREV(target, EBADMSG);
#endif

#ifdef EBUSY
_ERRCMN_DEFINE_CONSTANT_WREV(target, EBUSY);
#endif

#ifdef ECANCELED
_ERRCMN_DEFINE_CONSTANT_WREV(target, ECANCELED);
#endif

#ifdef ECHILD
_ERRCMN_DEFINE_CONSTANT_WREV(target, ECHILD);
#endif

#ifdef ECONNABORTED
_ERRCMN_DEFINE_CONSTANT_WREV(target, ECONNABORTED);
#endif

#ifdef ECONNREFUSED
_ERRCMN_DEFINE_CONSTANT_WREV(target, ECONNREFUSED);
#endif

#ifdef ECONNRESET
_ERRCMN_DEFINE_CONSTANT_WREV(target, ECONNRESET);
#endif

#ifdef EDEADLK
_ERRCMN_DEFINE_CONSTANT_WREV(target, EDEADLK);
#endif

#ifdef EDESTADDRREQ
_ERRCMN_DEFINE_CONSTANT_WREV(target, EDESTADDRREQ);
#endif

#ifdef EDOM
_ERRCMN_DEFINE_CONSTANT_WREV(target, EDOM);
#endif

#ifdef EDQUOT
_ERRCMN_DEFINE_CONSTANT_WREV(target, EDQUOT);
#endif

#ifdef EEXIST
_ERRCMN_DEFINE_CONSTANT_WREV(target, EEXIST);
#endif

#ifdef EFAULT
_ERRCMN_DEFINE_CONSTANT_WREV(target, EFAULT);
#endif

#ifdef EFBIG
_ERRCMN_DEFINE_CONSTANT_WREV(target, EFBIG);
#endif

#ifdef EHOSTUNREACH
_ERRCMN_DEFINE_CONSTANT_WREV(target, EHOSTUNREACH);
#endif

#ifdef EIDRM
_ERRCMN_DEFINE_CONSTANT_WREV(target, EIDRM);
#endif

#ifdef EILSEQ
_ERRCMN_DEFINE_CONSTANT_WREV(target, EILSEQ);
#endif

#ifdef EINPROGRESS
_ERRCMN_DEFINE_CONSTANT_WREV(target, EINPROGRESS);
#endif

#ifdef EINTR
_ERRCMN_DEFINE_CONSTANT_WREV(target, EINTR);
#endif

#ifdef EINVAL
_ERRCMN_DEFINE_CONSTANT_WREV(target, EINVAL);
#endif

#ifdef EIO
_ERRCMN_DEFINE_CONSTANT_WREV(target, EIO);
#endif

#ifdef EISCONN
_ERRCMN_DEFINE_CONSTANT_WREV(target, EISCONN);
#endif

#ifdef EISDIR
_ERRCMN_DEFINE_CONSTANT_WREV(target, EISDIR);
#endif

#ifdef ELOOP
_ERRCMN_DEFINE_CONSTANT_WREV(target, ELOOP);
#endif

#ifdef EMFILE
_ERRCMN_DEFINE_CONSTANT_WREV(target, EMFILE);
#endif

#ifdef EMLINK
_ERRCMN_DEFINE_CONSTANT_WREV(target, EMLINK);
#endif

#ifdef EMSGSIZE
_ERRCMN_DEFINE_CONSTANT_WREV(target, EMSGSIZE);
#endif

#ifdef EMULTIHOP
_ERRCMN_DEFINE_CONSTANT_WREV(target, EMULTIHOP);
#endif

#ifdef ENAMETOOLONG
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENAMETOOLONG);
#endif

#ifdef ENETDOWN
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENETDOWN);
#endif

#ifdef ENETRESET
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENETRESET);
#endif

#ifdef ENETUNREACH
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENETUNREACH);
#endif

#ifdef ENFILE
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENFILE);
#endif

#ifdef ENOBUFS
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOBUFS);
#endif

#ifdef ENODATA
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENODATA);
#endif

#ifdef ENODEV
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENODEV);
#endif

#ifdef ENOENT
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOENT);
#endif

#ifdef ENOEXEC
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOEXEC);
#endif

#ifdef ENOLCK
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOLCK);
#endif

#ifdef ENOLINK
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOLINK);
#endif

#ifdef ENOMEM
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOMEM);
#endif

#ifdef ENOMSG
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOMSG);
#endif

#ifdef ENOPROTOOPT
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOPROTOOPT);
#endif

#ifdef ENOSPC
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOSPC);
#endif

#ifdef ENOSR
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOSR);
#endif

#ifdef ENOSTR
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOSTR);
#endif

#ifdef ENOSYS
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOSYS);
#endif

#ifdef ENOTCONN
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTCONN);
#endif

#ifdef ENOTDIR
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTDIR);
#endif

#ifdef ENOTEMPTY
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTEMPTY);
#endif

#ifdef ENOTSOCK
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTSOCK);
#endif

#ifdef ENOTSUP
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTSUP);
#endif

#ifdef ENOTTY
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENOTTY);
#endif

#ifdef ENXIO
_ERRCMN_DEFINE_CONSTANT_WREV(target, ENXIO);
#endif

#ifdef EOPNOTSUPP
_ERRCMN_DEFINE_CONSTANT_WREV(target, EOPNOTSUPP);
#endif

#ifdef EOVERFLOW
_ERRCMN_DEFINE_CONSTANT_WREV(target, EOVERFLOW);
#endif

#ifdef EPERM
_ERRCMN_DEFINE_CONSTANT_WREV(target, EPERM);
#endif

#ifdef EPIPE
_ERRCMN_DEFINE_CONSTANT_WREV(target, EPIPE);
#endif

#ifdef EPROTO
_ERRCMN_DEFINE_CONSTANT_WREV(target, EPROTO);
#endif

#ifdef EPROTONOSUPPORT
_ERRCMN_DEFINE_CONSTANT_WREV(target, EPROTONOSUPPORT);
#endif

#ifdef EPROTOTYPE
_ERRCMN_DEFINE_CONSTANT_WREV(target, EPROTOTYPE);
#endif

#ifdef ERANGE
_ERRCMN_DEFINE_CONSTANT_WREV(target, ERANGE);
#endif

#ifdef EROFS
_ERRCMN_DEFINE_CONSTANT_WREV(target, EROFS);
#endif

#ifdef ESPIPE
_ERRCMN_DEFINE_CONSTANT_WREV(target, ESPIPE);
#endif

#ifdef ESRCH
_ERRCMN_DEFINE_CONSTANT_WREV(target, ESRCH);
#endif

#ifdef ESTALE
_ERRCMN_DEFINE_CONSTANT_WREV(target, ESTALE);
#endif

#ifdef ETIME
_ERRCMN_DEFINE_CONSTANT_WREV(target, ETIME);
#endif

#ifdef ETIMEDOUT
_ERRCMN_DEFINE_CONSTANT_WREV(target, ETIMEDOUT);
#endif

#ifdef ETXTBSY
_ERRCMN_DEFINE_CONSTANT_WREV(target, ETXTBSY);
#endif

#ifdef EWOULDBLOCK
_ERRCMN_DEFINE_CONSTANT_WREV(target, EWOULDBLOCK);
#endif

#ifdef EXDEV
_ERRCMN_DEFINE_CONSTANT_WREV(target, EXDEV);
#endif

}




	char *get_custom_err_str(int _errno) {
		int n = sizeof(custom_errs);
		while(n > 0) {
			if(custom_errs[n-1].code == _errno) {
				return (char *) custom_errs[n-1].label;
			}
			n--;
		}
		return NULL;
	}


	const int max_error_buf = 255;

	char *get_error_str(int _errno) {
		char *ret = NULL;
		if(_errno < _ERR_CUSTOM_ERROR_CUTOFF) {
			char *buf = (char *) malloc(max_error_buf);
			char *ret = strerror_r(_errno,buf,max_error_buf);
			if ( ret == NULL ) DBG_OUT("strerror_r bad return\n");
		} else {
			char *s = get_custom_err_str(_errno);
			ret = strdup(s);
		}
		return ret;
	}


	void free_error_str(char *b) {
		free(b);
	}

	void err_ev::setError(int e,const char *m)
	{
		_errno = e;
		if(errstr) free(errstr);
		if(!m)
			errstr = get_error_str(e);
		else
			errstr = ::strdup(m);
	}

	v8::Local<v8::Value> errno_to_JS(int _errno, const char *prefix) {
		v8::Local<v8::Object> retobj = Nan::New<v8::Object>();
		bool custom = false;
		if(_errno) {
			char *temp = NULL;
			char *errstr = NULL;
			if(_errno < _ERR_CUSTOM_ERROR_CUTOFF) {
				errstr = get_error_str(_errno);
			} else {
				errstr = get_custom_err_str(_errno);
				Nan::Set(retobj, Nan::New("message").ToLocalChecked(), Nan::New(errstr).ToLocalChecked());
			}
			if(errstr) {
				if(prefix) {
					int len = strlen(prefix)+strlen(errstr)+2;
					temp = (char *) malloc(len);
					memset(temp,0,len);
					strcpy(temp, prefix);
					strcat(temp, errstr);
				} else {
					temp = errstr;
				}
				Nan::Set(retobj, Nan::New("message").ToLocalChecked(), Nan::New<v8::String>(temp).ToLocalChecked());
				if(!custom) free_error_str(errstr);
				if(prefix) {
					free(temp);
				}
			}
			Nan::Set(retobj, Nan::New("errno").ToLocalChecked(), Nan::New<v8::Integer>(_errno));
		}
		return retobj;
	}

	v8::Local<v8::Value> errno_to_JSError(int _errno, const char *prefix) {
		v8::Local<v8::Value> retobj;
		bool custom = false;
		if(_errno) {
			char *temp = NULL;
			char *errstr = NULL;
			if(_errno < _ERR_CUSTOM_ERROR_CUTOFF) {
				errstr = get_error_str(_errno);
			} else {
				errstr = get_custom_err_str(_errno);
				custom = true;
			}
			if(errstr) {
					if(prefix) {
						int len = strlen(prefix)+strlen(errstr)+2;
						temp = (char *) malloc(len);
						memset(temp,0,len);
						strcpy(temp, prefix);
						strcat(temp, errstr);
					} else {
						temp = errstr;
					}
					retobj = v8::Exception::Error(Nan::New<v8::String>(temp).ToLocalChecked());

					if(!custom) {
						free_error_str(errstr);
					}
					if(prefix) {
						free(temp);
					}

			} else
				retobj = v8::Exception::Error(Nan::New("Error").ToLocalChecked());

			Nan::Set(retobj->ToObject(), Nan::New("errno").ToLocalChecked(), Nan::New<v8::Integer>(_errno));
		}
		return retobj;
	}


	v8::Handle<v8::Value> err_ev_to_JS(err_ev &e, const char *prefix) {
		//		v8::Local<v8::Object> retobj = v8::Object::New();
			v8::Local<v8::Value> retobj = Nan::New<v8::Primitive>(Nan::Undefined());

			if(e.hasErr()) {
				char *temp = NULL;
				if(e.errstr) {
					int len = strlen(e.errstr)+1;
					if(prefix) len += strlen(prefix)+1;
					temp = (char *) malloc(len);
					memset(temp,0,len);
					if(prefix) {
						strcpy(temp, prefix);
						strcat(temp, e.errstr);
					} else
						strcpy(temp, e.errstr);
					retobj = v8::Exception::Error(Nan::New(temp).ToLocalChecked());
					free(temp);
				}
				else retobj = v8::Exception::Error(Nan::New("Error").ToLocalChecked());
				Nan::Set(retobj->ToObject(), Nan::New("errno").ToLocalChecked(), Nan::New<v8::Integer>(e._errno));
			}
			return retobj;
		}
}
