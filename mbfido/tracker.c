/*****************************************************************************
 *
 * File ..................: tosser/tracker.c
 * Purpose ...............: Netmail tracker / router
 * Last modification date : 12-May-2001
 *
 *****************************************************************************
 * Copyright (C) 1997-2001
 *   
 * Michiel Broek		FIDO:		2:280/2802
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
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************/

#include "../lib/libs.h"
#include "../lib/structs.h"
#include "../lib/records.h"
#include "../lib/common.h"
#include "../lib/clcomm.h"
#include "../lib/dbnode.h"
#include "../lib/dbftn.h"
#include "tracker.h"


extern char	nodes_fil[81];
extern long	nodes_pos;

/*
 *  Netmail tracker. Return TRUE if we found a valid route.
 *  If we did find a route, it is returned in the route pointer.
 *  If not, return FALSE. The calling program must bounce the
 *  original message.
 */
int TrackMail(fidoaddr too, fidoaddr *route)
{
	int	rc, i;

	Syslog('r', "Tracking destination %s", aka2str(too));

	rc = GetRoute(aka2str(too) , route);
	if (rc == R_NOROUTE) {
		WriteError("No route to %s, not parsed", aka2str(too));
		return R_NOROUTE;
	}
	if (rc == R_UNLISTED) {
		WriteError("No route to %s, unlisted node", aka2str(too));
		return R_UNLISTED;
	}

	/*
	 *  If local aka
	 */
	if (rc == R_LOCAL)
		return R_LOCAL;

	/*
	 *  Now look again if from the routing result we find a
	 *  direct link. If so, maybe adjust something.
	 */
	if (SearchNode(*route)) {
		Syslog('r', "Known node: %s", aka2str(nodes.Aka[0]));
		Syslog('r', "Check \"too\" %s", aka2str(too));

		if (nodes.RouteVia.zone) {
			route->zone  = nodes.RouteVia.zone;
			route->net   = nodes.RouteVia.net;
			route->node  = nodes.RouteVia.node;
			route->point = nodes.RouteVia.point;
			sprintf(route->domain, "%s", nodes.RouteVia.domain);
			return R_ROUTE;
		} else {
			for (i = 0; i < 20; i++)
				if (route->zone == nodes.Aka[i].zone)
					break;
			route->zone  = nodes.Aka[i].zone;
			route->net   = nodes.Aka[i].net;
			route->node  = nodes.Aka[i].node;
			route->point = nodes.Aka[i].point;
			sprintf(route->domain, "%s", nodes.Aka[i].domain);
			return R_ROUTE;
		}
	}

	return rc;
}



int AreWeHost(faddr *);
int AreWeHost(faddr *dest)
{
	int	i, j;

	/*
	 * First a fast run in our own zone.
	 */
	for (i = 0; i < 40; i++)
		if (CFG.akavalid[i] && (CFG.aka[i].zone == dest->zone))
			if (CFG.aka[i].node == 0)
				return i;

	for (i = 0; i < 40; i++)
		if (CFG.akavalid[i])
			if (SearchFidonet(dest->zone))
				for (j = 0; j < 6; j++)
					if (CFG.aka[i].zone == fidonet.zone[j])
						if (CFG.aka[i].node == 0)
							return i;

	return -1;
}



int AreWeHub(faddr *);
int AreWeHub(faddr *dest)
{
	int	i, j;
	node	*nl;
	faddr	*fido;

	for (i = 0; i < 40; i++)
		if (CFG.akavalid[i])
			if (CFG.aka[i].zone == dest->zone) {
				fido = fido2faddr(CFG.aka[i]);
				nl = getnlent(fido);
				tidy_faddr(fido);
				if (nl->addr.domain)
					free(nl->addr.domain);
				if (nl->type == NL_HUB)
					return i;
			}

	for (i = 0; i < 40; i++)
		if (CFG.akavalid[i])
			if (SearchFidonet(dest->zone))
				for (j = 0; j < 6; j++)
					if (CFG.aka[i].zone == fidonet.zone[j]) {
						fido = fido2faddr(CFG.aka[i]);
						nl = getnlent(fido);
						tidy_faddr(fido);
						if (nl->addr.domain)
							free(nl->addr.domain);
						if (nl->type == NL_HUB)
							return i;
					}

	return -1;
}



