#ifndef _STREAM_H_
#define _STREAM_H_
#include <stdio.h>
typedef struct stream *stream_t;

struct stream {
	size_t (*read)(void*,size_t,size_t,stream_t);
	int (*seek)(stream_t,long int,int);
	long int (*tell)(stream_t);
	void (*close)(stream_t);
	void* userData;
};

stream_t stream_openread(const char *filename);
int stream_fgetc(stream_t stream);
stream_t stream_wrap(stream_t src, int len, int closeparent);
/* These methods are for C# interop purposes only - create and destroy a stream object */
stream_t stream_alloc();
void stream_destroy(stream_t stream);
#endif