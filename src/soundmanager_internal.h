#ifndef _OPENAL_UTILS_H_
#define _OPENAL_UTILS_H_
/* include correct OpenAL headers */
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <stdio.h>
#include "mystdlib.h"
#include "stream.h"
#include "config.h"

struct sound_internal {
	void *data;
	int dataSize;
	int frequency;
	int format;
};

/* Decoding and loading */

struct sound_stream {
	stream_t stream;
	int dataSize;
	int frequency;
	int format;
	int blockSize;
	int source;
	int dataLeft;
	volatile int state;
};

/* Error Checking and Logging */

#ifndef LOG_ERROR
#define LOG_ERROR_F(x, ...) printf("[soundmanager] err: " #x "\n", __VA_ARGS__);
#define LOG_ERROR(x) printf("[soundmanager] err: " #x "\n");
#endif

const char *GetOpenALErrorString(int errID);

void CheckOpenALError(const char* stmt, const char* fname, int line);

#ifndef AL_CHECK
#ifdef _DEBUG
	#define AL_CHECK(stmt) do { \
		stmt; \
		CheckOpenALError(#stmt, __FILE__, __LINE__); \
	} while (0);
#else
	#define AL_CHECK(stmt) stmt
#endif
#endif

#endif