#ifndef	_ORPHANS_H
#define	_ORPHANS_H

/* orphans.h */

/*
 * Linked list of orpaned ticfiles.
 */
typedef struct _orphans {
        struct _orphans     *next;              /* Linked list              */
        char                TicName[13];        /* TIC filename             */
        char                Area[21];           /* TIC area                 */
        char                FileName[81];       /* TIC filename             */
        unsigned            Orphaned    : 1;    /* Real orphaned file       */
        unsigned            BadCRC      : 1;    /* Present but wrong crc    */
        unsigned            Purged      : 1;    /* Can be purged            */
} orphans;

void tidy_orphans(orphans **);
void fill_orphans(orphans **, char *, char *, char *, int, int);

#endif
