/*
 * dhclient-cfuncs-errors.h
 *
 *  Created on: Mar 17, 2015
 *      Author: ed
 * (c) 2015, WigWag Inc.
 */
#ifndef DHCLIENT_CFUNCS_ERRORS_H_
#define DHCLIENT_CFUNCS_ERRORS_H_

// see errcmn_inl.h for stringification of these errors.

#define DHCLIENT_CUSTOM_ERROR_CUTOFF 2000
#define DHCLIENT_INVALID_CONFIG 2001
#define DHCLIENT_EXEC_ERROR 2002

#define DHCLIENT_OTHER_ERROR 3001

extern const char *MEM_FAILURE_STR;


#define ERROR_OUT(s,...) fprintf(stderr, "**ERROR DHCLIENT** " s, ##__VA_ARGS__ )

#ifdef DEBUG
#define DBG_OUT(s,...) fprintf(stderr, "**DEBUG DHCLIENT** " s, ##__VA_ARGS__ )
#else
#define DBG_OUT(s,...) {}
#endif

#define DHCLIENT_MAX_ERR_STR 1000

#ifdef __cplusplus
extern "C" {
#endif


extern __thread char parse_err_str[DHCLIENT_MAX_ERR_STR];
extern char *get_parser_error_str();

#ifdef __cplusplus
}
#endif

#define SET_DHCLIENT_ERROR_STR(s,...) { char *_err = (char *) malloc(DHCLIENT_MAX_ERR_STR); \
		if(_err) { \
	        snprintf(_err,DHCLIENT_MAX_ERR_STR, s, ##__VA_ARGS__ ); \
            *err = _err; \
	    } else { fprintf(stderr, MEM_FAILURE_STR); } }


#define SET_ERROR_P_STR(cstr,s,...) { char *_err = (char *) malloc(DHCLIENT_MAX_ERR_STR); \
		if(_err) { \
	        snprintf(_err,DHCLIENT_MAX_ERR_STR, s, ##__VA_ARGS__ ); \
            *cstr = _err; \
	    } else { fprintf(stderr, MEM_FAILURE_STR); } }

#define INIT_ERROR_STR(cstr) { memset(cstr,0,sizeof(cstr)); }

#define APPEND_ERROR_STR(cstr,s,...) { \
		    int _len = strlen(cstr); \
            snprintf(((char *)cstr)+_len,DHCLIENT_MAX_ERR_STR-_len, s, ##__VA_ARGS__ ); }

#define INIT_ERROR_P_STR(cstr,LEN) { memset(cstr,0,LEN); }

#define APPEND_ERROR_P_STR(LEN,cstr,s,...) do { \
		    int _len = strlen(cstr); \
            snprintf(((char *)(cstr))+_len,LEN-_len, s, ##__VA_ARGS__ ); } while(0)



#endif /* DHCLIENT_CFUNCS_ERRORS_H_ */
