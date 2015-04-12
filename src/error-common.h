/*
 * error-common.h
 *
 *  Created on: Oct 31, 2014
 *      Author: ed
 * (c) 2015, Framez Inc
 */
#ifndef ERROR_COMMON_H_
#define ERROR_COMMON_H_


#include <string.h>
#include <stdlib.h>

#include <v8.h>


#include <errno.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dhclient-cfuncs-errors.h"

// https://gcc.gnu.org/onlinedocs/cpp/Stringification.html
#define xstr(s) str(s)
#define str(s) #s

// concept from node.js src/node_constants.cc
#define _ERRCMN_DEFINE_CONSTANT(target, constant)                         \
  (target)->Set(v8::String::NewSymbol(#constant),                         \
                v8::Number::New(constant),                                \
                static_cast<v8::PropertyAttribute>(                       \
                    v8::ReadOnly|v8::DontDelete))

// our mode - this is the same thing, with a reverse lookup key also
#define _ERRCMN_DEFINE_CONSTANT_WREV(target, constant)                    \
  (target)->Set(v8::String::NewSymbol(#constant),                         \
                v8::Number::New(constant),                                \
                static_cast<v8::PropertyAttribute>(                       \
                    v8::ReadOnly|v8::DontDelete));                        \
  (target)->Set(v8::String::New( xstr(constant) ),                                \
                v8::String::New(#constant),                         \
                static_cast<v8::PropertyAttribute>(                       \
                    v8::ReadOnly|v8::DontDelete));

// custom error codes should be above this value
#define _ERR_CUSTOM_ERROR_CUTOFF  DHCLIENT_CUSTOM_ERROR_CUTOFF

namespace _errcmn {

	void DefineConstants(v8::Handle<v8::Object> target);



	/** NOTE: you should always pass in a string with this when using setError() */
	const int OTHER_ERROR = DHCLIENT_OTHER_ERROR;
	char *get_error_str(int _errno);
	void free_error_str(char *b);
	v8::Local<v8::Value> errno_to_JS(int _errno, const char *prefix = NULL);
	v8::Local<v8::Value> errno_to_JSError(int _errno, const char *prefix = NULL);
	struct err_ev {
		char *errstr;
		int _errno;
		err_ev(void) : errstr(NULL), _errno(0) {};
		void setError(int e,const char *m=NULL);
		err_ev(int e) : err_ev() {
			setError(e);
		}
		err_ev(const err_ev &o) = delete;
		inline err_ev &operator=(const err_ev &o) = delete;
		inline err_ev &operator=(err_ev &&o) {
			this->errstr = o.errstr;  // transfer string to other guy...
			this->_errno = o._errno;
			o.errstr = NULL; o._errno = 0;
			return *this;
		}
		inline void clear() {
			if(errstr) ::free(errstr); errstr = NULL;
			_errno = 0;
		}
		~err_ev() {
			if(errstr) ::free(errstr);
		}
		bool hasErr() { return (_errno != 0); }
	};
	v8::Handle<v8::Value> err_ev_to_JS(err_ev &e, const char *prefix = NULL);
}

// BUILDTYPE is a node-gyp-dev thing
#ifdef _ERRCMN_MODULE_STR_
#undef _ERRCMN_MODULE_STR_
#endif
#define _ERRCMN_MODULE_STR_ "DHCLIENT"

#ifdef ERRCMN_DEBUG_BUILD
#pragma message "Build is Debug"
// confused? here: https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
#define ERROR_OUT(s,...) fprintf(stderr, "**ERROR " _ERRCMN_MODULE_STR_ "** " s, ##__VA_ARGS__ )
//#define ERROR_PERROR(s,...) fprintf(stderr, "*****ERROR***** " s, ##__VA_ARGS__ );
#define ERROR_PERROR(s,E,...) { char *__S=_err_common::get_error_str(E); fprintf(stderr, "**ERROR*" _ERRCMN_MODULE_STR_ "* [ %s ] " s, __S, ##__VA_ARGS__ ); _err_common::free_error_str(__S); }

#define DBG_OUT(s,...) fprintf(stderr, "**DEBUG " _ERRCMN_MODULE_STR_ "** " s, ##__VA_ARGS__ )
#define IF_DBG( x ) { x }
#else
#define ERROR_OUT(s,...) fprintf(stderr, "**ERROR " _ERRCMN_MODULE_STR_ "** " s, ##__VA_ARGS__ )//#define ERROR_PERROR(s,...) fprintf(stderr, "*****ERROR***** " s, ##__VA_ARGS__ );
#define ERROR_PERROR(s,E,...) { char *__S=_err_common::get_error_str(E); fprintf(stderr, "**ERROR " _ERRCMN_MODULE_STR_ "** [ %s ] " s, __S, ##__VA_ARGS__ ); _err_common::free_error_str(__S); }
#define DBG_OUT(s,...) {}
#define IF_DBG( x ) {}
#endif


#endif /* ERROR_COMMON_H_ */
