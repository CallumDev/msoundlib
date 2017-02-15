#include "../soundmanager_internal.h"
#include "../loaders.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char chunkID[4];
	uint32_t chunkSize;
	char format[4];
} riff_header_t;

typedef struct {
	char subChunkID[4];
	uint32_t subChunkSize;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
} wave_format_t;

typedef struct {
  char subChunkID[4];
  uint32_t subChunk2Size;
} wave_data_t;

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_MP3 0x55
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#define WAVE_FORMAT_IEEE_FLOAT		0x0003 /* IEEE Float */
#define WAVE_FORMAT_ALAW		0x0006 /* ALAW */
#define WAVE_FORMAT_MULAW		0x0007 /* MULAW */
#define WAVE_FORMAT_IMA_ADPCM		0x0011 /* IMA ADPCM */

streaming_sound_t riff_getstream(stream_t stream, const char *filename)
{
	wave_format_t wave_format;
	riff_header_t riff_header;
	wave_data_t wave_data;
	streaming_sound_t retsound;

	stream->read(&riff_header, sizeof(riff_header_t), 1, stream);

	if(memcmp(riff_header.chunkID, "RIFF", 4) != 0 ||
		memcmp(riff_header.format, "WAVE", 4) != 0) {
		LOG_ERROR_F("Invalid RIFF or WAVE header in '%s'", filename);
		stream->close(stream);
		return 0;
	} 

	stream->read (&wave_format, sizeof(wave_format_t), 1, stream);

	if(memcmp(wave_format.subChunkID, "fmt ", 4) != 0) {
		char actual[5] = { wave_format.subChunkID[0],
		wave_format.subChunkID[1], wave_format.subChunkID[2], wave_format.subChunkID[3], '\0' };
		LOG_ERROR_F("Invalid Wave Format in '%s' ('%s')", filename, actual);
		stream->close(stream);
		return 0;
	}

	if(wave_format.subChunkSize > 16)
		stream->seek(stream, wave_format.subChunkSize - 16, SEEK_CUR);

	int has_data = 0;
	while(!has_data) {
		if(!stream->read(&wave_data, sizeof(wave_data_t), 1, stream))
		{
			stream->close(stream);
			LOG_ERROR_F("Unable to find WAVE data in '%s'", filename);
			return 0;
		}
		has_data = (memcmp(wave_data.subChunkID, "data", 4) == 0) ? 1 : 0;
		if(!has_data) {
			//skip chunk
			stream->seek(stream, wave_data.subChunk2Size, SEEK_CUR);
		}
	}

	switch (wave_format.audioFormat) {
		case WAVE_FORMAT_PCM:
			break; //Default decoder
		case WAVE_FORMAT_MP3:
			//mp3 file wrapped in wav - because why not?
			#ifdef CONFIG_ENABLE_MP3
			return mp3_getstream(stream_wrap(stream, wave_data.subChunk2Size, 1), filename);
			#else
			LOG_ERROR("Mp3 support not enabled");
			stream->close(stream);
			return 0;
			#endif
		default:
			LOG_ERROR_F("Unsupported format in WAVE file '%s' '%x'",filename, wave_format.audioFormat);
			stream->close(stream);
			return 0;
	}

	retsound = (streaming_sound_t)malloc(sizeof(struct sound_stream));

	if(wave_format.numChannels == 1) {
		if (wave_format.bitsPerSample == 8) {
			retsound->format = AL_FORMAT_MONO8;
		} else if (wave_format.bitsPerSample == 16) {
			retsound->format = AL_FORMAT_MONO16;
		}
	} else if (wave_format.numChannels == 2) {
		if (wave_format.bitsPerSample == 8) {
			retsound->format = AL_FORMAT_STEREO8;
		} else if (wave_format.bitsPerSample == 16) {
			retsound->format = AL_FORMAT_STEREO16;
		}
	}

	

	retsound->frequency = wave_format.sampleRate;
	retsound->stream = stream_wrap(stream, wave_data.subChunk2Size, 1);
	retsound->dataSize = wave_data.subChunk2Size;
	retsound->blockSize = 32768;
	return retsound;
}