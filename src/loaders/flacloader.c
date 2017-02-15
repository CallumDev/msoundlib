#include "../soundmanager_internal.h"
#include "../loaders.h"
#ifdef CONFIG_ENABLE_FLAC
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include "dr_flac.h"

size_t read_stream_drflac(void *pUserData, void *pBufferOut, size_t size)
{
	stream_t stream = (stream_t)pUserData;
	return stream->read(pBufferOut, 1, size, stream);
}

int seek_stream_drflac(void *pUserData, int offset, drflac_seek_origin origin)
{
	stream_t stream = (stream_t)pUserData;
	int or;
	if(origin == drflac_seek_origin_start) or = SEEK_SET;
	if(origin == drflac_seek_origin_current) or = SEEK_CUR;
	return !stream->seek(stream, offset, or);
}


typedef struct {
	drflac *pFlac;
	stream_t baseStream;
} flac_userdata_t;

size_t flac_read(void* ptr, size_t size, size_t count, stream_t stream)
{
	flac_userdata_t *userdata = (flac_userdata_t*)stream->userData;
	size_t sampleCount = (size * count) / 2;
	size_t samplesRead = (size_t)drflac_read_s16(userdata->pFlac, (size_t)sampleCount, (dr_int16*)ptr);
	return samplesRead * 2;
}

int flac_seek(stream_t stream, long int offset, int origin)
{
	if(offset != 0 || origin != SEEK_SET) {
		LOG_ERROR("flac seek only supports reset");
		return 0;
	}
	flac_userdata_t *userdata = (flac_userdata_t*)stream->userData;
	return drflac_seek_to_sample(userdata->pFlac, 0);
}

long int flac_tell(stream_t stream)
{
	LOG_ERROR("flac_tell not implemented");
	return 0;
}

void flac_close(stream_t stream)
{
	flac_userdata_t *userdata = (flac_userdata_t*)stream->userData;
	drflac_close(userdata->pFlac);
	userdata->baseStream->close(userdata->baseStream);
	free(userdata);
	free(stream);
}

streaming_sound_t flac_getstream(stream_t stream, const char *filename)
{
	drflac *pFlac = drflac_open(read_stream_drflac, seek_stream_drflac, (void*)stream);
	if(!pFlac) {
		if(!strcmp(filename,"OGG_STREAM")) {
			LOG_ERROR("Unable to open ogg file");
		} else {
			LOG_ERROR("Flac decode failed");
		}
		stream->close(stream);
		return NULL;
	}

	flac_userdata_t *userdata = (flac_userdata_t*)malloc(sizeof(flac_userdata_t));
	userdata->pFlac = pFlac;
	userdata->baseStream = stream;

	stream_t data = stream_alloc();
	data->read = &flac_read;
	data->seek = &flac_seek;
	data->tell = &flac_tell;
	data->close = &flac_close;
	data->userData = userdata;

	streaming_sound_t retsound = (streaming_sound_t)malloc(sizeof(struct sound_stream));
	retsound->frequency = pFlac->sampleRate;
	retsound->stream = data;
	retsound->dataSize = -1;
	retsound->blockSize = 8192;
	if(pFlac->channels == 2) {
		retsound->format = AL_FORMAT_STEREO16;
	} else {
		retsound->format = AL_FORMAT_MONO16;
	}
	return retsound;
}

#endif