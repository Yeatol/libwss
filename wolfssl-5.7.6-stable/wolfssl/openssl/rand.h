/* rand.h
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* rand.h for openSSL */

#ifndef WOLFSSL_RAND_COMPAT_H_
#define WOLFSSL_RAND_COMPAT_H_

#include <wolfssl/openssl/ssl.h>
#include <wolfssl/wolfcrypt/random.h>

#ifndef OPENSSL_COEXIST

typedef WOLFSSL_RAND_METHOD RAND_METHOD;

#define RAND_set_rand_method     wolfSSL_RAND_set_rand_method

#endif /* !OPENSSL_COEXIST */

#endif /* WOLFSSL_RAND_COMPAT_H_ */
