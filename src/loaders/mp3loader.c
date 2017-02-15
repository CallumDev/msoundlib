#include "../soundmanager_internal.h"
#include "../loaders.h"
#ifdef CONFIG_ENABLE_MP3
#include <mpg123.h>

ssize_t mpg123_streamread(void *handle, void *ptr, size_t size)
{
	stream_t stream = (stream_t)handle;
	return (ssize_t)stream->read(ptr, 1, size, stream);
}

off_t mpg123_streamlseek(void *handle, off_t offset, int pos)
{
	stream_t stream = (stream_t)handle;
	stream->seek(stream, (long int)offset, pos);
	return (off_t)(stream->tell(stream));
}

void mpg123_streamcleanup(void *handle)
{
	stream_t stream = (stream_t)handle;
	stream->close(stream);
}

size_t mp3_read(void* ptr, size_t size, size_t count, stream_t stream)
{
	mpg123_handle *mh = (mpg123_handle*)stream->userData;
	size_t read_size;
	mpg123_read(mh, (unsigned char*)ptr, size * count, &read_size);
	return read_size;
}

int mp3_seek(stream_t stream, long int offset, int origin)
{
	mpg123_handle *mh = (mpg123_handle*)stream->userData;
	return (int)mpg123_seek(mh, (off_t)offset, origin);
} 

long int mp3_tell(stream_t stream)
{
	mpg123_handle *mh = (mpg123_handle*)stream->userData;
	return (long int)mpg123_tell(mh);
}

void mp3_close(stream_t stream)
{
	mpg123_handle *mh = (mpg123_handle*)stream->userData;
	mpg123_close(mh);
	free(stream);
}

static int inited = 0;
static int initcode = 1;
static int init_lib()
{
	if(!inited) {
		inited = 1;
		if(mpg123_init() != MPG123_OK) {
			initcode = 0;
			return 0;
		}
		return 1;
	}
	return initcode;
}

streaming_sound_t mp3_getstream(stream_t stream, const char *filename)
{
	int err;
	if(!init_lib()) {
		LOG_ERROR("failed to init libmpg123");
		return NULL;
	}
	//Init handle
	mpg123_handle *mh;
	mh = mpg123_new(NULL, NULL);
	if(!mh) {
		LOG_ERROR("Failed to alloc libmpg123 handle");
		return NULL;
	}
	//Setup parameters
	mpg123_format_none(mh);
	const long *rates;
	size_t rate_count;
	mpg123_rates(&rates, &rate_count);
	for(int i = 0; i < rate_count; i++) {
		mpg123_format(mh, rates[i], MPG123_STEREO | MPG123_MONO, MPG123_ENC_SIGNED_16);
	}
	//Open file
	if((err = mpg123_replace_reader_handle(mh,
		&mpg123_streamread,
		&mpg123_streamlseek,
		&mpg123_streamcleanup
	)) != MPG123_OK) LOG_ERROR_F("mpg reader handle failed: %s", mpg123_plain_strerror(err));
	if(mpg123_open_handle(mh, (void*)stream) != MPG123_OK) {
		LOG_ERROR("mpg123_open_handle failed");
	}
	size_t done;
	if((err = mpg123_read(mh, NULL, 0, &done)) != MPG123_OK) {
		if(err != MPG123_DONE && err != MPG123_NEW_FORMAT) {
			LOG_ERROR_F("mp3 decode failed: %s", mpg123_plain_strerror(err));
			stream->close(stream);
			return NULL;
		}
	}
	long rate;
	int channels;
	int encoding;
	mpg123_getformat(mh, &rate, &channels, &encoding);
	//no other formats
	mpg123_format_none(mh);
	mpg123_format(mh, rate, channels == 2 ? MPG123_STEREO : MPG123_MONO, MPG123_ENC_SIGNED_16);
	//begin
	mpg123_seek(mh, 0, SEEK_SET);
	//Create stream
	streaming_sound_t snd = (streaming_sound_t)malloc(sizeof(struct sound_stream));
	snd->dataSize = -1;
	snd->blockSize = 32768;
	snd->frequency = (int)rate;
	if(channels == 1) {
		snd->format = AL_FORMAT_MONO16;
	} else {
		snd->format = AL_FORMAT_STEREO16;
	}
	stream_t mp3stream = stream_alloc();
	mp3stream->read = &mp3_read;
	mp3stream->seek = &mp3_seek;
	mp3stream->tell = &mp3_tell;
	mp3stream->close = &mp3_close;
	mp3stream->userData = (void*)mh;
	snd->stream = mp3stream;
	return snd;
}
#endif