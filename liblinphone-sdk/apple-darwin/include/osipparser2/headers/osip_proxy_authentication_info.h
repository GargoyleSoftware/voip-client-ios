/*
  The oSIP library implements the Session Initiation Protocol (SIP -rfc3261-)
  Copyright (C) 2001,2002,2003,2004,2005,2006,2007 Aymeric MOIZARD jack@atosc.org
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or(at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _OSIP_PROXY_AUTHENTICATION_INFO_H_
#define _OSIP_PROXY_AUTHENTICATION_INFO_H_

#include <osipparser2/headers/osip_authentication_info.h>

/**
 * @file osip_proxy_authentication_info.h
 * @brief oSIP osip_proxy_authentication_info header definition.
 */

/**
 * @defgroup oSIP_PROXY_AUTH_INFO oSIP proxy-authentication-info header definition.
 * @ingroup oSIP_HEADERS
 * @{
 */

/**
 * Structure for Proxy-Authentication-Info headers.
 * @var osip_proxy_authentication_info_t
 */
  typedef osip_authentication_info_t osip_proxy_authentication_info_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocate a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_init(header) osip_authentication_info_init(header)
/**
 * Parse a Authenication-Info element.
 * @param header The element to work on.
 * @param hvalue The string to parse.
 */
#define osip_proxy_authentication_info_parse(header, hvalue) osip_authentication_info_parse(header, hvalue)
/**
 * Get a string representation of a Authenication-Info element.
 * @param header The element to work on.
 * @param dest A pointer on the new allocated string.
 */
#define osip_proxy_authentication_info_to_str(header, dest) osip_authentication_info_to_str(header, dest)
/**
 * Free a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_free osip_authentication_info_free
/**
 * Clone a Authenication-Info element.
 * @param header The element to work on.
 * @param dest A pointer on the copy of the element.
 */
#define osip_proxy_authentication_info_clone(header, dest) osip_authentication_info_clone(header, dest)

/**
 * Get value of the nextnonce parameter from a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_get_nextnonce(header) osip_authentication_info_get_nextnonce(header)
/**
 * Add the nextnonce parameter from a Authenication-Info element.
 * @param header The element to work on.
 * @param value The value of the new parameter.
 */
#define osip_proxy_authentication_info_set_nextnonce(header, value) osip_authentication_info_set_nextnonce(header, value)
/**
 * Get value of the cnonce parameter from a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_get_cnonce(header) osip_authentication_info_get_cnonce(header)
/**
 * Add the cnonce parameter from a Authenication-Info element.
 * @param header The element to work on.
 * @param value The value of the new parameter.
 */
#define osip_proxy_authentication_info_set_cnonce(header, value) osip_authentication_info_set_cnonce(header, value)
/**
 * Get value of the qop_options parameter from a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_get_qop_options(header) osip_authentication_info_get_qop_options(header)
/**
 * Add the qop_options parameter from a Authenication-Info element.
 * @param header The element to work on.
 * @param value The value of the new parameter.
 */
#define osip_proxy_authentication_info_set_qop_options(header, value) osip_authentication_info_set_qop_options(header, value)
/**
 * Get value of the rspauth parameter from a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_get_rspauth(header) osip_authentication_info_get_rspauth(header)
/**
 * Add the rspauth parameter from a Authenication-Info element.
 * @param header The element to work on.
 * @param value The value of the new parameter.
 */
#define osip_proxy_authentication_info_set_rspauth(header, value) osip_authentication_info_set_rspauth(header, value)
/**
 * Get value of the nc parameter from a Authenication-Info element.
 * @param header The element to work on.
 */
#define osip_proxy_authentication_info_get_nonce_count(header) osip_authentication_info_get_nonce_count(header)
/**
 * Add the nc parameter from a Authenication-Info element.
 * @param header The element to work on.
 * @param value The value of the new parameter.
 */
#define osip_proxy_authentication_info_set_nonce_count(header, value) osip_authentication_info_set_nonce_count(header, value)


#ifdef __cplusplus
}
#endif

/** @} */

#endif