/*
 * Get routing information for specified netmail address.
 */
int GetRoute(char *ftn, fidoaddr *res)
{
	node		*dnlent, *bnlent;
	unsigned short	myregion;
	faddr		*dest, *best, *maddr;
	fidoaddr	*fido;
	int		me_host = -1, me_hub = -1;
	int		i;
	fidoaddr	dir;
	FILE		*fil;

	memset(res, 0, sizeof(fidoaddr));
	dest = parsefnode(ftn);
	if (SearchFidonet(dest->zone)) {
		if (dest->domain)
			free(dest->domain);
		dest->domain = xstrcpy(fidonet.domain);
	}
	best = bestaka_s(dest);
	Syslog('r', "Get route for: %s", ascfnode(dest, 0xff));

	/*
	 * Check if the destination is ourself.
	 */
	for (i = 0; i < 40; i++) {
		if (CFG.akavalid[i] && (CFG.aka[i].zone == dest->zone) &&
		   (CFG.aka[i].net == dest->net) && (CFG.aka[i].node == dest->node)) {
			if (dest->point == CFG.aka[i].point) {
				Syslog('+', "R: %s => Loc %s", ascfnode(dest, 0x0f), aka2str(CFG.aka[i]));
				memcpy(res, &CFG.aka[i], sizeof(fidoaddr));
				tidy_faddr(best);
				tidy_faddr(dest);
				return R_LOCAL;
			}
			if (dest->point && (!CFG.aka[i].point)) {
				Syslog('+', "R: %s => My point", ascfnode(dest, 0xff));
				fido = faddr2fido(dest);
				memcpy(res, fido, sizeof(fidoaddr));
				free(fido);
				tidy_faddr(best);
				tidy_faddr(dest);
				return R_DIRECT;
			}
		}
	}

        if (best->point) {
                /*
                 *  We are a point, so don't bother the rest of the tests, route
                 *  to our boss.
                 */
                res->zone = best->zone;
                res->net  = best->net;
                res->node = best->node;
                res->point = 0;
                Syslog('+', "R: %s => My boss %s", ascfnode(dest, 0x0f), aka2str(*res));
                tidy_faddr(best);
                tidy_faddr(dest);
                return R_DIRECT;
        }

	/*
	 * Now test several possible setup matches.
	 */
	dir.zone  = dest->zone;
	dir.net   = dest->net;
	dir.node  = dest->node;
	dir.point = dest->point;
	sprintf(dir.domain, "%s", dest->domain);

	/*
	 * First direct match
	 */
	if (SearchNode(dir)) {
		for (i = 0; i < 20; i++) {
			if ((dir.zone  == nodes.Aka[i].zone) &&
			    (dir.node  == nodes.Aka[i].node) &&
			    (dir.net   == nodes.Aka[i].net) &&
			    (dir.point == nodes.Aka[i].point)) {
				memcpy(res, &nodes.Aka[i], sizeof(fidoaddr));
				Syslog('+', "R: %s => Dir link %s", ascfnode(dest, 0x0f), aka2str(*res));
				tidy_faddr(best);
				tidy_faddr(dest);
				return R_DIRECT;
			}
		}
	}

	/*
	 * Again, but now for points
	 */
	dir.point = 0;
	if (SearchNode(dir)) {
		for (i = 0; i < 20; i++) {
			if ((dir.zone  == nodes.Aka[i].zone) &&
			    (dir.node  == nodes.Aka[i].node) &&
			    (dir.net   == nodes.Aka[i].net)) {
				memcpy(res, &nodes.Aka[i], sizeof(fidoaddr));
				res->point = 0;
				Syslog('+', "R: %s => Boss link %s", ascfnode(dest, 0x0f), aka2str(*res));
				tidy_faddr(best);
				tidy_faddr(dest);
				return R_DIRECT;
			}
		}
	}

	/*
	 * Check if we know the uplink, but first check if the node is listed.
	 */
	dnlent = (node *)malloc(sizeof(node));
	memcpy(dnlent, getnlent(dest), sizeof(node));
	if (dnlent->addr.domain)
		free(dnlent->addr.domain);

	if (!(dnlent->pflag & NL_DUMMY)) {
		dir.node = dnlent->upnode;
		dir.net  = dnlent->upnet;
		if (SearchNode(dir)) {
			for (i = 0; i < 20; i++) {
				if ((dir.zone  == nodes.Aka[i].zone) &&
				    (dir.node  == nodes.Aka[i].node) &&
				    (dir.net   == nodes.Aka[i].net)) {
					memcpy(res, &nodes.Aka[i], sizeof(fidoaddr));
					res->point = 0;
					Syslog('+', "R: %s => Uplink %s", ascfnode(dest, 0x0f), aka2str(*res));
					free(dnlent);
					tidy_faddr(best);
					tidy_faddr(dest);
					return R_DIRECT;
				}
			}
		}
	}

	/*
	 *  We don't know the route from direct links. We will first see
	 *  what we are, host, hub or node.
	 */
	me_host = AreWeHost(dest);
	if (me_host == -1)
		me_hub = AreWeHub(dest);
	bnlent = getnlent(best);
	myregion = bnlent->region;

	/*
	 *  This is default routing for hosts:
	 *   1.  Out of zone and region mail goes to the myzone:myregion/0
	 *   2.  Out of net mail goes to host myzone:destnet/0
	 *   3.  Nodes without hub are my downlinks, no route.
	 *   4.  The rest goes to the hubs.
	 */
	if (me_host != -1) {
		sprintf(res->domain, "%s", CFG.aka[me_host].domain);
		if (((myregion != dnlent->region) && (!(dnlent->pflag & NL_DUMMY))) ||
		     (CFG.aka[me_host].zone != dest->zone)) {
			res->zone = CFG.aka[me_host].zone;
			res->net  = myregion;
			Syslog('+', "R: %s => Region %s", ascfnode(dest, 0x0f), aka2str(*res));
			free(dnlent);
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_ROUTE;
		}
		if (CFG.aka[me_host].net != dest->net) {
			res->zone = dest->zone;
			res->net  = dest->net;
			Syslog('+', "R: %s => Host %s", ascfnode(dest, 0x0f), aka2str(*res));
			free(dnlent);
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_ROUTE;
		}
		if (dnlent->upnode == 0) {
			res->zone  = dest->zone;
			res->net   = dest->net;
			res->node  = dest->node;
			Syslog('+', "R: %s => Dir link %s", ascfnode(dest, 0x0f), aka2str(*res));
			free(dnlent);
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_ROUTE;
		}
		res->zone = CFG.aka[me_host].zone;
		res->net  = dnlent->upnet;
		res->node = dnlent->upnode;
		Syslog('+', "R: %s => Hub %s", ascfnode(dest, 0x0f), aka2str(*res));
		free(dnlent);
		if (bnlent->addr.domain)
			free(bnlent->addr.domain);
		tidy_faddr(best);
		tidy_faddr(dest);
		return R_ROUTE;
	}

	/*
	 *  This is the default routing for hubs.
	 *   1.  If the nodes hub is our own hub, it's a downlink.
	 *   2.  Kick everything else to the host.
	 */
	if (me_hub != -1) {
		sprintf(res->domain, "%s", CFG.aka[me_hub].domain);
		if ((dnlent->upnode == CFG.aka[me_hub].node) &&
		    (dnlent->upnet  == CFG.aka[me_hub].net) &&
		    (dnlent->addr.zone == CFG.aka[me_hub].zone)) {
			res->zone  = dest->zone;
			res->net   = dest->net;
			res->node  = dest->node;
			res->point = dest->point;
			Syslog('+', "R: %s => Dir link %s", ascfnode(dest, 0x0f), aka2str(*res));
			free(dnlent);
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_DIRECT;
		} else {
			res->zone = CFG.aka[me_hub].zone;
			res->net  = CFG.aka[me_hub].net;
			Syslog('+', "R: %s => My host %s", ascfnode(dest, 0xff), aka2str(*res));
			free(dnlent);
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_ROUTE;
		}
	}
	free(dnlent);

	/*
	 *  Routing for normal nodes, everything goes to the hub or host.
	 */
	if ((me_hub == -1) && (me_host == -1)) {
		if (bnlent->pflag != NL_DUMMY) {
			res->zone = bnlent->addr.zone;
			res->net  = bnlent->upnet;
			res->node = bnlent->upnode;
			sprintf(res->domain, "%s", bnlent->addr.domain);
			Syslog('+', "R: %s => %s", ascfnode(dest, 0xff), aka2str(*res));
			if (bnlent->addr.domain)
				free(bnlent->addr.domain);
			tidy_faddr(best);
			tidy_faddr(dest);
			return R_ROUTE;
		}

		if (bnlent->addr.domain)
			free(bnlent->addr.domain);

		/*
		 *  If the above failed, we are probably a new node without
		 *  a nodelist entry. We will switch to plan B.
		 */
		if ((fil = fopen(nodes_fil, "r")) != NULL) {
			fread(&nodeshdr, sizeof(nodeshdr), 1, fil);
			nodes_pos = -1;
			while (fread(&nodes, nodeshdr.recsize, 1, fil) == 1) {
				fseek(fil, nodeshdr.filegrp + nodeshdr.mailgrp, SEEK_CUR);
				for (i = 0; i < 20; i++) {
					if ((nodes.Aka[i].zone) && 
					    (nodes.Aka[i].zone == best->zone) &&
					    (nodes.Aka[i].net  == best->net)) {
						maddr = fido2faddr(nodes.Aka[i]);
						bnlent = getnlent(maddr);
						tidy_faddr(maddr);
						if (bnlent->addr.domain)
							free(bnlent->addr.domain);
						if ((bnlent->type == NL_HUB) || 
						    (bnlent->type == NL_HOST)) {
							fclose(fil);
							memcpy(res, &nodes.Aka[i], sizeof(fidoaddr));
							Syslog('r', "R: %s => %s", ascfnode(dest, 0xff), aka2str(*res));
							tidy_faddr(best);
							tidy_faddr(dest);
							return R_DIRECT;
						}
					}
				}
			}
			fclose(fil);
		}
	}

	WriteError("Routing parse error 2");
	tidy_faddr(best);
	tidy_faddr(dest);
	return R_NOROUTE;
}



