/* eXosip_transport_hook.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */                
#ifndef __EXOSIP_TRANSPORT_HOOK_H__
#define __EXOSIP_TRANSPORT_HOOK_H__
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/select.h>

#endif

typedef int (*eXosip_recvfrom_cb)(int socket, void * buffer, size_t length, int flags, struct sockaddr * address, socklen_t * address_len,void* userdata);
typedef int (*eXosip_sendto_cb)(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len,void* userdata);
typedef int (*eXosip_select_cb)(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout,void* userdata);

/**
 * default implementation 
 */
int eXosip_recvfrom(int socket, void * buffer, size_t length, int flags, struct sockaddr * address, socklen_t * address_len);
int eXosip_sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
int eXosip_select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds, struct timeval * timeout);


typedef struct _eXosip_transport_hooks {
	void* data;
	eXosip_recvfrom_cb recvfrom;
	eXosip_sendto_cb sendto;
	eXosip_select_cb select;
} eXosip_transport_hooks_t;

#define recvfrom eXosip_recvfrom
#define sendto eXosip_sendto
#define select eXosip_select
/**
 * use NULL to unregister
 */
int eXosip_transport_hook_register(eXosip_transport_hooks_t* cbs);
int eXosip_get_udp_socket(void);
int eXosip_get_control_fd(void);

#endif /*__EXOSIP_TRANSPORT_HOOK_H__*/
