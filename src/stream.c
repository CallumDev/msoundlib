#include "soundmanager_internal.h"

size_t stream_fread(void* ptr, size_t size, size_t count, stream_t stream)
{
	FILE *file = (FILE*)stream->userData;
	return fread(ptr, size, count, file);
}

int stream_fseek(stream_t stream, long int offset, int origin)
{
	FILE *file = (FILE*)stream->userData;
	return fseek(file, offset, origin);
} 

long int stream_ftell(stream_t stream)
{
	FILE *file=(FILE*)stream->userData;
	return ftell(file);
}

void stream_fclose(stream_t stream)
{
	FILE *file = (FILE*)stream->userData;
	free(stream);
}

stream_t stream_alloc()
{
	return (stream_t)malloc(sizeof(struct stream));
}

void stream_destroy(stream_t stream)
{
	free(stream);
}

int stream_fgetc(stream_t stream) 
{
  unsigned char c;
  size_t size = stream->read(&c, 1, 1, stream);
  if(!size) {
    return EOF;
  }
  return (int)c;
}

stream_t stream_openread(const char *filename)
{
	FILE *fptr = platform_fopen(filename, "rb");
	if(!fptr)
		return NULL;
	stream_t stream = (stream_t)malloc(sizeof(struct stream));
	stream->read = &stream_fread;
	stream->seek = &stream_fseek;
	stream->tell = &stream_ftell;
	stream->close = &stream_fclose;
	stream->userData = (void*)fptr;
	return stream;
}

typedef struct {
	stream_t source;
	long int offset;
	int len;
	int closeparent;
} wrapper_data_t;

#define MIN(x,y) ((x) > (y) ? (y) : (x))

size_t stream_wrapread(void* ptr, size_t size, size_t count, stream_t stream)
{
	wrapper_data_t *data = (wrapper_data_t*)stream->userData;
	long int curr = data->source->tell(data->source);
	size = MIN((size_t)(data->len - (curr - data->offset)),size);
	if(size <= 0)
		return 0;
	size_t read = data->source->read(ptr, size, count, data->source);
	return read;
}

int stream_wrapseek(stream_t stream, long int offset, int origin)
{
	wrapper_data_t *data = (wrapper_data_t*)stream->userData;
	long int off = offset;
	if(origin == SEEK_SET) {
		off += data->offset;
	}
	if(origin == SEEK_END) {
		origin = SEEK_SET;
		off = data->offset + data->len + offset;
	}
	return data->source->seek(data->source, off, origin);
} 

long int stream_wraptell(stream_t stream)
{
	wrapper_data_t *data= (wrapper_data_t*)stream->userData;
	long int src = data->source->tell(data->source);
	return src - data->offset;
}

void stream_wrapclose(stream_t stream)
{
	wrapper_data_t *data = (wrapper_data_t*)stream->userData;
	if(data->closeparent)
		data->source->close(data->source);
	free(data);
	free(stream);
}

stream_t stream_wrap(stream_t src, int len, int closeparent)
{
	stream_t stream = (stream_t)malloc(sizeof(struct stream));
	wrapper_data_t *data = (wrapper_data_t*)malloc(sizeof(wrapper_data_t));
	data->offset = src->tell(src);
	data->len = len;
	data->source = src;
	data->closeparent = closeparent;
	stream->userData = (void*)data;
	stream->read = &stream_wrapread;
	stream->seek = &stream_wrapseek;
	stream->tell = &stream_wraptell;
	stream->close = &stream_wrapclose;
	return stream;
}



