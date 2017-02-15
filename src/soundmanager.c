#include "soundmanager.h"
#include "soundmanager_internal.h"
#include "uintstack.h"
#include <assert.h>
#include <stdint.h>

#define MAX_FUNCQUEUE 1024
#define MAX_BUFFERS 512
#define MAX_SOURCES 16

struct soundmanager_internal {
	ALCdevice *deviceAL;
	ALCcontext *contextAL;
	uintstack_t *buffers;
	uintstack_t *sources;
	int activeSounds[MAX_SOURCES];
	streaming_sound_t activeStreamers[MAX_SOURCES];
	thread_t mainthread;
	mutex_t mutex;
	volatile int running;
	volatile int thread_running;
	void (*functionQueue[MAX_FUNCQUEUE])(soundmanager_t,void*);
	void *functionDatas[MAX_FUNCQUEUE];
	int functionCurrent;
};

static int soundmanager_pushfunc(soundmanager_t mgr, void (*function)(soundmanager_t,void*), void *userData)
{
	thread_mutex_lock(&mgr->mutex);
	if((mgr->functionCurrent + 1) >= MAX_FUNCQUEUE) {
		thread_mutex_unlock(&mgr->mutex);
		return 0;
	}
	mgr->functionCurrent++;
	mgr->functionQueue[mgr->functionCurrent] = function;
	mgr->functionDatas[mgr->functionCurrent] = userData;
	thread_mutex_unlock(&mgr->mutex);
	return 1;
}

void *soundmanager_updatethread(void *arg);

MSOUND_EXPORT(soundmanager_t) soundmanager_new()
{
	soundmanager_t mgr = (soundmanager_t)malloc(sizeof(struct soundmanager_internal));
	mgr->deviceAL = alcOpenDevice(NULL);
	if(mgr->deviceAL == NULL)
	{
		LOG_ERROR("Failed to get OpenAL device.");
		free(mgr);
		return 0;
	}
	mgr->contextAL = alcCreateContext(mgr->deviceAL, NULL);
	AL_CHECK(alcMakeContextCurrent(mgr->contextAL));
	mgr->buffers = uintstack_new(MAX_BUFFERS);
	mgr->sources = uintstack_new(MAX_SOURCES);

	for(int i = 0; i < MAX_BUFFERS; i++)
	{
		uint32_t id;
		AL_CHECK(alGenBuffers((ALuint)1, &id));
		assert(uintstack_push(mgr->buffers, id));
	}

	for(int i = 0; i < MAX_SOURCES; i++)
	{
		uint32_t id;
		AL_CHECK(alGenSources((ALuint)1, &id));
		assert(uintstack_push(mgr->sources, id));
		mgr->activeSounds[i] = -1;
	}

	mgr->running = 1;
	mgr->thread_running = 1;
	mgr->functionCurrent = -1;
	thread_mutex_init(&mgr->mutex);
	thread_create(&mgr->mainthread, &soundmanager_updatethread, (void*)mgr);
	return mgr;
}

int buffer_data(streaming_sound_t src, int buf)
{
	unsigned char *bytes = (unsigned char*)malloc(src->blockSize);
	size_t ret = src->stream->read(bytes, 1, src->blockSize, src->stream);
	if(ret == 0)
		return 0;
	AL_CHECK(alBufferData(buf, src->format, (void*)bytes, (int)ret, src->frequency));
	free(bytes);
	return 1;
}

