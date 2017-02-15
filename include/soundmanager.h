#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

#ifdef _WIN32
#define MSOUND_EXPORT(x) __stdcall x __declspec(dllexport) 
#else
#define MSOUND_EXPORT(x) x
#endif

typedef struct soundmanager_internal *soundmanager_t;
typedef struct sound_stream *streaming_sound_t;
typedef struct sound_internal *sound_t;


MSOUND_EXPORT(soundmanager_t) soundmanager_new();
MSOUND_EXPORT(void) soundmanager_playsound(soundmanager_t mgr, sound_t sound);
MSOUND_EXPORT(void) soundmanager_playstreaming(soundmanager_t mgr, streaming_sound_t sound);
MSOUND_EXPORT(void) soundmanager_pausestreaming(soundmanager_t mgr, streaming_sound_t sound);
MSOUND_EXPORT(void) soundmanager_stopstreaming(soundmanager_t mgr, streaming_sound_t sound);

#define SOUND_PLAYING 1
#define SOUND_PAUSED 2
#define SOUND_FADINGIN 3
#define SOUND_FADINGOUT 4
#define SOUND_STOPPED 0
MSOUND_EXPORT(int) soundmanager_querystreaming(soundmanager_t mgr, streaming_sound_t sound);

MSOUND_EXPORT(void) soundmanager_destroy(soundmanager_t mgr);

MSOUND_EXPORT(sound_t) sound_fromfile(const char *filename);
MSOUND_EXPORT(streaming_sound_t) streaming_sound_fromfile(const char *filename);

MSOUND_EXPORT(void) sound_destroy(sound_t sound);
MSOUND_EXPORT(void) streaming_sound_destroy(streaming_sound_t sound);
#endif