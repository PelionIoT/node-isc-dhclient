/*
 * dhclient-cfuncs.h
 *
 *  Created on: Mar 16, 2015
 *      Author: ed
 * (c) 2015, WigWag Inc.
 */
#ifndef DHCLIENT_CFUNCS_H_
#define DHCLIENT_CFUNCS_H_

// exported functions from dhclient-cfuncs.c

#include "dhclient-cfuncs-errors.h"

#define DHCLIENT_MAX_INTERFACES 10

// Defines needed for isc-dhcp
//#define LOCALSTATEDIR "/NOVAR"

// end defines needed

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct dhclient_config_t {
		int local_family;  // = AF_INET  or  AF_INET6
		int local_port;    // = 0
		char *server;      // = NULL   the DHCP server address to use (instead of broadcast)
		char *interfaces[DHCLIENT_MAX_INTERFACES];
		// IPv6 stuff:
		int wanted_ia_na;  // = -1   see dhclient option: -T and -P  and -N
		int wanted_ia_ta;  // = 0    see dhclient option: -T
		int wanted_ia_pd;  // = 0    see dhclient option: -P
		int stateless;     // = 0    see dhclient option: -S
		int duid_type;     // = 0    see dhclient option: -D
		int quiet;         // = 0    if 1 be chatty on stdout, only shoudl be used for debugging

		int exit_mode;     // = 0 these are for dhclient process related stuff - probably can remove them
		int release_mode;  // = 0
	} dhclient_config;


	void init_defaults_config(dhclient_config *config);
	int do_dhclient_request(char **errstr, dhclient_config *config);

#ifdef __cplusplus
}
#endif

#define DEFAULT_RETRIES 3
#define TIMEOUT_FOR_RETRY 5


#endif /* DHCLIENT_CFUNCS_H_ */
