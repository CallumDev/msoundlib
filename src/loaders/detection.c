#include "../soundmanager_internal.h"
#include "../loaders.h"
#include <string.h>

streaming_sound_t auto_getstream(stream_t stream, const char *filename)
{
	unsigned char magic[32];
	/* Read in magic */
	stream->read(magic,32,1,stream);
	stream->seek(stream,0,SEEK_SET);
	/* Detect file type */
	//Wav
	if(memcmp(magic, "RIFF", 4) == 0) {
		return riff_getstream(stream,filename);
	}
	//Ogg
	if(memcmp(magic, "OggS", 4) == 0) {
		#ifdef CONFIG_ENABLE_OGG
		return ogg_getstream(stream,filename);
		#else
		LOG_ERROR("Ogg support not enabled");
		stream->close(stream);
		return NULL;
		#endif
	}
	//Flac
	if(memcmp(magic, "fLaC", 4) == 0) {
		#ifdef CONFIG_ENABLE_FLAC
		return flac_getstream(stream, filename);
		#else
		LOG_ERROR("Flac support not enabled");
		stream->close(stream);
		return NULL;
		#endif
	}
	//Mp3
	if(memcmp(magic,"ID3", 3) == 0) {
		#ifdef CONFIG_ENABLE_MP3
		return mp3_getstream(stream, filename);
		#else
		LOG_ERROR("Mp3 support not enabled");
		stream->close(stream);
		return NULL;
		#endif
	}
	if(magic[0] == 0xFF && magic[1] == 0xFB) {
		#ifdef CONFIG_ENABLE_MP3
		return mp3_getstream(stream, filename);
		#else
		LOG_ERROR("Mp3 support not enabled");
		stream->close(stream);
		return NULL;
		#endif
	}

	LOG_ERROR_F("Unable to detect file type in '%s", filename);
	return NULL;
}