void *soundmanager_updatethread(void *arg)
{
	soundmanager_t mgr = (soundmanager_t)arg;
	while(mgr->running) {
		// process functions
		thread_mutex_lock(&mgr->mutex);
		while(mgr->functionCurrent >= 0) {
			mgr->functionQueue[mgr->functionCurrent](mgr, mgr->functionDatas[mgr->functionCurrent]);
			mgr->functionCurrent--;
		}
		thread_mutex_unlock(&mgr->mutex);
		// update
		// check active sounds
		for(int i = 0; i < MAX_SOURCES; i++) {
			if(mgr->activeSounds[i] == -1) // no sound here
				continue;
			// Check if playing, if not - add to free sources queue
			int val;
			alGetSourcei(mgr->activeSounds[i], AL_SOURCE_STATE, &val);
			if(val != AL_PLAYING) {
				uintstack_push(mgr->sources, mgr->activeSounds[i]);
				mgr->activeSounds[i] = -1;
				printf("sound completed - source pushed\n");
			}
		}
		// check streamers
		for(int i = 0; i < MAX_SOURCES; i++) {
			if(!mgr->activeStreamers[i])
				continue;
			streaming_sound_t stream = mgr->activeStreamers[i];
			if(stream->dataLeft) {
				int processed;
				AL_CHECK(alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &processed));
				for(int i = 0; i < processed; i++) {
					ALuint which;
					AL_CHECK(alSourceUnqueueBuffers(stream->source, 1, &which));
					if(buffer_data(stream, which)) {
						AL_CHECK(alSourceQueueBuffers(stream->source, 1, &which));
					} else {
						stream->dataLeft = 0;
						printf("no data - returning buffer\n");
						uintstack_push(mgr->buffers, which);
						break;
					}
				}
			}
			// Add buffers
			// Make sure source is playing
			int val;
			alGetSourcei(stream->source, AL_SOURCE_STATE, &val);
			if(val != AL_PLAYING && val != AL_PAUSED) {
				if(stream->dataLeft) {
					printf("reset source \n");
					alSourcePlay(stream->source);
				} else {
					stream->state = SOUND_STOPPED;
					uintstack_push(mgr->sources, stream->source);
					int processed = 0;
					int queued = 0;
					AL_CHECK(alSourceStop(stream->source));
					AL_CHECK(alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &processed));
					for(int i = 0; i < processed; i++) {
						ALuint buf;
						AL_CHECK(alSourceUnqueueBuffers(stream->source, 1, &buf));
						uintstack_push(mgr->buffers, buf);
					}
					stream->source = 0;
					stream->stream->seek(stream->stream,0,SEEK_SET);
					mgr->activeStreamers[i] = NULL;
				}
			}
		}
		/* sleep - reduce CPU usage */
		thread_sleep(1);
	}
	mgr->thread_running = 0;
	printf("Thread shutdown\n");
	return NULL;
}

MSOUND_EXPORT(void) soundmanager_destroy(soundmanager_t mgr)
{
	printf("Send thread shutdown request\n");
	mgr->running = 0;
	while(mgr->thread_running) ;
	printf("Shutdown\n");
	alcMakeContextCurrent(NULL);
	alcDestroyContext(mgr->contextAL);
	alcCloseDevice(mgr->deviceAL);
	thread_mutex_destroy(&mgr->mutex);
	free(mgr->sources);
	free(mgr->buffers);
	free(mgr);
}

void soundmanager_playfunc(soundmanager_t mgr, void *data)
{
	sound_t sound = (void*)data;
	uint32_t buffer, source;
	if(!uintstack_pop(mgr->buffers, &buffer)) {
		LOG_ERROR("play sound: no free buffers\n");
		return;
	}
	if(!uintstack_pop(mgr->sources, &source)) {
		LOG_ERROR("play sound: no free sources\n");
		return;
	}
	/* Add sound to list of active sounds */
	for(int i = 0; i < MAX_SOURCES; i++) {
		if(mgr->activeSounds[i] == -1) {
			mgr->activeSounds[i] = (int)source;
			break;
		}
	}
	/* Play sound */
	AL_CHECK(alBufferData(buffer, sound->format, (void*)sound->data, sound->dataSize, sound->frequency));
	AL_CHECK(alSourceQueueBuffers(source, 1, &buffer));
	AL_CHECK(alSourcePlay(source));
}

MSOUND_EXPORT(void) soundmanager_playsound(soundmanager_t mgr, sound_t sound)
{
	if(!soundmanager_pushfunc(mgr, soundmanager_playfunc, (void*)sound)) {
		LOG_ERROR("Function queue overflow");
	}
}

