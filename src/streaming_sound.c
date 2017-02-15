#include "soundmanager.h"
#include "soundmanager_internal.h"
#include "loaders.h"

MSOUND_EXPORT(streaming_sound_t) streaming_sound_fromfile(const char *filename)
{
	printf("Called!\n");
	streaming_sound_t sound = auto_getstream(stream_openread(filename), filename);
	sound->source = 0;
	sound->dataLeft = 0;
	sound->state = SOUND_STOPPED;
	return sound;
}

MSOUND_EXPORT(void) streaming_sound_destroy(streaming_sound_t sound)
{
	
}
