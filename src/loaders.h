#ifndef _LOADERS_H_
#define _LOADERS_H_
#include "soundmanager.h"
#include "stream.h"
//Main function - call this one
streaming_sound_t auto_getstream(stream_t stream, const char *filename);
//Codecs
streaming_sound_t riff_getstream(stream_t stream, const char *filename);
streaming_sound_t mp3_getstream(stream_t stream, const char *filename);
streaming_sound_t ogg_getstream(stream_t stream, const char *filename);
streaming_sound_t flac_getstream(stream_t stream, const char *filename);
#endif
