/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qerr.h"

const char *qerr_to_str(enum qerr error)
{
	switch (error) {
	case QERR_EADDRNOTAVAIL:
		return "Address not available";
	case QERR_EAFNOSUPPORT:
		return "Address family not supported";
	case QERR_EAGAIN:
		return "Resource temporarily unavailable";
	case QERR_EBADF:
		return "Bad file descriptor";
	case QERR_EBADMSG:
		return "Bad message";
	case QERR_EBUSY:
		return "Device or resource busy";
	case QERR_ECONNREFUSED:
		return "Connection refused";
	case QERR_EEXIST:
		return "File exists";
	case QERR_EFAULT:
		return "Bad address";
	case QERR_EINTR:
		return "Interrupted system call";
	case QERR_EINVAL:
		return "Invalid argument";
	case QERR_EIO:
		return "I/O error";
	case QERR_EMSGSIZE:
		return "Message too long";
	case QERR_ENETDOWN:
		return "Network is down";
	case QERR_ENOBUFS:
		return "No buffer space available";
	case QERR_ENOENT:
		return "No such region or scheduler";
	case QERR_ENOMEM:
		return "Not enough memory";
	case QERR_ENOTSUP:
		return "Operation not supported";
	case QERR_EPERM:
		return "Permission denied";
	case QERR_EPIPE:
		return "Broken pipe";
	case QERR_EPROTO:
		return "Protocol error";
	case QERR_EPROTONOSUPPORT:
		return "Protocol not supported";
	case QERR_ERANGE:
		return "Result too large";
	case QERR_ETIME:
		return "Timer expired";
	case QERR_SE_EINVAL:
		return "Invalid arguments given to SE";
	case QERR_SE_ENOKEY:
		return "No session key found for given id";
	case QERR_SE_ENOSUBKEY:
		return "No sub-session key found for given id";
	case QERR_SE_ERDSFETCHFAIL:
		return "Unexpected failure in SE while fetching keys";
	default:
		return "Unknown QERR error value";
	}
}
