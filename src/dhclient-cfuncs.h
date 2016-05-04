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

// leases are returned as JSON string in our variation of dhclient.
// this is max size of that string
#define MAX_LEASE_STR_SIZE 1000

// Defines needed for isc-dhcp
//#define LOCALSTATEDIR "/NOVAR"

// end defines needed

#ifdef __cplusplus
extern "C" {
#endif


// huh? explanation..
// for some reason the ISC folks decided to use C++ keywords as symbol names in
// their C code. (shaking head - granted maybe some of this was started before 1991)
// anyway, this deals with that by defining what we need instead of pulling in their headers.

// if the dhcpd.h header has not been pulled in do this...
// see dhcpd.h:~1000

#if ! defined(SV_DEFAULT_LEASE_TIME) && defined(__cplusplus)

#include <time.h>
typedef time_t TIME;

enum policy { P_IGNORE, P_ACCEPT, P_PREFER, P_REQUIRE, P_DONT };
struct iaddr {
	unsigned len;
	unsigned char iabuf [16];
};
struct iaddrmatch {
	struct iaddr addr;
	struct iaddr mask;
};struct iaddrmatchlist {
	struct iaddrmatchlist *next;
	struct iaddrmatch match;
};
struct string_list {
	struct string_list *next;
	char string [1];
};
#endif

// analgous to client_config in dhcpd.h, we comment out the stuff we don't care about
struct node_dhclient_client_config {
	/*
	 * When a message has been received, run these statements
	 * over it.
	 */
//	struct group *on_receipt;

	/*
	 * When a message is sent, run these statements.
	 */
//	struct group *on_transmission;

//	struct option **required_options;  /* Options that MUST be present. */
//	struct option **requested_options; /* Options to request (ORO/PRL). */

	TIME timeout;			/* Start to panic if we don't get a
					   lease in this time period when
					   SELECTING. */
	TIME initial_delay;             /* Set initial delay before first
					   transmission. */
	TIME initial_interval;		/* All exponential backoff intervals
					   start here. */
	TIME retry_interval;		/* If the protocol failed to produce
					   an address before the timeout,
					   try the protocol again after this
					   many seconds. */
	TIME select_interval;		/* Wait this many seconds from the
					   first DHCPDISCOVER before
					   picking an offered lease. */
	TIME reboot_timeout;		/* When in INIT-REBOOT, wait this
					   long before giving up and going
					   to INIT. */
	TIME backoff_cutoff;		/* When doing exponential backoff,
					   never back off to an interval
					   longer than this amount. */
	u_int32_t requested_lease;	/* Requested lease time, if user
					   doesn't configure one. */
	struct string_list *media;	/* Possible network media values. */
	char *script_name;		/* Name of config script. */
	char *vendor_space_name;	/* Name of config script. */
	enum policy bootp_policy;
					/* Ignore, accept or prefer BOOTP
					   responses. */
	enum policy auth_policy;
					/* Require authentication, prefer
					   authentication, or don't try to
					   authenticate. */
//	struct string_list *medium;	/* Current network medium. */

	struct iaddrmatchlist *reject_list;	/* Servers to reject. */

	int omapi_port;			/* port on which to accept OMAPI
					   connections, or -1 for no
					   listener. */
	int do_forward_update;		/* If nonzero, and if we have the
					   information we need, update the
					   A record for the address we get. */
};


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

		// extra options we've added (most were compile time options previously)
		// or in clparse.c - as config file options
//		int do_forward_update;  // = 1   See NSPUDATE header flag
//		struct client_config;
		char *config_options;
		int config_options_len;
		char* initial_leases;
	} dhclient_config;

extern __thread dhclient_config *threadConfig;  // defined in dhclient-cfuncs.c

extern void init_defaults_config(dhclient_config *config);
extern int do_dhclient_request(char **errstr, dhclient_config *config);
extern int do_dhclient_hibernate(char **errstr, dhclient_config *config);
extern int do_dhclient_awaken(char **errstr, dhclient_config *config);
extern int do_dhclient_release(char **errstr, dhclient_config *config);
extern int do_dhclient_shutdown(char **errstr, dhclient_config *config);

extern int submit_lease_to_v8(char *json);
extern int submit_hibernate_complete_to_v8(void);
extern int submit_awaken_complete_to_v8(void);
extern int submit_release_complete_to_v8(void);

#ifdef __cplusplus
}
#endif

#define DEFAULT_RETRIES 3
#define TIMEOUT_FOR_RETRY 5


#endif /* DHCLIENT_CFUNCS_H_ */
