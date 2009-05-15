/*****************************************************************************
 *
 * $Id: backalias.c,v 1.5 2004/02/21 17:22:01 mbroek Exp $
 * Purpose ...............: Alias functions.
 *
 *****************************************************************************
 * Copyright (C) 1997-2004
 *   
 * Michiel Broek                FIDO:           2:280/2802
 * Beekmansbos 10
 * 1971 BV IJmuiden
 * the Netherlands
 *
 * This file is part of MBSE BBS.
 *
 * This BBS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * MBSE BBS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MBSE BBS; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *****************************************************************************/

#include "../config.h"
#include "../lib/mbselib.h"
#include "../lib/users.h"
#include "../lib/mbsedb.h"
#include "backalias.h"



static struct aliaslist {
	struct aliaslist	*next;
	faddr			*addr;
	char			*alias;
} *alist = NULL;



char *backalias(faddr *fa)
{
	struct aliaslist	*tmp;

	for (tmp = alist; tmp; tmp = tmp->next)
		if ((!fa->domain || !tmp->addr->domain || !strcasecmp(fa->domain,tmp->addr->domain)) &&
		    (!fa->zone   || (fa->zone == tmp->addr->zone)) && (fa->net == tmp->addr->net) && 
		    (fa->node == tmp->addr->node) && (fa->point == tmp->addr->point) && (fa->name) && 
		    (tmp->addr->name) && (strcasecmp(fa->name,tmp->addr->name) == 0)) {
			Syslog('m', "Address \"%s\" has local alias \"%s\"", ascinode(fa,0x7f), MBSE_SS(tmp->alias));
			return tmp->alias;
		}
	return NULL;
}



void readalias(char *fn)
{
	FILE			*fp;
	char			buf[256], *k, *v;
	struct aliaslist	*tmp = NULL;
	faddr			*ta = NULL;

	if ((fp = fopen(fn,"r")) == NULL) {
		WriteError("$cannot open system alias file %s", MBSE_SS(fn));
		return;
	}

	while (fgets(buf, sizeof(buf)-1, fp)) {
		k = strtok(buf, " \t:");
		v = strtok(NULL, " \t\n\r\0:");
		if (k && v)
			if ((ta = parsefaddr(v))) {
				if (alist) {
					tmp->next = (struct aliaslist *) xmalloc(sizeof(struct aliaslist));
					tmp = tmp->next;
				} else {
					alist = (struct aliaslist *) xmalloc(sizeof(struct aliaslist));
					tmp = alist;
				}
				tmp->next = NULL;
				tmp->addr = ta;
				ta = NULL;
				tmp->alias = xstrcpy(k);
			}
	}
	fclose(fp);
}

