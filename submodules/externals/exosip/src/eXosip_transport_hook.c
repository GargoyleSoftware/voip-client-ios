/* eXosip_transport_hook.c
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


#include "eXosip2/eXosip_transport_hook.h"
#include "eXosip2.h"

/*
remember to build eXosip with:
make DEFS="-DHAVE_CONFIG_H -include eXosip2/eXosip_transport_hook.h"
*/
#ifdef sendto
	#undef sendto
#endif
#ifdef recvfrom
	#undef recvfrom
#endif
#ifdef select
	#undef select
#endif



static eXosip_transport_hooks_t* eXosip_transport_hook_cbs=0;

extern eXosip_t eXosip;

int eXosip_get_control_fd(void){
	return jpipe_get_read_descr(eXosip.j_socketctl);
}

int eXosip_sendto(int fd,const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
	if (eXosip_transport_hook_cbs) {
		return eXosip_transport_hook_cbs->sendto(fd, buf, len, flags, to, tolen,eXosip_transport_hook_cbs->data);
	} else {
		return sendto(fd, buf, len, flags, to, tolen);
	}

}

int eXosip_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen){
	if (eXosip_transport_hook_cbs) {
		return eXosip_transport_hook_cbs->recvfrom(fd,buf,len,flags,from,fromlen,eXosip_transport_hook_cbs->data);
	} else {
		return recvfrom(fd,buf,len,flags,from,fromlen);
	}

}

int eXosip_select(int nfds, fd_set *s1, fd_set *s2, fd_set *s3, struct timeval *tv){
	if (eXosip_transport_hook_cbs) {
		return eXosip_transport_hook_cbs->select(nfds,s1,s2,s3,tv,eXosip_transport_hook_cbs->data);
	} else {
		return select(nfds,s1,s2,s3,tv);
	}

}

int eXosip_transport_hook_register(eXosip_transport_hooks_t* cbs) {
	eXosip_transport_hook_cbs=cbs;	
}
