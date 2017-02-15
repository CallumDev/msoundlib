#include "soundmanager.h"
#include "soundmanager_internal.h"
#include "loaders.h"
#include <string.h>

sound_t sound_fromstream(stream_t stream, const char *filename);

int sound_read_all(streaming_sound_t source, sound_t dest)
{
	if(source->dataSize > 0) {
		dest->dataSize = source->dataSize;
		dest->data = (void*)malloc(source->dataSize);
		if(!source->stream->read(dest->data, source->dataSize, 1, source->stream)) {
			free(dest->data);
			return 0;
		}
		return 1;
	} else {
		//Surely there's a better way of doing this
		unsigned char *buffer = (unsigned char *)malloc(source->blockSize);
		unsigned char *buffer2 = (unsigned char *)malloc(source->blockSize);
		int bufferSize = source->blockSize;
		int bufferCurrent = 0;
		size_t read = 0;
		while((read = source->stream->read(buffer2, source->blockSize, 1, source->stream))) {
			int r = (int)read;
			if(bufferCurrent + r > bufferSize) {
				bufferSize *= 2;
				buffer = realloc(buffer, bufferSize);
			}
			memcpy(&buffer[bufferCurrent], buffer2, read);
			bufferCurrent += r;
		}
		buffer = realloc(buffer, bufferCurrent);
		free(buffer2);
		printf("Decoded %d bytes\n", bufferCurrent);
		dest->data = buffer;
		dest->dataSize = bufferCurrent;
		return 1;
	}
	return 0;
}

MSOUND_EXPORT(sound_t) sound_fromfile(const char *filename)
{
	stream_t stream = stream_openread(filename);
	if(!stream) {
		LOG_ERROR_F("Failed to open file '%s'", filename);
		return NULL;
	}
	streaming_sound_t data = auto_getstream(stream, filename);
	if(!data) {
		return NULL;
	}
	sound_t retsound = (sound_t)malloc(sizeof(struct sound_internal));
	retsound->frequency = data->frequency;
	retsound->format = data->format;
	if(!sound_read_all(data, retsound)) {
		LOG_ERROR_F("Failed to read WAVE data for '%s'", filename);
		data->stream->close(data->stream);
		free(retsound);
		retsound = NULL;
	}
	data->stream->close(data->stream);
	free(data);
	return retsound;
}

MSOUND_EXPORT(void) sound_destroy(sound_t sound)
{
	if(sound) {
		free(sound->data);
		free(sound);
	}
}