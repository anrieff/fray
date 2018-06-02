/***************************************************************************
 *   Copyright (C) 2009-2018 by Veselin Georgiev, Slavomir Kaslev,         *
 *                              Deyan Hadzhiev et al                       *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @File cxxptl-sdl.cpp
 * @Brief SDL port of the CXXPTL library
 */
#include "cxxptl-sdl.h"
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
// also, define the macros min and max. Otherwise, at least on MSVC 7.1, they get defined
// to some actual working macros by windows.h's minions, but this puzzles up std::min and std::max
#define min min
#define max max
#include <windows.h>

#define GET_PROCESSOR_COUNT_DEFINED
int system_get_processor_count(void)
{
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return system_info.dwNumberOfProcessors;
}

int atomic_add(volatile int *addr, int val)
{
	return InterlockedExchangeAdd((long*)addr, val);
}

#else
// !_WIN32:
int atomic_add(volatile int *addr, int val)
{
	__asm__ __volatile__(
			"lock; xadd	%0,	%1\n"
	:"=r"(val), "=m"(*addr)
	:"0"(val), "m"(*addr)
			    );
	return val;
}

#endif

#if defined __linux__ || defined unix
// Linux here
#include <sys/sysinfo.h>
#include <unistd.h>

#define GET_PROCESSOR_COUNT_DEFINED
int system_get_processor_count(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif

#ifdef __APPLE__
// Mac OS X here
#include <unistd.h>
#include <mach/clock_types.h>
#include <mach/clock.h>
#include <mach/mach.h>

#define GET_PROCESSOR_COUNT_DEFINED
int system_get_processor_count(void)
{
	kern_return_t kr;
	host_basic_info_data_t basic_info;
	host_info_t info = (host_info_t)&basic_info;
	host_flavor_t flavor = HOST_BASIC_INFO;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	kr = host_info(mach_host_self(), flavor, info, &count);
	if (kr != KERN_SUCCESS) return 1;
	return basic_info.avail_cpus;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Here follow any platform-agnostic functions:

int get_processor_count(void)
{
	static int cached_cpucount = -1;
	if (cached_cpucount == -1) {
#ifdef GET_PROCESSOR_COUNT_DEFINED
		cached_cpucount = system_get_processor_count();
#else
		cached_cpucount = 1;
		fprintf(stderr, "get_processor_count(): Warning: Don't know how to obtain the number of\n");
		fprintf(stderr, "processors on your system. Assuming 1. Fix me, if that doesn't suit you.\n");
#endif
	}
	return cached_cpucount;
}

/**
 * @class Mutex
 */
Mutex::Mutex()
{
	cs = SDL_CreateMutex();
}

Mutex::~Mutex()
{
	SDL_DestroyMutex(cs);
}

void Mutex::enter(void)
{
	SDL_mutexP(cs);
}

void Mutex::leave(void)
{
	SDL_mutexV(cs);
}

/**
 * @class Event
 */
Event::Event()
{
	c = SDL_CreateCond();
	m = SDL_CreateMutex();
	state = 0;
}

Event::~Event()
{
	SDL_DestroyCond(c);
	SDL_DestroyMutex(m);
}

void Event::wait(void)
{
	SDL_LockMutex(m);
	if (state == 1) {
		state = 0;
		SDL_UnlockMutex(m);
		return;
	}
	SDL_CondWait(c, m);
	state = 0;
	SDL_UnlockMutex(m);
}

void Event::signal(void)
{
	SDL_LockMutex(m);
	state = 1;
	SDL_UnlockMutex(m);
	SDL_CondSignal(c);
}

/**
 * @class Barrier
 */
Barrier::Barrier(int cpu_count)
{
	m = SDL_CreateMutex();
	c = SDL_CreateCond();
	set_threads(cpu_count);
}

Barrier::~Barrier()
{
	SDL_DestroyCond(c);
	SDL_DestroyMutex(m);
}

void Barrier::set_threads(int cpu_count)
{
	counter = cpu_count;
	state = 0;
}

void Barrier::checkout(void)
{
	int r = --counter;
	if (r) {
		SDL_LockMutex(m);
		if (state == 1) {
			SDL_UnlockMutex(m);
			return;
		}
		SDL_CondWait(c, m);
		SDL_UnlockMutex(m);
	} else {
		SDL_LockMutex(m);
		state = 1;
		SDL_UnlockMutex(m);
		SDL_CondBroadcast(c);
	}
}

/**
 @class ThreadPool
 **/

void relent(void) // sleep off a bit
{
	SDL_Delay(10);
}

void my_thread_proc(ThreadInfoStruct*);
int sdl_thread_proc(void* data)
{
	ThreadInfoStruct *info = static_cast<ThreadInfoStruct*>(data);
	// fix broken pthreads implementations, where this proc's frame
	// is not 16 byte aligned...
	//
#if defined __GNUC__ && (__GNUC__ < 4) && !defined __x86_64__
	__asm __volatile("andl	$-16,	%%esp" ::: "%esp");
#endif
	//
	my_thread_proc(info);
	return 0;
}

void new_thread(SDL_Thread **handle, ThreadInfoStruct *info)
{
	*handle = SDL_CreateThread(sdl_thread_proc, info);
}
 
void ThreadPool::one_more_thread(void)
{
	ThreadInfoStruct& ti = info[active_count];
	ti.thread_index = active_count;
	ti.state = THREAD_CREATING;
	ti.counter = &counter;
	ti.thread_pool_event = &thread_pool_event;
	ti.execute_class = NULL;
	ti.waiting = &waiting;
	new_thread(&ti.thread, &ti);
	
	++active_count;
	for (int i = active_count-1; i >= 0; i--)
		info[i].thread_count = active_count;
}

ThreadPool::ThreadPool()
{
	active_count = 0;
	counter = 0;
	m_n = -1;
}

ThreadPool::~ThreadPool()
{
	killall_threads();
}

void ThreadPool::killall_threads(void)
{
	for (; active_count> 0; active_count --) {
		ThreadInfoStruct& i = info[active_count-1];
		while (i.state != THREAD_SLEEPING) relent();
		i.state = THREAD_EXITING;
		i.myevent.signal();
		while (i.state != THREAD_DEAD) relent();
	}
}

void ThreadPool::run(Parallel *para, int threads_count)
{
	if (threads_count == 1) {
		para->entry(0, 1);
		return;
	}
	while (active_count < threads_count) 
		one_more_thread();
	int n = threads_count;
	m_n = -1;
	
	waiting = true;
	counter = n;
	for (int i = 0; i < n; i++) {
		info[i].thread_index = i;
		info[i].thread_count = n;
		info[i].execute_class = para;
		while (info[i].state != THREAD_SLEEPING) {
			relent();
		}
		info[i].state = THREAD_RUNNING;
		info[i].myevent.signal();
	}
	thread_pool_event.wait();
	waiting = false;
	
	// round robin all threads until they come to rest
	while (1) {
		bool good = true;
		for (int i = 0; i < n; i++) if (info[i].state != THREAD_SLEEPING) good = false;
		if (good) break;
		relent();
	}
}

void ThreadPool::run_async(Parallel *para, int threads_count)
{
	while (active_count < threads_count) 
		one_more_thread();
	int n = threads_count;
	m_n = n;
	
	waiting = true;
	counter = n;
	for (int i = 0; i < n; i++) {
		info[i].thread_index = i;
		info[i].thread_count = n;
		info[i].execute_class = para;
		while (info[i].state != THREAD_SLEEPING) {
			relent();
		}
		info[i].state = THREAD_RUNNING;
		info[i].myevent.signal();
	}
}

void ThreadPool::wait(void)
{
	int n = m_n;
	if (n < 0) {
		/*
		 * Hmm...
		 * 1) wait() called twice?
		 * 2) wait() called after run() (not after run_async())?
		 * 3) wait() called after ThreadPool creation
		 * even more idiotic scenarios are possible...
		 */
		return;
	}
	thread_pool_event.wait();
	waiting = false;
	
	// round robin all threads until they come to rest
	while (1) {
		bool good = true;
		for (int i = 0; i < n; i++) if (info[i].state != THREAD_SLEEPING) good = false;
		if (good) break;
		relent();
	}
	m_n = -1; // prevent wait()ing again
}


void ThreadPool::preload_threads(int count)
{
	if (count > 1 && active_count < count)
		run(NULL, count);
}


/**
 thread function (my_thread_proc)
 **/
void my_thread_proc(ThreadInfoStruct *info)
{
	bool she = false;
	do {

		info->state = THREAD_SLEEPING;
		info->myevent.wait();
		switch (info->state) {
			case THREAD_EXITING: she = true; break;
			case THREAD_RUNNING:
			{
				if (info->execute_class) info->execute_class->entry(info->thread_index, info->thread_count);
				int res = --(*(info->counter));
				if (!res) {
					info->thread_pool_event->signal();
					do {
						relent();
					} while (*(info->waiting));
				}
				break;
			}
			default: break;
		}
	} while (!she);
	info->state = THREAD_DEAD;
}
