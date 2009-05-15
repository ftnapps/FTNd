#ifndef _FILE_H
#define _FILE_H

/* $Id: file.h,v 1.3 2005/10/11 20:49:48 mbse Exp $ */

void File_RawDir(char *);	/* Raw Directory List of a Directory         */
void File_List(void);		/* List files in current Area                */
void Download(void);		/* Tagged file download			     */
int  DownloadDirect(char *, int);/* Download a file direct		     */
int  KeywordScan(void);		/* Search a file on a keyword		     */
int  FilenameScan(void);	/* Search a file on filenames		     */
int  NewfileScan(int);		/* Scan for new files			     */
int  Upload(void);		/* Upload a file.			     */
void FileArea_List(char *);	/* Select file area			     */
void SetFileArea(unsigned int);	/* Select new area and load globals	     */
void EditTaglist(void);		/* Edit download taglist		     */
void List_Home(void);		/* List users home directory		     */
void Delete_Home(void);		/* Delete file from home directory	     */
int  Download_Home(void);	/* Allows user to download from home dir     */
int  Upload_Home(void);		/* Allows user to upload to home directory   */
void Copy_Home(void);		/* Copy a file to home directory	     */
void ViewFile(char *);		/* View a file in the current area.	     */

#endif

