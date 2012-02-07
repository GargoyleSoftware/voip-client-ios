/*
  eXosip - This is the eXtended osip library.
  Copyright (C) 2002,2003,2004,2005,2006,2007  Aymeric MOIZARD  - jack@atosc.org
  
  eXosip is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  eXosip is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifdef ENABLE_MPATROL
#include <mpatrol.h>
#endif


#include "eXosip2.h"

extern eXosip_t eXosip;


/* some methods to extract transaction information from a eXosip_call_t */

int
eXosip_remove_transaction_from_call(osip_transaction_t * tr, eXosip_call_t * jc)
{
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;
	eXosip_dialog_t *jd;
	int pos = 0;

	if (jc->c_inc_tr == tr) {
		jc->c_inc_tr = NULL;	/* can be NULL */
		return OSIP_SUCCESS;
	}

	for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
		pos = 0;
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			if (inc_tr == tr) {
				osip_list_remove(jd->d_inc_trs, pos);
				return OSIP_SUCCESS;
			}
			pos++;
		}
	}

	if (jc->c_out_tr == tr) {
		jc->c_out_tr = NULL;	/* can be NULL */
		return OSIP_SUCCESS;
	}

	for (jd = jc->c_dialogs; jd != NULL; jd = jd->next) {
		pos = 0;
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			if (out_tr == tr) {
				osip_list_remove(jd->d_out_trs, pos);
				return OSIP_SUCCESS;
			}
			pos++;
		}
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
						  "eXosip: No information.\n"));
	return OSIP_NOTFOUND;
}

osip_transaction_t *eXosip_find_last_transaction(eXosip_call_t * jc,
												 eXosip_dialog_t * jd,
												 const char *method)
{
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;

	inc_tr = eXosip_find_last_inc_transaction(jc, jd, method);
	out_tr = eXosip_find_last_out_transaction(jc, jd, method);
	if (inc_tr == NULL)
		return out_tr;
	if (out_tr == NULL)
		return inc_tr;

	if (inc_tr->birth_time > out_tr->birth_time)
		return inc_tr;
	return out_tr;
}

osip_transaction_t *eXosip_find_last_inc_transaction(eXosip_call_t * jc,
													 eXosip_dialog_t * jd,
													 const char *method)
{
	osip_transaction_t *inc_tr;
	int pos;

	inc_tr = NULL;
	pos = 0;
	if (method == NULL || method[0] == '\0')
		return NULL;
	if (jd != NULL) {
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			if (0 == osip_strcasecmp(inc_tr->cseq->method, method))
				break;
			else
				inc_tr = NULL;
			pos++;
		}
	} else
		inc_tr = NULL;

	return inc_tr;
}

osip_transaction_t *eXosip_find_last_out_transaction(eXosip_call_t * jc,
													 eXosip_dialog_t * jd,
													 const char *method)
{
	osip_transaction_t *out_tr;
	int pos;

	out_tr = NULL;
	pos = 0;
	if (jd == NULL && jc == NULL)
		return NULL;
	if (method == NULL || method[0] == '\0')
		return NULL;

	if (jd != NULL) {
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			if (0 == osip_strcasecmp(out_tr->cseq->method, method))
				break;
			else
				out_tr = NULL;
			pos++;
		}
	}

	return out_tr;
}

osip_transaction_t *eXosip_find_last_invite(eXosip_call_t * jc,
											eXosip_dialog_t * jd)
{
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;

	inc_tr = eXosip_find_last_inc_invite(jc, jd);
	out_tr = eXosip_find_last_out_invite(jc, jd);
	if (inc_tr == NULL)
		return out_tr;
	if (out_tr == NULL)
		return inc_tr;

	if (inc_tr->birth_time > out_tr->birth_time)
		return inc_tr;
	return out_tr;
}

osip_transaction_t *eXosip_find_last_inc_invite(eXosip_call_t * jc,
												eXosip_dialog_t * jd)
{
	osip_transaction_t *inc_tr;
	int pos;

	inc_tr = NULL;
	pos = 0;
	if (jd != NULL) {
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			if (0 == strcmp(inc_tr->cseq->method, "INVITE"))
				break;
			else
				inc_tr = NULL;
			pos++;
		}
	} else
		inc_tr = NULL;

	if (inc_tr == NULL)
		return jc->c_inc_tr;	/* can be NULL */

	return inc_tr;
}

osip_transaction_t *eXosip_find_last_out_invite(eXosip_call_t * jc,
												eXosip_dialog_t * jd)
{
	osip_transaction_t *out_tr;
	int pos;

	out_tr = NULL;
	pos = 0;
	if (jd == NULL && jc == NULL)
		return NULL;

	if (jd != NULL) {
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			if (0 == strcmp(out_tr->cseq->method, "INVITE"))
				break;
			else
				out_tr = NULL;
			pos++;
		}
	}

	if (out_tr == NULL)
		return jc->c_out_tr;	/* can be NULL */

	return out_tr;
}

#ifndef MINISIZE

osip_transaction_t *eXosip_find_previous_invite(eXosip_call_t * jc,
												eXosip_dialog_t * jd,
												osip_transaction_t * last_invite)
{
	osip_transaction_t *inc_tr;
	osip_transaction_t *out_tr;
	int pos;

	inc_tr = NULL;
	pos = 0;
	if (jd != NULL) {
		while (!osip_list_eol(jd->d_inc_trs, pos)) {
			inc_tr = osip_list_get(jd->d_inc_trs, pos);
			if (inc_tr == last_invite) {
				/* we don't want the current one */
				inc_tr = NULL;
			} else if (0 == strcmp(inc_tr->cseq->method, "INVITE"))
				break;
			else
				inc_tr = NULL;
			pos++;
		}
	} else
		inc_tr = NULL;

	if (inc_tr == NULL)
		inc_tr = jc->c_inc_tr;	/* can be NULL */
	if (inc_tr == last_invite) {
		/* we don't want the current one */
		inc_tr = NULL;
	}

	out_tr = NULL;
	pos = 0;

	if (jd != NULL) {
		while (!osip_list_eol(jd->d_out_trs, pos)) {
			out_tr = osip_list_get(jd->d_out_trs, pos);
			if (out_tr == last_invite) {
				/* we don't want the current one */
				out_tr = NULL;
			} else if (0 == strcmp(out_tr->cseq->method, "INVITE"))
				break;
			else
				out_tr = NULL;
			pos++;
		}
	}

	if (out_tr == NULL)
		out_tr = jc->c_out_tr;	/* can be NULL */

	if (out_tr == last_invite) {
		/* we don't want the current one */
		out_tr = NULL;
	}

	if (inc_tr == NULL)
		return out_tr;
	if (out_tr == NULL)
		return inc_tr;

	if (inc_tr->birth_time > out_tr->birth_time)
		return inc_tr;
	return out_tr;
}

#endif
