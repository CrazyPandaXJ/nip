/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* Simple error reporting utilities
 * Author: Janne Toivola
 * Version: $Id: niperrorhandler.h,v 1.4 2011-01-23 23:01:47 jatoivol Exp $
 * */

#ifndef __NIPERRORHANDLER_H__
#define __NIPERRORHANDLER_H__

#ifndef __STDC_VERSION__
#define __STDC_VERSION__ 199901L
#endif
#ifndef _POSIX_VERSION
#define _POSIX_VERSION 200112L
#endif

#include <errno.h>

// TODO: get rid of these
#define NIP_NO_ERROR 0
#define NIP_ERROR_NULLPOINTER EFAULT
#define NIP_ERROR_DIVBYZERO EDOM
#define NIP_ERROR_INVALID_ARGUMENT EINVAL
#define NIP_ERROR_OUTOFMEMORY ENOMEM
#define NIP_ERROR_IO EIO
#define NIP_ERROR_GENERAL 6 // TODO: return the root cause!
#define NIP_ERROR_FILENOTFOUND ENOENT
#define NIP_ERROR_BAD_LUCK 8 // FIXME: srsly?


/**
 * Method for reporting an error. 
 * @param srcFile is the source file (__FILE__)
 * @param line is the number of the line in the source code (__LINE__)
 * @param error is for example ENOMEM
 * @param if verbose is other than 0, a message will be printed
 * @return the same error code to be passed on
 */
int nip_report_error(char *srcFile, int line, int error, int verbose);

/**
 * Method for resetting the error counter. */
void nip_reset_error_handler();

/**
 * Method for checking what was the last error */
int nip_check_error_type();

/**
 * Method for checking how many errors have occured */
int nip_check_error_counter();

#endif