void TestRoute(char *dest)
{
	fidoaddr	result;
	int		rc;

	
	rc = GetRoute(dest, &result);
	if (rc == R_NOROUTE)
		printf("Route %d %23s => no route\n", rc, dest);
	else if (rc == R_UNLISTED)
		printf("Route %d %23s => unlisted node\n", rc, dest);
	else
		printf("Route %d %23s => %s\n", rc, dest, aka2str(result));
}



void TestTracker(void)
{
	colour(7, 0);
	TestRoute((char *)"2:2801/16@fidonet");
	TestRoute((char *)"2:2801/16.1");
	TestRoute((char *)"2:2801/805.3");
	TestRoute((char *)"2:2801/899.1@fidonet");
	TestRoute((char *)"2:2801/890@fidonet");
	TestRoute((char *)"2:2801/1008");
	TestRoute((char *)"2:2801/21");
	TestRoute((char *)"2:2801/899@fidonet");
	TestRoute((char *)"2:2801/807");
	TestRoute((char *)"92:100/0@bibnet");
	TestRoute((char *)"92:100/5@bibnet");
	TestRoute((char *)"92:100/45");
	TestRoute((char *)"2:28/0");
	TestRoute((char *)"2:2801/1002@fidonet");
	TestRoute((char *)"2:2801/206");
	TestRoute((char *)"2:2/0@fidonet");
	TestRoute((char *)"2:2/3001");
	TestRoute((char *)"2:2801/28");
	TestRoute((char *)"2:2801/307.50");
	TestRoute((char *)"2:280/901");
	TestRoute((char *)"2:280/9");
	TestRoute((char *)"2:203/111");
	TestRoute((char *)"1:213/350");
	TestRoute((char *)"9:314/8@virnet");
	TestRoute((char *)"9:314/8.1@virnet");
}