void soundmanager_startstreamfunc(soundmanager_t mgr, void *data)
{
	streaming_sound_t sound = (streaming_sound_t)data;
	if(sound->source) {
		AL_CHECK(alSourcePlay(sound->source));
		return;
	}

	uint32_t source;
	if(!uintstack_pop(mgr->sources, &source)) {
		LOG_ERROR("play streamer: no free sources\n");
		return;
	}
	uint32_t buf0;
	uint32_t buf1;
	uint32_t buf2;
	if(!uintstack_pop(mgr->buffers, &buf0)) {
		LOG_ERROR("play streamer: not enough buffers\n");
		return;
	}
	if(!uintstack_pop(mgr->buffers, &buf1)) {
		LOG_ERROR("play streamer: not enough buffers\n");
		uintstack_push(mgr->buffers, buf0);
		return;
	}
	if(!uintstack_pop(mgr->buffers, &buf2)) {
		LOG_ERROR("play streamer: not enough buffers\n");
		uintstack_push(mgr->buffers, buf0);
		uintstack_push(mgr->buffers, buf1);
		return;
	}
	sound->source = source;
	//Pre-buffer
	buffer_data(sound, buf0);
	AL_CHECK(alSourceQueueBuffers(source, 1, &buf0));
	if(buffer_data(sound, buf1)) {
		AL_CHECK(alSourceQueueBuffers(source, 1, &buf1));
		if(buffer_data(sound,buf2)) {
			AL_CHECK(alSourceQueueBuffers(source, 1, &buf2));
			sound->dataLeft = 1;
		} else {
			sound->dataLeft = 0;
			uintstack_push(mgr->buffers, buf2);
		}
	} else {
		sound->dataLeft = 0;
		uintstack_push(mgr->buffers, buf1);
	}
	printf("data left: %d\n", sound->dataLeft);
	// Add source
	for(int i = 0; i < MAX_SOURCES; i++) {
		if(!mgr->activeStreamers[i]) {
			mgr->activeStreamers[i] = sound;
			break;
		}
	}
	// Play source
	AL_CHECK(alSourcePlay(source));
	printf("Stream started\n");
}

MSOUND_EXPORT(void) soundmanager_playstreaming(soundmanager_t mgr, streaming_sound_t sound)
{
	if(!soundmanager_pushfunc(mgr, soundmanager_startstreamfunc, (void*)sound)) {
		LOG_ERROR("Function queue overflow");
	}
}

void soundmanager_pausestreamfunc(soundmanager_t mgr, void *data)
{
	streaming_sound_t sound = (streaming_sound_t)data;
	if(sound->source) {
		AL_CHECK(alSourcePause(sound->source));
	}
}

MSOUND_EXPORT(void) soundmanager_pausestreaming(soundmanager_t mgr, streaming_sound_t sound)
{
	sound->state = SOUND_PAUSED;
	if(!soundmanager_pushfunc(mgr,soundmanager_pausestreamfunc, (void*)sound)) {
		LOG_ERROR("Function queue overflow");
	}
}

void soundmanager_stopstreamfunc(soundmanager_t mgr, void *data)
{
	streaming_sound_t sound = (streaming_sound_t)data;
	if(sound->source) {
		sound->dataLeft = 0;
		AL_CHECK(alSourceStop(sound->source));
	}
}

MSOUND_EXPORT(void) soundmanager_stopstreaming(soundmanager_t mgr, streaming_sound_t sound)
{
	sound->state = SOUND_STOPPED;
	if(!soundmanager_pushfunc(mgr, soundmanager_stopstreamfunc, (void*)sound)) {
		LOG_ERROR("Function queue overflow");
	}
}

MSOUND_EXPORT(int) soundmanager_querystreaming(soundmanager_t mgr, streaming_sound_t sound)
{
	return sound->state;
}

const char *GetOpenALErrorString(int errID)
{
	if (errID == AL_NO_ERROR) return "";
	if (errID == AL_INVALID_NAME) return "Invalid name.";
	if (errID == AL_INVALID_ENUM) return "Invalid enum.";
	if (errID == AL_INVALID_VALUE) return "Invalid value.";
	if (errID == AL_INVALID_OPERATION) return "Invalid operation.";
	if (errID == AL_OUT_OF_MEMORY) return "Out of memory.";

	return "Unknown error.";
}

void CheckOpenALError(const char* stmt, const char* fname, int line)
{
	ALenum err = alGetError();
	if(err != AL_NO_ERROR)
	{
		LOG_ERROR_F("OpenAL error %08x, (%s) at %s:%i - for %s", err, stmt, fname, line, GetOpenALErrorString(err));
	}
}