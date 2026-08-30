/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qerr.h"

#include <zephyr.h>

enum qerr qerr_convert_os_to_qerr(int error)
{
	if (error > 0)
		return error;

	switch (error) {
	case 0:
		return QERR_SUCCESS;
	case -EADDRNOTAVAIL:
		return QERR_EADDRNOTAVAIL;
	case -EAFNOSUPPORT:
		return QERR_EAFNOSUPPORT;
	case -EAGAIN:
		return QERR_EAGAIN;
	case -EBADF:
		return QERR_EBADF;
	case -EBADMSG:
		return QERR_EBADMSG;
	case -EBUSY:
		return QERR_EBUSY;
	case -ECONNREFUSED:
		return QERR_ECONNREFUSED;
	case -EEXIST:
		return QERR_EEXIST;
	case -EFAULT:
		return QERR_EFAULT;
	case -EINTR:
		return QERR_EINTR;
	case -EINVAL:
		return QERR_EINVAL;
	case -EIO:
		return QERR_EIO;
	case -EMSGSIZE:
		return QERR_EMSGSIZE;
	case -ENETDOWN:
		return QERR_ENETDOWN;
	case -ENOBUFS:
		return QERR_ENOBUFS;
	case -ENOENT:
		return QERR_ENOENT;
	case -ENOMEM:
		return QERR_ENOMEM;
	case -ENOTSUP:
		return QERR_ENOTSUP;
	case -EPERM:
		return QERR_EPERM;
	case -EPIPE:
		return QERR_EPIPE;
	case -EPROTO:
		return QERR_EPROTO;
	case -EPROTONOSUPPORT:
		return QERR_EPROTONOSUPPORT;
	case -ERANGE:
		return QERR_ERANGE;
	case -ETIME:
		return QERR_ETIME;
	case -ENODEV:
		return QERR_ENODEV;
	case -ENOSPC:
		return QERR_ENOSPC;
	default:
		return QERR_EINVAL;
	}
}

int qerr_convert_qerr_to_os(enum qerr error)
{
	if (error > 0)
		return error;

	switch (error) {
	case QERR_SUCCESS:
		return 0;
	case QERR_EADDRNOTAVAIL:
		return -EADDRNOTAVAIL;
	case QERR_EAFNOSUPPORT:
		return -EAFNOSUPPORT;
	case QERR_EAGAIN:
		return -EAGAIN;
	case QERR_EBADF:
		return -EBADF;
	case QERR_EBADMSG:
		return -EBADMSG;
	case QERR_EBUSY:
		return -EBUSY;
	case QERR_ECONNREFUSED:
		return -ECONNREFUSED;
	case QERR_EEXIST:
		return -EEXIST;
	case QERR_EFAULT:
		return -EFAULT;
	case QERR_EINTR:
		return -EINTR;
	case QERR_EINVAL:
		return -EINVAL;
	case QERR_EIO:
		return -EIO;
	case QERR_EMSGSIZE:
		return -EMSGSIZE;
	case QERR_ENETDOWN:
		return -ENETDOWN;
	case QERR_ENOBUFS:
		return -ENOBUFS;
	case QERR_ENOENT:
		return -ENOENT;
	case QERR_ENOMEM:
		return -ENOMEM;
	case QERR_ENOTSUP:
		return -ENOTSUP;
	case QERR_EPERM:
		return -EPERM;
	case QERR_EPIPE:
		return -EPIPE;
	case QERR_EPROTO:
		return -EPROTO;
	case QERR_EPROTONOSUPPORT:
		return -EPROTONOSUPPORT;
	case QERR_ERANGE:
		return -ERANGE;
	case QERR_ETIME:
		return -ETIME;
	case QERR_ENODEV:
		return -ENODEV;
	case QERR_ENOSPC:
		return -ENOSPC;
	default:
		return -EINVAL;
	}
}
