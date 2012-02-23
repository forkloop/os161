/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver code for whale mating problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * 08 Feb 2012 : GWA : Driver code is in kern/synchprobs/driver.c. We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.


/////////////////////////////////////////FIXME////////////////////////////////////////////
#define NMATING 10

struct lock *pop_lock;
struct cv *maker_cv;

volatile unsigned long male_pop[NMATING];
volatile unsigned long female_pop[NMATING];
volatile int male_head, male_tail, female_head, female_tail;

void whalemating_init() {

    //extern int male_head, male_tail, female_head, female_tail;
    //extern struct lock *pop_lock;
    //extern struct cv *maker_cv;

    male_head = male_tail = 0;
    female_head = female_tail = 0;

    pop_lock = lock_create("population");
    if(pop_lock == NULL) {
        panic("whalemating: lock failed\n");
    }

    maker_cv = cv_create("maker cv");
    if(maker_cv == NULL) {
        panic("whalemating: condition variable failed\n");
    }

    //return;
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void whalemating_cleanup() {

    lock_destroy(pop_lock);
    cv_destroy(maker_cv);
  //return;
}

void
male(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  //(void)which;

  male_start();
	// Implement this function
    //extern int male_tail, female_head, female_tail;
    //extern unsigned long male_pop[];
    //extern struct lock *pop_lock;
    //extern struct cv *maker_cv;

    lock_acquire(pop_lock);
    male_pop[male_tail++] = which;

    if(female_head != female_tail)
        cv_signal(maker_cv, pop_lock);
    lock_release(pop_lock);

  male_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
female(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  //(void)which;

  female_start();
	// Implement this function
    //extern int male_head, male_tail, female_tail;
    //extern unsigned long female_pop[];
    //extern struct lock *pop_lock;
    //extern struct cv *maker_cv;

    lock_acquire(pop_lock);
    female_pop[female_tail++] = which;
    if(male_head != male_tail)
        cv_signal(maker_cv, pop_lock);
    lock_release(pop_lock);

  female_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

void
matchmaker(void *p, unsigned long which)
{
	struct semaphore * whalematingMenuSemaphore = (struct semaphore *)p;
  (void)which;

  matchmaker_start();
	// Implement this function
    //extern int male_head, male_tail, female_head, female_tail;
    //extern unsigned long male_pop[], female_pop[];
    //extern struct lock *pop_lock;
    //extern struct cv *maker_cv;

	lock_acquire(pop_lock);
	while((male_head==male_tail)||(female_head==female_tail))
        cv_wait(maker_cv, pop_lock);

    kprintf("Male %ld, Female %ld, and Maker %ld\n", male_pop[male_head++], female_pop[female_head++], which);
    lock_release(pop_lock);

  matchmaker_end();

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // whalemating driver can return to the menu cleanly.
  V(whalematingMenuSemaphore);
  return;
}

/*
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is,
 * of course, stable under rotation)
 *
 *   | 0 |
 * --     --
 *    0 1
 * 3       1
 *    3 2
 * --     --
 *   | 2 |
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X
 * first.
 *
 * You will probably want to write some helper functions to assist
 * with the mappings. Modular arithmetic can help, e.g. a car passing
 * straight through the intersection entering from direction X will leave to
 * direction (X + 2) % 4 and pass through quadrants X and (X + 3) % 4.
 * Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in drivers.c.
 */

// 13 Feb 2012 : GWA : Adding at the suggestion of Isaac Elbaz. These
// functions will allow you to do local initialization. They are called at
// the top of the corresponding driver code.

volatile int quad_status[4];
struct lock *quad_lock;
struct cv *quad_cv;

void stoplight_init() {

	// @forkloop
	//
//	int quad_status[4];
//	struct lock *quad_lock;
//	struct cv *quad_cv;

	quad_status[0]=quad_status[1]=quad_status[2]=quad_status[3]=0;

	quad_lock = lock_create("quadrant lock");
	if(quad_lock == NULL) {

		panic("stoplight: lock created failure.\n");
	}
	quad_cv = cv_create("quadrant cv");
	if(quad_cv == NULL) {

		panic("stoplight: cv created failure.\n");
	}
/*
    struct lock lock_list[4];
    char lock_name[6];



    for(int i=0;i<4;i++) {
        snprintf(lock_name, sizeof(lock_name), "lock%d\0", i);
        lock_list[i] = lock_create(lock_name);
        if(lock_list[i] == NULL) {
            panic("stoplight: lock created failure.\n");
        }
    }
	//return;
*/
}

// 20 Feb 2012 : GWA : Adding at the suggestion of Nikhil Londhe. We don't
// care if your problems leak memory, but if you do, use this to clean up.

void stoplight_cleanup() {

    lock_destroy(quad_lock);
    cv_destroy(quad_cv);
  //return;
}

void
gostraight(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  //(void)direction;

  // @forkloop
  //
	int next;
	next = (direction+3)%4;

	lock_acquire(quad_lock);
	while(quad_status[direction]||quad_status[next])
		cv_wait(quad_cv, quad_lock);
	quad_status[direction]=quad_status[next]=1;
	// first quadrant
	inQuadrant(direction);
	lock_release(quad_lock);

	// second quadrant
	lock_acquire(quad_lock);
	quad_status[direction]=0;
    inQuadrant(next);
    cv_broadcast(quad_cv, quad_lock);
	//cv_signal(quad_cv, quad_lock);
	lock_release(quad_lock);

	// leave
	lock_acquire(quad_lock);
	quad_status[next]=0;
    leaveIntersection();
    cv_broadcast(quad_cv, quad_lock);
	//cv_signal(quad_cv, quad_lock);
	lock_release(quad_lock);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnleft(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
  //(void)direction;

    // @forkloop
	//
    int next, nnext;
    next = (direction+3)%4;
    nnext = (direction+2)%4;

    lock_acquire(quad_lock);
	while(quad_status[direction]||quad_status[next])
		cv_wait(quad_cv, quad_lock);

	//quad_status[direction]=quad_status[next]=1;
	quad_status[direction]=1;
	quad_status[next] = 1;
	inQuadrant(direction);
	//cv_signal(quad_cv);
	lock_release(quad_lock);

	// third quad
	lock_acquire(quad_lock);
	while(quad_status[nnext])
		cv_wait(quad_cv, quad_lock);
	quad_status[direction]=0;
	//quad_status[next] = 0;
	inQuadrant(next);
	quad_status[nnext] = 1;
    cv_broadcast(quad_cv, quad_lock);
	//cv_signal(quad_cv, quad_lock);
	lock_release(quad_lock);

	// leave
	lock_acquire(quad_lock);
	inQuadrant(nnext);
	quad_status[next] = 0;
    cv_broadcast(quad_cv, quad_lock);
	//cv_signal(quad_cv, quad_lock);
	lock_release(quad_lock);

	lock_acquire(quad_lock);
	leaveIntersection();
	quad_status[nnext] = 0;
    cv_broadcast(quad_cv, quad_lock);
	//cv_signal(quad_cv, quad_lock);
	lock_release(quad_lock);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}

void
turnright(void *p, unsigned long direction)
{
	struct semaphore * stoplightMenuSemaphore = (struct semaphore *)p;
 // (void)direction;

    // @forkloop
	//
    // this won't have deadlock
    lock_acquire(quad_lock);
	while (quad_status[direction]) {
		cv_wait(quad_cv, quad_lock);
	}
	quad_status[direction] = 1;
	inQuadrant(direction);
    lock_release(quad_lock);

    lock_acquire(quad_lock);
    leaveIntersection();
    quad_status[direction]=0;
    //cv_signal(quad_cv, quad_lock);
    cv_broadcast(quad_cv, quad_lock);
    lock_release(quad_lock);

  // 08 Feb 2012 : GWA : Please do not change this code. This is so that your
  // stoplight driver can return to the menu cleanly.
  V(stoplightMenuSemaphore);
  return;
}
