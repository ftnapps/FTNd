/*****************************************************************************
 *
 * $Id: mbbw.c,v 1.4 2007/08/25 18:32:09 mbse Exp $
 * Purpose ...............: Dump Bluewave packets
 *
 *****************************************************************************
 * Copyright (C) 1997-2007
 *   
 * Michiel Broek		FIDO:	2:280/2802
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
#include "../lib/bluewave.h"



int main(int argc, char **argv)
{
    char	    *bwname, *temp;
    FILE	    *fp;
    INF_HEADER	    Inf;
    INF_AREA_INFO   AreaInf;
    MIX_REC	    Mix;
    FTI_REC	    Fti;
    int		    i, pos;

    printf("\nMBBW: MBSE BBS %s Bluewave Dump\n", VERSION);
    printf("      %s\n\n", COPYRIGHT);

    if (argc != 2) {
	printf("Usage:    mbbw [packetname]\n\n");
	return 0;
    }

    bwname = xstrcpy(argv[1]);
    temp = calloc(PATH_MAX, sizeof(char));
    snprintf(temp, PATH_MAX, "%s.INF", bwname);

    if ((fp = fopen(temp, "r"))) {
	printf("---- %s.INF -------------------------------------------------------\n", bwname);
	if (fread(&Inf, sizeof(INF_HEADER), 1, fp) == 1) {
	    printf("\n");
	    printf("Packet version           %d\n", Inf.ver);
	    for (i = 0; i < 5; i++)
		if (strlen((char *)Inf.readerfiles[i]))
		    printf("Display file %d           %s\n", i+1, Inf.readerfiles[i]);
	    printf("Registration number      %s\n", Inf.regnum);
	    printf("Login name               %s\n", Inf.loginname);
	    printf("Aliasname                %s\n", Inf.aliasname);
	    printf("Password type            %d\n", Inf.passtype);
	    printf("Network address          %d:%d/%d.%d\n", Inf.zone, Inf.net, Inf.node, Inf.point);
	    printf("Sysop                    %s\n", Inf.sysop);
	    printf("Control                  %sConfig %sFreq\n", (Inf.ctrl_flags & INF_NO_CONFIG) ? "No":"",
						       (Inf.ctrl_flags & INF_NO_FREQ) ? "No":"");
	    printf("System name              %s\n", Inf.systemname);
	    printf("Max freqs                %d\n", Inf.maxfreqs);
	    printf("Is QWK                   %s\n", Inf.is_QWK ? "Yes":"No");
	    printf("User flags               %s%s%s%s%s%s\n", (Inf.uflags & INF_HOTKEYS)     ? "Hotkeys ":"",
						    (Inf.uflags & INF_XPERT)       ? "Expert ":"",
						    (Inf.uflags & INF_GRAPHICS)    ? "ANSI ":"",
						    (Inf.uflags & INF_NOT_MY_MAIL) ? "Not-my-mail ":"",
						    (Inf.uflags & INF_EXT_INFO)    ? "Ext-info ":"",
						    (Inf.uflags & INF_NUMERIC_EXT) ? "Numeric-ext":"");
	    for (i = 0; i < 10; i++)
		if (strlen((char *)Inf.keywords[i]))
		    printf("Keywords %2d              %s\n", i+1, Inf.keywords[i]);
	    for (i = 0; i < 10; i++)
		if (strlen((char *)Inf.filters[i]))
		    printf("Filters %2d               %s\n", i+1, Inf.filters[i]);
	    for (i = 0; i < 3; i++)
		if (strlen((char *)Inf.macros[i]))
		    printf("Macro %d                  %s\n", i+1, Inf.macros[i]);
	    printf("Netmail flags                     %s%s%s%s%s%s%s\n", (Inf.netmail_flags & INF_CAN_CRASH)  ? "Crash ":"",
							     (Inf.netmail_flags & INF_CAN_ATTACH) ? "Attach ":"",
							     (Inf.netmail_flags & INF_CAN_KSENT)  ? "KSent ":"",
							     (Inf.netmail_flags & INF_CAN_HOLD)   ? "Hold ":"",
							     (Inf.netmail_flags & INF_CAN_IMM)    ? "Immediate ":"",
							     (Inf.netmail_flags & INF_CAN_FREQ)   ? "Freq ":"",
							     (Inf.netmail_flags & INF_CAN_DIRECT) ? "Direct ":"");
	    printf("Credits                  %d\n", Inf.credits);
	    printf("Debits                   %d\n", Inf.debits);
	    printf("Can forward              %s\n", Inf.can_forward ? "Yes":"No");
	    printf("INF header length (1230) %d\n", Inf.inf_header_len);
	    printf("INF areainfo len (80)    %d\n", Inf.inf_areainfo_len);
	    printf("MIX structlen (14)       %d\n", Inf.mix_structlen);
	    printf("FTI structlen (186)      %d\n", Inf.fti_structlen);
	    printf("Uses UPL file            %s\n", Inf.uses_upl_file ? "Yes":"No");
	    printf("From/To length           %d\n", Inf.from_to_len);
	    printf("Subject length           %d\n", Inf.subject_len);
	    printf("Packet ID                %s\n", Inf.packet_id);
	    printf("File list type           %d\n", Inf.file_list_type);
	    printf("Max packet size          %d\n", Inf.max_packet_size);

	    fseek(fp, (long)Inf.inf_header_len, SEEK_SET);
	    while (fread(&AreaInf, sizeof(INF_AREA_INFO), 1, fp)) {
		printf("\n");
		printf("Area number              %s\n", AreaInf.areanum);
		printf("Area tag                 %s\n", AreaInf.echotag);
		printf("Area title               %s\n", AreaInf.title);
		printf("Area flags               %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", 
							(AreaInf.area_flags & INF_SCANNING)      ? "Active ":"",
							(AreaInf.area_flags & INF_ALIAS_NAME)    ? "Aliases ":"",
							(AreaInf.area_flags & INF_ANY_NAME)      ? "Any-name ":"",
							(AreaInf.area_flags & INF_ECHO)          ? "Echo ":"",
							(AreaInf.area_flags & INF_NETMAIL)       ? "Netmail ":"",
							(AreaInf.area_flags & INF_POST)          ? "Post ":"",
							(AreaInf.area_flags & INF_NO_PRIVATE)    ? "No-private ":"",
							(AreaInf.area_flags & INF_NO_PUBLIC)     ? "No-public ":"",
							(AreaInf.area_flags & INF_NO_TAGLINE)    ? "No-tagline ":"",
							(AreaInf.area_flags & INF_NO_HIGHBIT)    ? "No-highbit ":"",
							(AreaInf.area_flags & INF_NOECHO)        ? "No-echo ":"",
							(AreaInf.area_flags & INF_HASFILE)       ? "Hasfile ":"",
							(AreaInf.area_flags & INF_PERSONAL)      ? "Personal ":"",
							(AreaInf.area_flags & INF_TO_ALL)        ? "To-all ":"");
		printf("Network type             %s\n", AreaInf.network_type ? "Internet":"Fidonet");
	    }
	    printf("\n");
	} else {
	    printf("Cannot read info header\n");
	}
	fclose(fp);
    }
    
    snprintf(temp, PATH_MAX, "%s.MIX", bwname);
    if ((fp = fopen(temp, "r"))) {
	printf("---- %s.MIX -------------------------------------------------------\n\n", bwname);
	printf("  Area  Total  Pers.   FTI ptr\n");
	printf("------ ------ ------ ---------\n");
	while (fread(&Mix, ORIGINAL_MIX_STRUCT_LEN, 1, fp)) {
	    printf("%6s %6d %6d %9d\n", Mix.areanum, Mix.totmsgs, Mix.numpers, Mix.msghptr);
	}
	printf("\n");
	fclose(fp);
    }

    snprintf(temp, PATH_MAX, "%s.FTI", bwname);
    pos = 0;
    if ((fp = fopen(temp, "r"))) {
	printf("---- %s.FTI -------------------------------------------------------\n\n", bwname);
	printf("   Pos From                                  Date                Msgnum Replto Replat    Ptr Length\n");
	printf("------ ------------------------------------- ------------------- ------ ------ ------ ------ ------\n");
	while (fread(&Fti, ORIGINAL_FTI_STRUCT_LEN, 1, fp)) {
	    printf("%6d %-36s %20s %6d %6d %6d %6d %6d\n", pos, Fti.from, Fti.date, Fti.msgnum, Fti.replyto, Fti.replyat,
		    Fti.msgptr, Fti.msglength);

	    pos += ORIGINAL_FTI_STRUCT_LEN;
	}
	fclose(fp);
    }

    free(bwname);
    free(temp);
    return 0;
}


