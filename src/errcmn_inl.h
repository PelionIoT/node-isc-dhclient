/*
 * pseudo_err_inl.h
 *
 *  Created on: Nov 18, 2014
 *      Author: ed
 * (c) 2014, WigWag Inc
 */

// inline insertion of extra constants
#include "dhclient-cfuncs-errors.h"

// codes defined in: dhclient-cfuncs-errors.h

custom_errno custom_errs[] = {
		{ "Invalid dhclient config.", DHCLIENT_INVALID_CONFIG },
		{ "General dhclient exec error.", DHCLIENT_EXEC_ERROR }
};
