#include "../soundmanager_internal.h"
#include "../loaders.h"
#ifdef CONFIG_ENABLE_OGG
#include "stb_vorbis.c"


#define OGG_BUFFER_SIZE 32768

const char *stb_vorbis_strerror(int err)
{
	switch(err) {
		case VORBIS__no_error:
			return "No error";
		case VORBIS_need_more_data:
			return "Need more data";
		case VORBIS_invalid_api_mixing:
			return "Invalid api mixing";
		case VORBIS_outofmem:
			return "Out of memory";
		case VORBIS_feature_not_supported:
			return "Feature not supported";
		case VORBIS_too_many_channels:
			return "Too many channels";
		case VORBIS_file_open_failure:
			return "File open failure"; //Won't happen
		case VORBIS_seek_without_length:
			return "Tried to seek with unknown length";
		case VORBIS_unexpected_eof:
			return "Unexpected end of file";
		case VORBIS_seek_invalid:
			return "Invalid seek";
		case VORBIS_invalid_setup:
			return "Invalid setup";
		case VORBIS_invalid_stream:
			return "Invalid stream";
		case VORBIS_missing_capture_pattern:
			return "Missing capture pattern";
		case VORBIS_invalid_stream_structure_version:
			return "Invalid stream structure version";
		case VORBIS_continued_packet_flag_invalid:
			return "Continued packet flag invalid";
		case VORBIS_incorrect_stream_serial_number:
			return "Incorrect stream serial number";
		case VORBIS_invalid_first_page:
			return "Invalid first page";
		case VORBIS_bad_packet_type:
			return "Bad packet type";
		case VORBIS_cant_find_last_page:
			return "Can't find last page";
		case VORBIS_seek_failed:
			return "Seek failed";
		default:
			return "Unknown";
	}
}


typedef struct {
	stb_vorbis *vorbis;
	stream_t baseStream;
	int channels;
} ogg_userdata_t;

size_t ogg_read(void* ptr, size_t size, size_t count, stream_t stream)
{
	ogg_userdata_t *userdata = (ogg_userdata_t*)stream->userData;
	size_t sz_bytes = size * count;
	if((sz_bytes % 2) != 0) {
		LOG_ERROR("ogg_read: buffer size must be a multiple of sizeof(short)");
		return 0;
	}
	int num_shorts = sz_bytes / sizeof(short);
	int res = stb_vorbis_get_samples_short_interleaved(userdata->vorbis, userdata->channels, (short*)ptr, num_shorts);
	return res * sizeof(short) * userdata->channels;
}

int ogg_seek(stream_t stream, long int offset, int origin)
{
	if(origin != SEEK_SET || offset != 0) {
		LOG_ERROR("ogg_seek: only can seek to SEEK_SET 0");
		return -1;
	}
	ogg_userdata_t *userdata = (ogg_userdata_t*)stream->userData;
	stb_vorbis_seek(userdata->vorbis, 0);
	return 0;
}

long int ogg_tell(stream_t stream)
{
	LOG_ERROR("ogg_tell: unsupported");
	return -1;
}

void ogg_close(stream_t stream)
{
	ogg_userdata_t *userdata = (ogg_userdata_t*)stream->userData;
	stb_vorbis_close(userdata->vorbis);
	userdata->baseStream->close(userdata->baseStream);
	free(userdata);
	free(stream);
}


streaming_sound_t ogg_getstream(stream_t stream, const char *filename)
{
	int err;
	stb_vorbis *vorbis = stb_vorbis_open_file(stream, 0, &err, NULL);
	if(!vorbis) {
		stream->seek(stream, 0, SEEK_SET);
		return flac_getstream(stream, filename);
	}
	stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	ogg_userdata_t *userdata = (ogg_userdata_t*)malloc(sizeof(ogg_userdata_t));
	userdata->channels = info.channels;
	userdata->vorbis = vorbis;
	userdata->baseStream = stream;
	stream_t data = stream_alloc();
	data->read = &ogg_read;
	data->seek = &ogg_seek;
	data->tell = &ogg_tell;
	data->close = &ogg_close;
	data->userData = userdata;

	streaming_sound_t retsound = (streaming_sound_t)malloc(sizeof(struct sound_stream));
	retsound->frequency = info.sample_rate;
	retsound->stream = data;
	retsound->dataSize = -1;
	retsound->blockSize = OGG_BUFFER_SIZE;
	if(info.channels == 2) {
		retsound->format = AL_FORMAT_STEREO16;
	} else {
		retsound->format = AL_FORMAT_MONO16;
	}
	return retsound;
}

#endif