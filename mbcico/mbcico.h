#ifndef	_MBCICO_H
#define	_MBCICO_H

/* $Id$ */

/*
 * Global threads for mbcico
 */
#define	NUM_THREADS	4
pthread_t threads[NUM_THREADS];


void usage(void);
void free_mem(void);
void die(int);

#endif
