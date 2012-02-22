/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>
#include <threadlist.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, int initial_count)
{
        struct semaphore *sem;

        KASSERT(initial_count >= 0);

        sem = kmalloc(sizeof(struct semaphore));
        if (sem == NULL) {
                return NULL;
        }

        sem->sem_name = kstrdup(name);
        if (sem->sem_name == NULL) {
                kfree(sem);
                return NULL;
        }

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
        sem->sem_count = initial_count;

        return sem;
}

void
sem_destroy(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
        kfree(sem->sem_name);
        kfree(sem);
}

void
P(struct semaphore *sem)
{
        KASSERT(sem != NULL);

        /*
         * May not block in an interrupt handler.
         *
         * For robustness, always check, even if we can actually
         * complete the P without blocking.
         */
        KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&sem->sem_lock);
        while (sem->sem_count == 0) {
		/*
		 * Bridge to the wchan lock, so if someone else comes
		 * along in V right this instant the wakeup can't go
		 * through on the wchan until we've finished going to
		 * sleep. Note that wchan_sleep unlocks the wchan.
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_lock(sem->sem_wchan);
		spinlock_release(&sem->sem_lock);
                wchan_sleep(sem->sem_wchan);

		spinlock_acquire(&sem->sem_lock);
        }
        KASSERT(sem->sem_count > 0);
        sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
        KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

        sem->sem_count++;
        KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
        struct lock *lock;

        lock = kmalloc(sizeof(struct lock));
        if (lock == NULL) {
            return NULL;
        }

        lock->lk_name = kstrdup(name);
        if (lock->lk_name == NULL) {
                kfree(lock);
                return NULL;
        }

        // init waiting channel
        lock->lk_wchan = wchan_create(lock->lk_name);
        if (lock->lk_wchan == NULL) {
            kfree(lock->lk_name);
            kfree(lock);
            return NULL;
        }

        // init spinlock
        spinlock_init(&lock->lk_lock);

        // add stuff here as needed
        lock->lk_value = 0;
        lock->lk_holder = NULL;

        return lock;
}

void
lock_destroy(struct lock *lock)
{
        KASSERT(lock != NULL);

        // add stuff here as needed
        spinlock_cleanup(&lock->lk_lock);
        wchan_destroy(lock->lk_wchan);

        lock->lk_holder = NULL;
        kfree(lock->lk_name);
        kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
        // Write this
        KASSERT(lock != NULL);
        // why need this ? thread.h
        KASSERT(curthread->t_in_interrupt == false);

        spinlock_acquire(&lock->lk_lock);
        // while (lock->lk_value != 0)
        while (lock->lk_value == 1) {
            wchan_lock(lock->lk_wchan);
            spinlock_release(&lock->lk_lock);
            wchan_sleep(lock->lk_wchan);
            spinlock_acquire(&lock->lk_lock);
        }
        KASSERT(lock->lk_value == 0);
        lock->lk_value = 1;
        // check curthread
        lock->lk_holder = curthread;
        spinlock_release(&lock->lk_lock);
        //(void)lock;  // suppress warning until code gets written
}

void
lock_release(struct lock *lock)
{
        // Write this
        KASSERT(lock != NULL);

        spinlock_acquire(&lock->lk_lock);
        if(lock_do_i_hold(lock) && lock->lk_value == 1) {
            lock->lk_value = 0;
            lock->lk_holder = NULL;
            wchan_wakeone(lock->lk_wchan);
        }
        spinlock_release(&lock->lk_lock);
        //(void)lock;  // suppress warning until code gets written
}

bool
lock_do_i_hold(struct lock *lock)
{
        // Write this
        // what this means ?
        KASSERT(lock != NULL);

		if(!CURCPU_EXISTS()) {
			return true;
		}
		return (lock->lk_holder == curthread);

       // (void)lock;  // suppress warning until code gets written

       // return true; // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
        struct cv *cv;

        cv = kmalloc(sizeof(struct cv));
        if (cv == NULL) {
                return NULL;
        }

        cv->cv_name = kstrdup(name);
        if (cv->cv_name==NULL) {
                kfree(cv);
                return NULL;
        }

        // add stuff here as needed
        cv->cv_wchan = wchan_create(cv->cv_name);
        if (cv->cv_wchan == NULL) {
            kfree(cv->cv_name);
            kfree(cv);
            return NULL;
        }

        return cv;
}

void
cv_destroy(struct cv *cv)
{
        KASSERT(cv != NULL);

        // add stuff here as needed
        wchan_destroy(cv->cv_wchan);
        kfree(cv->cv_name);
        kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
        // Write this
        KASSERT(cv != NULL && lock != NULL);

        lock_release(lock);
        wchan_lock(cv->cv_wchan);
        wchan_sleep(cv->cv_wchan);
        lock_acquire(lock);
       // (void)cv;    // suppress warning until code gets written
       // (void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    // Write this
    KASSERT(cv != NULL);

    wchan_wakeone(cv->cv_wchan);
	//(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	// Write this
	KASSERT(cv != NULL);

	wchan_wakeall(cv->cv_wchan);
	//(void)cv;    // suppress warning until code gets written
	(void)lock;  // suppress warning until code gets written
}


////////////////////////////////////////////////////////////
// my_threadlist
//

void my_threadlist_init(struct my_threadlist *p)
{
    DEBUGASSERT(p != NULL);

    p->count = 0;
    p->next = NULL;

}

void my_threadlist_add(struct my_threadlist *p, struct thread *t)
{
    DEBUGASSERT(p != NULL);
    DEBUGASSERT(t != NULL);

    struct my_threadlist *cursor, *node;

    // malloc a new node
    node = kmalloc(sizeof(struct my_threadlist));
    if(node == NULL) {
        panic("my_threadlist_add: add new thread failed.\n");
    }
    node->next = NULL;
    node->t = t;

    cursor = p;
    while(cursor->next != NULL) {
        cursor = cursor->next;
    }
    cursor->next = node;
    p->count++;
}

void my_threadlist_remove(struct my_threadlist *p, struct thread *t)
{
    DEBUGASSERT(p != NULL);
    DEBUGASSERT(t != NULL);

    struct my_threadlist *c1, *c2;

    c1 = p;
    c2 = p->next;

    while(c2 != NULL) {
        if(c2->t == t) {
            c1->next = c2->next;
            p->count--;
            kfree(c2);
            break;
        }
        c1 = c2;
        c2 = c2->next;
    }
}
////////////////////////////////////////////////////////////
//
// Reader-Writer Lock.


struct rwlock *
rwlock_create(const char *name)
{
    struct rwlock *rw;
    char *read_wchan_name, *write_wchan_name;

    rw = kmalloc(sizeof(struct rwlock));
    if (rw == NULL) {
        return NULL;
    }

    rw->rwlock_name = kstrdup(name);
    if (rw->rwlock_name == NULL) {
        kfree(rw);
        return NULL;
        }

	// reader wait channel
    read_wchan_name = kmalloc(5*sizeof(char));
    if (read_wchan_name == NULL) {
        return NULL;
    }
	// use snprintf
    //read_wchan_name = strcat(read_wchan_name, name);
    //read_wchan_name = strcat(read_wchan_name, "read\0");
	rw->reader_wchan = wchan_create("read");
	if (rw->reader_wchan == NULL) {
	    kfree(read_wchan_name);
		kfree(rw->rwlock_name);
		kfree(rw);
		return NULL;
	}

	// writer wait channel
    write_wchan_name = kmalloc(6*sizeof(char));
    if (write_wchan_name == NULL) {
        return NULL;
    }
    //write_wchan_name = strcat(write_wchan_name, name);
    //write_wchan_name = strcat(write_wchan_name, "write\0");
    rw->writer_wchan = wchan_create("write");
    if (rw->writer_wchan == NULL) {
        kfree(read_wchan_name);
        kfree(write_wchan_name);
        kfree(rw->rwlock_name);
        kfree(rw);
        return NULL;
    }

	spinlock_init(&rw->rwlock_spinlock);
    //threadlist_init(&rw->rwlock_list);
    my_threadlist_init(&rw->rwlock_list);

    rw->rwlock_count = 0;
    rw->rwlock_mode = 0;

    kfree(read_wchan_name);
    kfree(write_wchan_name);
    return rw;

}

void
rwlock_destroy(struct rwlock *rw)
{
    KASSERT(rw != NULL);

	// wchan_cleanup will assert if anyone's waiting on it

	wchan_destroy(rw->writer_wchan);
	wchan_destroy(rw->reader_wchan);
	//threadlist_cleanup(&rw->rwlock_list);
	spinlock_cleanup(&rw->rwlock_spinlock);
    kfree(rw->rwlock_name);
    kfree(rw);

}

void
rwlock_acquire_read(struct rwlock *rw)
{

    KASSERT(rw != NULL);
    //KASSERT(mode == -1 || mode == 1);

    // if current state is read
    spinlock_acquire(&rw->rwlock_spinlock);
    if(rw->rwlock_mode == -1) {
        // if total reader # excel MAX_NUM
        if(rw->rwlock_count > 10) {
            wchan_lock(rw->reader_wchan);
            spinlock_release(&rw->rwlock_spinlock);
            wchan_sleep(rw->reader_wchan);
        }
        // else add it into the threadlist
        else {
            //threadlist_addhead(&rw->rwlock_list, curthread);
            my_threadlist_add(&rw->rwlock_list, curthread);
            rw->rwlock_count++;
            spinlock_release(&rw->rwlock_spinlock);
        }
    }
    // if current state is write
    else if(rw->rwlock_mode == 1) {
            wchan_lock(rw->reader_wchan);
            spinlock_release(&rw->rwlock_spinlock);
            wchan_sleep(rw->reader_wchan);
    }
    else {
		// we'll be in reader mode now
        rw->rwlock_mode = -1;
        rw->rwlock_count = 1; // safer
		//rw->rwlock_count++;
        //threadlist_addhead(&rw->rwlock_list, curthread);
        my_threadlist_add(&rw->rwlock_list, curthread);
        spinlock_release(&rw->rwlock_spinlock);
    }
}

void
rwlock_acquire_write(struct rwlock *rw)
{
	KASSERT(rw != NULL);

	spinlock_acquire(&rw->rwlock_spinlock);
	// we'll be in write mode
	if (rw->rwlock_mode == 0) {

		rw->rwlock_mode = 1;
		rw->rwlock_count = 1;
		//threadlist_addhead(&rw->rwlock_list, curthread);
		my_threadlist_add(&rw->rwlock_list, curthread);
		spinlock_release(&rw->rwlock_spinlock);

	}
	else if (rw->rwlock_mode == 1) {

		if(rw->rwlock_count == 0) {

			rw->rwlock_count = 1;
			//threadlist_addhead(&rw->rwlock_list, curthread);
			my_threadlist_add(&rw->rwlock_list, curthread);
			spinlock_release(&rw->rwlock_spinlock);
		}
		else {

			wchan_lock(rw->writer_wchan);
			spinlock_release(&rw->rwlock_spinlock);
			wchan_sleep(rw->writer_wchan);
		}
	}
	else {
		// if it is in reader mode

		wchan_lock(rw->writer_wchan);
		spinlock_release(&rw->rwlock_spinlock);
		wchan_sleep(rw->writer_wchan);
	}
}

void
rwlock_release_read(struct rwlock *rw)
{

    KASSERT(rw != NULL);

    spinlock_acquire(&rw->rwlock_spinlock);
    //threadlist_remove(&rw->rwlock_list, curthread);
    my_threadlist_remove(&rw->rwlock_list, curthread);

    if(rw->rwlock_list.count == 0) {
    // or switch to write mode if it was in read mode
        if(!wchan_isempty(rw->writer_wchan)) {
			rw->rwlock_mode = 1;
			rw->rwlock_count = 0;
            wchan_wakeone(rw->writer_wchan);
        }
        else if(!wchan_isempty(rw->reader_wchan)){
            rw->rwlock_mode = -1;
            rw->rwlock_count = 0;
            wchan_wakeall(rw->reader_wchan);
        }
        else {
            rw->rwlock_mode = 0;
            rw->rwlock_count = 0;
        }
    }
    spinlock_release(&rw->rwlock_spinlock);

}

void
rwlock_release_write(struct rwlock *rw)
{

	KASSERT(rw != NULL);

	spinlock_acquire(&rw->rwlock_spinlock);
	//threadlist_remove(&rw->rwlock_list, curthread);
    my_threadlist_remove(&rw->rwlock_list, curthread);

	if (rw->rwlock_list.count == 0) {
        if(!wchan_isempty(rw->reader_wchan)) {
            rw->rwlock_mode = -1;
            rw->rwlock_count = 0;
            wchan_wakeall(rw->reader_wchan);
        }
        else if(!wchan_isempty(rw->writer_wchan)) {
            rw->rwlock_mode = 1;
            rw->rwlock_count = 0;
            wchan_wakeone(rw->writer_wchan);
        }
        else {
            rw->rwlock_mode = 0;
            rw->rwlock_count = 0;
        }
	}
	spinlock_release(&rw->rwlock_spinlock);
}
