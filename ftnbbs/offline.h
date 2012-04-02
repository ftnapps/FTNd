#ifndef	_OFFLINE_H
#define	_OFFLINE_H



/*
 *  Each area has a tag if the area exists, so we can check
 *  at login if the sysop added new areas (or deleted). This
 *  file is synced at login. Location: usershomedir/.olrtags
 */
struct	_olrtagrec {
	unsigned short	Available	: 1;	/* Is the area available     */
	unsigned short	Tagged		: 1;	/* Is this area tagged	     */
	unsigned short	ScanNew		: 1;	/* Scan for new mail	     */
};

struct	_olrtagrec	olrtagrec;


struct _qwkhdr {
	unsigned char	Msgstat;		/* Message status		*/
	unsigned char	Msgnum[7];		/* Message number		*/
	unsigned char	Msgdate[8];		/* Message date MM-DD-YY	*/
	unsigned char	Msgtime[5];		/* Message time HH:MM		*/
	unsigned char	MsgTo[25];		/* Message To:			*/
	unsigned char	MsgFrom[25];		/* Message From:		*/
	unsigned char	MsgSubj[25];		/* Message Subject:		*/
	unsigned char	Msgpass[12];		/* Message password		*/
	unsigned char	Msgrply[8];		/* Message reply to		*/
	unsigned char	Msgrecs[6];		/* Length in records		*/
	unsigned char	Msglive;		/* Message active status	*/
	unsigned char	Msgarealo;		/* Lo-byte message area		*/
	unsigned char	Msgareahi;		/* Hi-byte message area		*/
	unsigned char	Msgfiller[3];		/* Filler bytes			*/
};

struct _qwkhdr	Qwk;


void OLR_TagArea(void);				/* Tag area(s)		     */
void OLR_UntagArea(void);			/* Untag area(s)	     */
void OLR_SyncTags(void);			/* Sync tag/msg area(s)	     */
void OLR_ViewTags(void);			/* View tagged areas	     */
void OLR_Upload(void);				/* Upload mail packet	     */
void OLR_RestrictDate(void);			/* Restrict download date    */
void OLR_DownBW(void);				/* Download BlueWave format  */
void OLR_DownQWK(void);				/* Download QWK format	     */
void OLR_DownASCII(void);			/* Download ASCII format     */


#endif

