#ifndef _MYSTDLIB_H_
#define _MYSTDLIB_H_
// Common headers
#include <stdio.h>
#include <stdlib.h>

// Quick and dirty threading abstraction
#ifdef _WIN32
/*windows threads*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define thread_t HANDLE
#define mutex_t HANDLE
#define thread_create(thread,start_routine,arg) do { \
	HANDLE __ident_ = CreateThread(NULL, 0, (start_routine), (arg), 0, NULL); \
	*(thread) = __ident_; \
} while (0)
#define thread_sleep(millisecs) Sleep((millisecs));
#define thread_mutex_init(mutex) do { \
	HANDLE __ident_ = CreateMutex(NULL, FALSE, NULL); \
	*(mutex) = __ident_; \
} while(0)
#define thread_mutex_lock(mutex) WaitForSingleObject(*(mutex), INFINITE)
#define thread_mutex_unlock(mutex) ReleaseMutex(*(mutex))
#define thread_mutex_destroy(mutex) CloseHandle(*(mutex))
#else
/*unix threads*/
#include <pthread.h>
#include <unistd.h>
#define thread_t pthread_t
#define mutex_t pthread_mutex_t
#define thread_create(thread,start_routine,arg) pthread_create((thread),NULL,(start_routine),(arg))
#if __APPLE__
#define thread_sleep(millisecs) usleep((useconds_t)((millisecs) * 1000));
#else
#include <time.h>
#define thread_sleep(millisecs) do { \
	struct timespec __sleeptime, __sleeptime2; \
	__sleeptime.tv_sec = 0; \
	__sleeptime.tv_nsec = (millisecs) * 1000000; \
	nanosleep(&__sleeptime, &__sleeptime2); \
} while(0)
#endif
#define thread_mutex_lock(mutex) pthread_mutex_lock((mutex))
#define thread_mutex_unlock(mutex) pthread_mutex_unlock((mutex))
#define thread_mutex_init(mutex) pthread_mutex_init((mutex), NULL);
#define thread_mutex_destroy(mutex) pthread_mutex_destroy((mutex));
#endif

// fopen wrapper
#ifdef _WIN32
FILE *fopen_wrapper(const char *filename, const char *mode);
#define platform_fopen(filename,mode) fopen_wrapper((filename),(mode))
#else
#define platform_fopen(filename,mode) fopen((filename),(mode))
#endif

#endif