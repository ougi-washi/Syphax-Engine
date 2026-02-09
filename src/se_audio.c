// Syphax-Engine - Ougi Washi

#include "se_audio.h"

#include "miniaudio/miniaudio.h"

#include "se_defines.h"
#include "syphax/s_array.h"
#include "syphax/s_files.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SE_AUDIO_CAPTURE_ANALYSIS_FRAMES 2048
#define SE_AUDIO_LOW_CUTOFF_HZ 250.0f
#define SE_AUDIO_MID_CUTOFF_HZ 4000.0f

typedef struct se_audio_clip se_audio_clip;
typedef struct se_audio_stream se_audio_stream;
typedef struct se_audio_capture se_audio_capture;

typedef s_array(se_audio_clip, se_audio_clip_array);
typedef s_array(se_audio_stream, se_audio_stream_array);
typedef s_array(se_audio_capture, se_audio_capture_array);

struct se_audio_engine {
	ma_context context;
	ma_engine engine;
	se_audio_config config;
	se_audio_clip_array clips;
	se_audio_stream_array streams;
	se_audio_capture_array captures;
	f32 bus_volumes[SE_AUDIO_BUS_COUNT];
	b8 initialized : 1;
};

struct se_audio_clip {
	se_audio_engine* owner;
	ma_sound sound;
	char path[SE_MAX_PATH_LENGTH];
	f32 base_volume;
	se_audio_bus bus;
	b8 in_use : 1;
	f32* analysis_pcm;
	ma_uint64 analysis_frame_count;
	u32 analysis_channels;
	u32 analysis_sample_rate;
};

struct se_audio_stream {
	se_audio_engine* owner;
	ma_sound sound;
	f32 base_volume;
	se_audio_bus bus;
	b8 in_use : 1;
};

struct se_audio_capture {
	se_audio_engine* owner;
	ma_device device;
	ma_pcm_rb ring_buffer;
	se_audio_capture_config config;
	ma_device_id device_id;
	b8 active : 1;
	f32* analysis_buffer;
	sz analysis_capacity_frames;
	sz analysis_frames_valid;
	sz analysis_sample_write;
	pthread_mutex_t analysis_mutex;
};

static const se_audio_config SE_AUDIO_DEFAULT_CONFIG = {
	.sample_rate = 48000,
	.channels = 2,
	.max_clips = 32,
	.max_streams = 8,
	.max_captures = 2,
	.enable_capture = true
};

static const se_audio_play_params SE_AUDIO_DEFAULT_PLAY_PARAMS = {
	.volume = 1.0f,
	.pan = 0.0f,
	.pitch = 1.0f,
	.looping = false,
	.bus = SE_AUDIO_BUS_MASTER
};

static se_audio_clip* se_audio_acquire_clip_slot(se_audio_engine* engine);
static se_audio_stream* se_audio_acquire_stream_slot(se_audio_engine* engine);
static se_audio_capture* se_audio_acquire_capture_slot(se_audio_engine* engine);
static void se_audio_clip_refresh_volume(se_audio_clip* clip);
static void se_audio_stream_refresh_volume(se_audio_stream* stream);
static const se_audio_play_params* se_audio_resolve_play_params(const se_audio_play_params* params);
static char* se_audio_resolve_resource_path(const char* relative_path, char* out_path, sz path_size);
static se_audio_bus se_audio_sanitize_bus(se_audio_bus bus);
static float se_audio_clamp_volume(float value);
static float se_audio_clamp_pan(float value);
static float se_audio_clamp_pitch(float value);
static void se_audio_capture_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
static void se_audio_capture_push_analysis(se_audio_capture* capture, const f32* frames, ma_uint32 frame_count);
static b8 se_audio_load_analysis_data(const char* full_path, se_audio_clip* clip);
static void se_audio_free_analysis_data(se_audio_clip* clip);
static void se_audio_compute_band_levels(const f32* samples, sz frame_count, u32 channels, u32 sample_rate, se_audio_band_levels* out_levels);
static f32 se_audio_lowpass_alpha(f32 cutoff_hz, u32 sample_rate);

se_audio_engine* se_audio_init(const se_audio_config* config) {
	se_audio_config resolved_config = SE_AUDIO_DEFAULT_CONFIG;
	if (config) {
		resolved_config = *config;
		if (resolved_config.sample_rate == 0) {
			resolved_config.sample_rate = SE_AUDIO_DEFAULT_CONFIG.sample_rate;
		}
		if (resolved_config.channels == 0) {
			resolved_config.channels = SE_AUDIO_DEFAULT_CONFIG.channels;
		}
		if (resolved_config.max_clips == 0) {
			resolved_config.max_clips = SE_AUDIO_DEFAULT_CONFIG.max_clips;
		}
		if (resolved_config.max_streams == 0) {
			resolved_config.max_streams = SE_AUDIO_DEFAULT_CONFIG.max_streams;
		}
		if (resolved_config.max_captures == 0) {
			resolved_config.max_captures = SE_AUDIO_DEFAULT_CONFIG.max_captures;
		}
	}

	se_audio_engine* engine = malloc(sizeof(se_audio_engine));
	if (!engine) {
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(engine, 0, sizeof(se_audio_engine));
	engine->config = resolved_config;
	for (u32 i = 0; i < SE_AUDIO_BUS_COUNT; i++) {
		engine->bus_volumes[i] = 1.0f;
	}

	ma_result ma_res = ma_context_init(NULL, 0, NULL, &engine->context);
	if (ma_res != MA_SUCCESS) {
		free(engine);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	ma_engine_config engine_config = ma_engine_config_init();
	engine_config.pContext = &engine->context;
	engine_config.sampleRate = engine->config.sample_rate;
	engine_config.channels = engine->config.channels;
	engine_config.noAutoStart = MA_FALSE;
	ma_res = ma_engine_init(&engine_config, &engine->engine);
	if (ma_res != MA_SUCCESS) {
		ma_context_uninit(&engine->context);
		free(engine);
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	s_array_init(&engine->clips, engine->config.max_clips);
	s_array_init(&engine->streams, engine->config.max_streams);
	s_array_init(&engine->captures, engine->config.max_captures);
	engine->initialized = true;
	se_set_last_error(SE_RESULT_OK);
	return engine;
}

void se_audio_shutdown(se_audio_engine* engine) {
	if (!engine) {
		return;
	}
	if (!engine->initialized) {
		return;
	}

	se_audio_stop_all(engine);

	s_foreach(&engine->clips, clip_index) {
		se_audio_clip* clip = s_array_get(&engine->clips, clip_index);
		if (clip->in_use) {
			ma_sound_uninit(&clip->sound);
			se_audio_free_analysis_data(clip);
		}
	}
	s_array_clear(&engine->clips);

	s_foreach(&engine->streams, stream_index) {
		se_audio_stream* stream = s_array_get(&engine->streams, stream_index);
		if (stream->in_use) {
			ma_sound_uninit(&stream->sound);
		}
	}
	s_array_clear(&engine->streams);

	s_foreach(&engine->captures, capture_index) {
		se_audio_capture* capture = s_array_get(&engine->captures, capture_index);
		if (capture->active) {
			ma_device_uninit(&capture->device);
			ma_pcm_rb_uninit(&capture->ring_buffer);
			if (capture->analysis_buffer) {
				free(capture->analysis_buffer);
				capture->analysis_buffer = NULL;
			}
			pthread_mutex_destroy(&capture->analysis_mutex);
		}
	}
	s_array_clear(&engine->captures);

	ma_engine_uninit(&engine->engine);
	ma_context_uninit(&engine->context);
	engine->initialized = false;
	free(engine);
}

void se_audio_update(se_audio_engine* engine) {
	if (!engine || !engine->initialized) {
		return;
	}
	/* Currently no per-frame tasks. Placeholder for streaming/capture book-keeping. */
}

void se_audio_stop_all(se_audio_engine* engine) {
	if (!engine || !engine->initialized) {
		return;
	}
	s_foreach(&engine->clips, clip_index) {
		se_audio_clip* clip = s_array_get(&engine->clips, clip_index);
		if (clip->in_use) {
			ma_sound_stop(&clip->sound);
		}
	}
	s_foreach(&engine->streams, stream_index) {
		se_audio_stream* stream = s_array_get(&engine->streams, stream_index);
		if (stream->in_use) {
			ma_sound_stop(&stream->sound);
		}
	}
}

void se_audio_bus_set_volume(se_audio_engine* engine, se_audio_bus bus, f32 volume) {
	if (!engine || !engine->initialized) {
		return;
	}
	se_audio_bus sanitized = se_audio_sanitize_bus(bus);
	engine->bus_volumes[sanitized] = se_audio_clamp_volume(volume);

	s_foreach(&engine->clips, clip_index) {
		se_audio_clip* clip = s_array_get(&engine->clips, clip_index);
		if (clip->in_use && clip->bus == sanitized) {
			se_audio_clip_refresh_volume(clip);
		}
	}
	s_foreach(&engine->streams, stream_index) {
		se_audio_stream* stream = s_array_get(&engine->streams, stream_index);
		if (stream->in_use && stream->bus == sanitized) {
			se_audio_stream_refresh_volume(stream);
		}
	}
}

f32 se_audio_bus_get_volume(se_audio_engine* engine, se_audio_bus bus) {
	if (!engine || !engine->initialized) {
		return 0.0f;
	}
	return engine->bus_volumes[se_audio_sanitize_bus(bus)];
}

se_audio_clip* se_audio_clip_load(se_audio_engine* engine, const char* relative_path) {
	if (!engine || !engine->initialized || !relative_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_audio_clip* clip = se_audio_acquire_clip_slot(engine);
	if (!clip) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_audio_resolve_resource_path(relative_path, full_path, sizeof(full_path))) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	ma_result ma_res = ma_sound_init_from_file(&engine->engine, full_path, MA_SOUND_FLAG_DECODE, NULL, NULL, &clip->sound);
	if (ma_res != MA_SUCCESS) {
		memset(clip, 0, sizeof(se_audio_clip));
		clip->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	clip->in_use = true;
	clip->owner = engine;
	clip->bus = SE_AUDIO_BUS_MASTER;
	clip->base_volume = 1.0f;
	strncpy(clip->path, relative_path, sizeof(clip->path) - 1);
	se_audio_load_analysis_data(full_path, clip);
	se_set_last_error(SE_RESULT_OK);
	return clip;
}

void se_audio_clip_unload(se_audio_engine* engine, se_audio_clip* clip) {
	if (!engine || !clip || clip->owner != engine) {
		return;
	}
	ma_sound_stop(&clip->sound);
	ma_sound_uninit(&clip->sound);
	se_audio_free_analysis_data(clip);
	memset(clip, 0, sizeof(se_audio_clip));
	clip->owner = engine;
}

b8 se_audio_clip_play(se_audio_clip* clip, const se_audio_play_params* params) {
	if (!clip || !clip->in_use || !clip->owner) {
		return false;
	}
	const se_audio_play_params* resolved = se_audio_resolve_play_params(params);
	clip->bus = se_audio_sanitize_bus(resolved->bus);
	clip->base_volume = se_audio_clamp_volume(resolved->volume);
	ma_sound_set_pan(&clip->sound, se_audio_clamp_pan(resolved->pan));
	ma_sound_set_pitch(&clip->sound, se_audio_clamp_pitch(resolved->pitch));
	ma_sound_set_looping(&clip->sound, resolved->looping);
	ma_sound_stop(&clip->sound);
	ma_sound_seek_to_pcm_frame(&clip->sound, 0);
	se_audio_clip_refresh_volume(clip);
	ma_result result = ma_sound_start(&clip->sound);
	return result == MA_SUCCESS;
}

void se_audio_clip_stop(se_audio_clip* clip) {
	if (!clip || !clip->in_use) {
		return;
	}
	ma_sound_stop(&clip->sound);
}

void se_audio_clip_set_volume(se_audio_clip* clip, f32 volume) {
	if (!clip || !clip->in_use) {
		return;
	}
	clip->base_volume = se_audio_clamp_volume(volume);
	se_audio_clip_refresh_volume(clip);
}

void se_audio_clip_set_pan(se_audio_clip* clip, f32 pan) {
	if (!clip || !clip->in_use) {
		return;
	}
	ma_sound_set_pan(&clip->sound, se_audio_clamp_pan(pan));
}

b8 se_audio_clip_is_playing(const se_audio_clip* clip) {
	if (!clip || !clip->in_use) {
		return false;
	}
	return ma_sound_is_playing(&clip->sound);
}

se_audio_stream* se_audio_stream_open(se_audio_engine* engine, const char* relative_path, const se_audio_play_params* params) {
	if (!engine || !engine->initialized || !relative_path) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_audio_stream* stream = se_audio_acquire_stream_slot(engine);
	if (!stream) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}

	char full_path[SE_MAX_PATH_LENGTH] = {0};
	if (!se_audio_resolve_resource_path(relative_path, full_path, sizeof(full_path))) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}

	ma_result ma_res = ma_sound_init_from_file(&engine->engine, full_path, MA_SOUND_FLAG_STREAM, NULL, NULL, &stream->sound);
	if (ma_res != MA_SUCCESS) {
		memset(stream, 0, sizeof(se_audio_stream));
		stream->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	stream->in_use = true;
	stream->owner = engine;
	const se_audio_play_params* resolved = se_audio_resolve_play_params(params);
	stream->bus = se_audio_sanitize_bus(resolved->bus);
	stream->base_volume = se_audio_clamp_volume(resolved->volume);
	ma_sound_set_pan(&stream->sound, se_audio_clamp_pan(resolved->pan));
	ma_sound_set_pitch(&stream->sound, se_audio_clamp_pitch(resolved->pitch));
	ma_sound_set_looping(&stream->sound, resolved->looping);
	se_audio_stream_refresh_volume(stream);
	ma_sound_start(&stream->sound);
	se_set_last_error(SE_RESULT_OK);
	return stream;
}

void se_audio_stream_close(se_audio_stream* stream) {
	if (!stream || !stream->in_use) {
		return;
	}
	se_audio_engine* owner = stream->owner;
	ma_sound_stop(&stream->sound);
	ma_sound_uninit(&stream->sound);
	memset(stream, 0, sizeof(se_audio_stream));
	stream->owner = owner;
}

void se_audio_stream_set_volume(se_audio_stream* stream, f32 volume) {
	if (!stream || !stream->in_use) {
		return;
	}
	stream->base_volume = se_audio_clamp_volume(volume);
	se_audio_stream_refresh_volume(stream);
}

void se_audio_stream_set_looping(se_audio_stream* stream, b8 looping) {
	if (!stream || !stream->in_use) {
		return;
	}
	ma_sound_set_looping(&stream->sound, looping);
}

void se_audio_stream_seek(se_audio_stream* stream, f64 seconds) {
	if (!stream || !stream->in_use) {
		return;
	}
	ma_sound_seek_to_second(&stream->sound, (float)seconds);
}

f64 se_audio_stream_get_cursor(const se_audio_stream* stream) {
	if (!stream || !stream->in_use) {
		return 0.0;
	}
	float cursor = 0.0f;
	ma_sound_get_cursor_in_seconds(&stream->sound, &cursor);
	return (f64)cursor;
}

u32 se_audio_capture_list_devices(se_audio_engine* engine, se_audio_device_info* out_devices, u32 max_devices) {
	if (!engine || !engine->initialized || !engine->config.enable_capture) {
		return 0;
	}
	ma_device_info* capture_infos = NULL;
	ma_uint32 capture_count = 0;
	ma_result result = ma_context_get_devices(&engine->context, NULL, NULL, &capture_infos, &capture_count);
	if (result != MA_SUCCESS) {
		fprintf(stderr, "se_audio_capture_list_devices :: failed to enumerate devices (%d)\n", result);
		return 0;
	}
	if (!out_devices || max_devices == 0) {
		return (u32)capture_count;
	}
	u32 copy_count = (capture_count < max_devices) ? (u32)capture_count : max_devices;
	for (u32 i = 0; i < copy_count; i++) {
		ma_device_info* src = capture_infos + i;
		se_audio_device_info* dst = out_devices + i;
		memset(dst, 0, sizeof(se_audio_device_info));
		strncpy(dst->name, src->name, SE_AUDIO_DEVICE_NAME_MAX - 1);
		dst->index = i;
		dst->is_default = (src->isDefault != 0);
	}
	return copy_count;
}

se_audio_capture* se_audio_capture_start(se_audio_engine* engine, const se_audio_capture_config* config) {
	if (!engine || !engine->initialized || !engine->config.enable_capture) {
		se_set_last_error(SE_RESULT_INVALID_ARGUMENT);
		return NULL;
	}
	se_audio_capture* capture = se_audio_acquire_capture_slot(engine);
	if (!capture) {
		se_set_last_error(SE_RESULT_CAPACITY_EXCEEDED);
		return NULL;
	}

	se_audio_capture_config resolved = {0};
	if (config) {
		resolved = *config;
	}
	if (resolved.sample_rate == 0) {
		resolved.sample_rate = engine->config.sample_rate;
	}
	if (resolved.channels == 0) {
		resolved.channels = engine->config.channels;
	}
	if (resolved.frames_per_buffer == 0) {
		resolved.frames_per_buffer = 1024;
	}

	ma_device_info* capture_infos = NULL;
	ma_uint32 capture_count = 0;
	ma_result ma_res = ma_context_get_devices(&engine->context, NULL, NULL, &capture_infos, &capture_count);
	if (ma_res != MA_SUCCESS) {
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}
	ma_device_id* selected_id = NULL;
	if (resolved.device_index < capture_count) {
		capture->device_id = capture_infos[resolved.device_index].id;
		selected_id = &capture->device_id;
	}

	ma_result rb_result = ma_pcm_rb_init(ma_format_f32, resolved.channels, resolved.frames_per_buffer * 8, NULL, NULL, &capture->ring_buffer);
	if (rb_result != MA_SUCCESS) {
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	capture->analysis_capacity_frames = SE_AUDIO_CAPTURE_ANALYSIS_FRAMES;
	sz analysis_samples = capture->analysis_capacity_frames * resolved.channels;
	capture->analysis_buffer = malloc(sizeof(f32) * analysis_samples);
	if (!capture->analysis_buffer) {
		ma_pcm_rb_uninit(&capture->ring_buffer);
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_OUT_OF_MEMORY);
		return NULL;
	}
	memset(capture->analysis_buffer, 0, sizeof(f32) * analysis_samples);
	capture->analysis_frames_valid = 0;
	capture->analysis_sample_write = 0;
	if (pthread_mutex_init(&capture->analysis_mutex, NULL) != 0) {
		free(capture->analysis_buffer);
		ma_pcm_rb_uninit(&capture->ring_buffer);
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	ma_device_config device_config = ma_device_config_init(ma_device_type_capture);
	device_config.capture.pDeviceID = selected_id;
	device_config.capture.format = ma_format_f32;
	device_config.capture.channels = resolved.channels;
	device_config.sampleRate = resolved.sample_rate;
	device_config.pUserData = capture;
	device_config.dataCallback = se_audio_capture_data_callback;
	ma_res = ma_device_init(&engine->context, &device_config, &capture->device);
	if (ma_res != MA_SUCCESS) {
		ma_pcm_rb_uninit(&capture->ring_buffer);
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	ma_res = ma_device_start(&capture->device);
	if (ma_res != MA_SUCCESS) {
		ma_device_uninit(&capture->device);
		ma_pcm_rb_uninit(&capture->ring_buffer);
		pthread_mutex_destroy(&capture->analysis_mutex);
		free(capture->analysis_buffer);
		memset(capture, 0, sizeof(se_audio_capture));
		capture->owner = engine;
		se_set_last_error(SE_RESULT_BACKEND_FAILURE);
		return NULL;
	}

	capture->config = resolved;
	capture->owner = engine;
	capture->active = true;
	se_set_last_error(SE_RESULT_OK);
	return capture;
}

void se_audio_capture_stop(se_audio_capture* capture) {
	if (!capture || !capture->active) {
		return;
	}
	se_audio_engine* owner = capture->owner;
	ma_device_uninit(&capture->device);
	ma_pcm_rb_uninit(&capture->ring_buffer);
	if (capture->analysis_buffer) {
		free(capture->analysis_buffer);
		capture->analysis_buffer = NULL;
	}
	pthread_mutex_destroy(&capture->analysis_mutex);
	memset(capture, 0, sizeof(se_audio_capture));
	capture->owner = owner;
}

sz se_audio_capture_read(se_audio_capture* capture, f32* dst_frames, sz max_frames) {
	if (!capture || !capture->active || !dst_frames || max_frames == 0) {
		return 0;
	}
	sz frames_read = 0;
	while (frames_read < max_frames) {
		ma_uint32 available = ma_pcm_rb_available_read(&capture->ring_buffer);
		if (available == 0) {
			break;
		}
		ma_uint32 wanted = (ma_uint32)((max_frames - frames_read) < available ? (max_frames - frames_read) : available);
		ma_uint32 acquired = wanted;
		void* buffer = NULL;
		if (ma_pcm_rb_acquire_read(&capture->ring_buffer, &acquired, &buffer) != MA_SUCCESS) {
			break;
		}
		if (acquired == 0 || !buffer) {
			break;
		}
		memcpy(dst_frames + frames_read * capture->config.channels, buffer, acquired * capture->config.channels * sizeof(f32));
		ma_pcm_rb_commit_read(&capture->ring_buffer, acquired);
		frames_read += acquired;
	}
	return frames_read;
}

b8 se_audio_capture_get_bands(se_audio_capture* capture, se_audio_band_levels* out_levels) {
	if (!capture || !capture->active || !capture->analysis_buffer || capture->analysis_frames_valid == 0 || !out_levels) {
		return false;
	}
	const u32 channels = capture->config.channels;
	if (channels == 0 || capture->config.sample_rate == 0) {
		return false;
	}
	sz frames = capture->analysis_frames_valid;
	sz sample_capacity = capture->analysis_capacity_frames * channels;
	sz sample_count = frames * channels;
	f32* scratch = malloc(sizeof(f32) * sample_count);
	if (!scratch) {
		return false;
	}
	pthread_mutex_lock(&capture->analysis_mutex);
	sz write_index = capture->analysis_sample_write;
	sz start = (write_index + sample_capacity - sample_count) % sample_capacity;
	for (sz i = 0; i < sample_count; i++) {
		scratch[i] = capture->analysis_buffer[(start + i) % sample_capacity];
	}
	pthread_mutex_unlock(&capture->analysis_mutex);
	se_audio_compute_band_levels(scratch, frames, channels, capture->config.sample_rate, out_levels);
	free(scratch);
	return true;
}

b8 se_audio_clip_get_bands(const se_audio_clip* clip, se_audio_band_levels* out_levels, sz max_frames) {
	if (!clip || !clip->in_use || !clip->analysis_pcm || clip->analysis_frame_count == 0 || !out_levels) {
		return false;
	}
	if (clip->analysis_channels == 0 || clip->analysis_sample_rate == 0) {
		return false;
	}
	ma_uint64 cursor_frames = 0;
	ma_result cursor_result = ma_sound_get_cursor_in_pcm_frames(&clip->sound, &cursor_frames);
	ma_uint64 total_frames = clip->analysis_frame_count;
	if (cursor_result != MA_SUCCESS || cursor_frames > total_frames) {
		cursor_frames = total_frames;
	}
	ma_uint64 window = (max_frames > 0) ? (ma_uint64)max_frames : 2048;
	if (window > total_frames) {
		window = total_frames;
	}
	ma_uint64 start = (cursor_frames > window) ? (cursor_frames - window) : 0;
	const f32* start_samples = clip->analysis_pcm + start * clip->analysis_channels;
	se_audio_compute_band_levels(start_samples, (sz)(window), clip->analysis_channels, clip->analysis_sample_rate, out_levels);
	return true;
}

b8 se_audio_buffer_analyze_bands(const f32* samples, sz frame_count, u32 channels, u32 sample_rate, se_audio_band_levels* out_levels) {
	if (!samples || !out_levels || frame_count == 0 || channels == 0 || sample_rate == 0) {
		return false;
	}
	se_audio_compute_band_levels(samples, frame_count, channels, sample_rate, out_levels);
	return true;
}

static se_audio_clip* se_audio_acquire_clip_slot(se_audio_engine* engine) {
	s_foreach(&engine->clips, i) {
		se_audio_clip* clip = s_array_get(&engine->clips, i);
		if (!clip->in_use) {
			clip->owner = engine;
			return clip;
		}
	}
	if (engine->clips.size >= engine->clips.capacity) {
		return NULL;
	}
	se_audio_clip* clip = s_array_increment(&engine->clips);
	memset(clip, 0, sizeof(se_audio_clip));
	clip->owner = engine;
	return clip;
}

static se_audio_stream* se_audio_acquire_stream_slot(se_audio_engine* engine) {
	s_foreach(&engine->streams, i) {
		se_audio_stream* stream = s_array_get(&engine->streams, i);
		if (!stream->in_use) {
			stream->owner = engine;
			return stream;
		}
	}
	if (engine->streams.size >= engine->streams.capacity) {
		return NULL;
	}
	se_audio_stream* stream = s_array_increment(&engine->streams);
	memset(stream, 0, sizeof(se_audio_stream));
	stream->owner = engine;
	return stream;
}

static se_audio_capture* se_audio_acquire_capture_slot(se_audio_engine* engine) {
	s_foreach(&engine->captures, i) {
		se_audio_capture* capture = s_array_get(&engine->captures, i);
		if (!capture->active) {
			capture->owner = engine;
			return capture;
		}
	}
	if (engine->captures.size >= engine->captures.capacity) {
		return NULL;
	}
	se_audio_capture* capture = s_array_increment(&engine->captures);
	memset(capture, 0, sizeof(se_audio_capture));
	capture->owner = engine;
	return capture;
}

static void se_audio_clip_refresh_volume(se_audio_clip* clip) {
	if (!clip || !clip->owner) {
		return;
	}
	const f32 bus_volume = clip->owner->bus_volumes[clip->bus];
	ma_sound_set_volume(&clip->sound, clip->base_volume * bus_volume);
}

static void se_audio_stream_refresh_volume(se_audio_stream* stream) {
	if (!stream || !stream->owner) {
		return;
	}
	const f32 bus_volume = stream->owner->bus_volumes[stream->bus];
	ma_sound_set_volume(&stream->sound, stream->base_volume * bus_volume);
}

static const se_audio_play_params* se_audio_resolve_play_params(const se_audio_play_params* params) {
	return params ? params : &SE_AUDIO_DEFAULT_PLAY_PARAMS;
}

static char* se_audio_resolve_resource_path(const char* relative_path, char* out_path, sz path_size) {
	if (!relative_path || !out_path || path_size == 0) {
		return NULL;
	}
	if (!s_path_join(out_path, path_size, RESOURCES_DIR, relative_path)) {
		fprintf(stderr, "se_audio :: path too long for %s\n", relative_path);
		return NULL;
	}
	return out_path;
}

static se_audio_bus se_audio_sanitize_bus(se_audio_bus bus) {
	if (bus < 0 || bus >= SE_AUDIO_BUS_COUNT) {
		return SE_AUDIO_BUS_MASTER;
	}
	return bus;
}

static float se_audio_clamp_volume(float value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 4.0f) {
		return 4.0f;
	}
	return value;
}

static float se_audio_clamp_pan(float value) {
	if (value < -1.0f) {
		return -1.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

static float se_audio_clamp_pitch(float value) {
	if (value < 0.125f) {
		return 0.125f;
	}
	if (value > 4.0f) {
		return 4.0f;
	}
	return value;
}

static void se_audio_capture_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
	(void)output;
	se_audio_capture* capture = device ? device->pUserData : NULL;
	if (!capture || !capture->active || !input) {
		return;
	}
	const ma_uint32 channels = capture->config.channels;
	const f32* source = (const f32*)input;
	ma_uint32 remaining = frame_count;
	while (remaining > 0) {
		ma_uint32 writable = ma_pcm_rb_available_write(&capture->ring_buffer);
		ma_uint32 frames_to_copy = remaining < writable ? remaining : writable;
		if (frames_to_copy > 0) {
			ma_uint32 acquired = frames_to_copy;
			void* buffer = NULL;
			if (ma_pcm_rb_acquire_write(&capture->ring_buffer, &acquired, &buffer) != MA_SUCCESS) {
				acquired = 0;
			}
			if (acquired > 0 && buffer) {
				se_audio_capture_push_analysis(capture, source, acquired);
				memcpy(buffer, source, acquired * channels * sizeof(f32));
				ma_pcm_rb_commit_write(&capture->ring_buffer, acquired);
				source += acquired * channels;
				remaining -= acquired;
				continue;
			}
		}
		/* No ring buffer space: push remaining samples to analysis and drop them to keep capture responsive. */
		if (remaining > 0) {
			se_audio_capture_push_analysis(capture, source, remaining);
			source += remaining * channels;
			remaining = 0;
		}
	}
}

static void se_audio_capture_push_analysis(se_audio_capture* capture, const f32* frames, ma_uint32 frame_count) {
	if (!capture || !capture->analysis_buffer || frame_count == 0 || !frames) {
		return;
	}
	const u32 channels = capture->config.channels;
	if (channels == 0) {
		return;
	}
	sz sample_capacity = capture->analysis_capacity_frames * channels;
	if (sample_capacity == 0) {
		return;
	}
	pthread_mutex_lock(&capture->analysis_mutex);
	const sz total_samples = (sz)frame_count * channels;
	for (sz i = 0; i < total_samples; i++) {
		capture->analysis_buffer[capture->analysis_sample_write] = frames[i];
		capture->analysis_sample_write = (capture->analysis_sample_write + 1) % sample_capacity;
	}
	sz frames_valid = capture->analysis_frames_valid + frame_count;
	if (frames_valid > capture->analysis_capacity_frames) {
		frames_valid = capture->analysis_capacity_frames;
	}
	capture->analysis_frames_valid = frames_valid;
	pthread_mutex_unlock(&capture->analysis_mutex);
}

static b8 se_audio_load_analysis_data(const char* full_path, se_audio_clip* clip) {
	if (!clip || !full_path) {
		return false;
	}
	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, clip->owner ? clip->owner->config.channels : 2, clip->owner ? clip->owner->config.sample_rate : 48000);
	ma_uint64 frame_count = 0;
	void* data = NULL;
	ma_result result = ma_decode_file(full_path, &config, &frame_count, &data);
	if (result != MA_SUCCESS || !data || frame_count == 0) {
		clip->analysis_pcm = NULL;
		clip->analysis_frame_count = 0;
		clip->analysis_channels = 0;
		clip->analysis_sample_rate = 0;
		return false;
	}
	if (config.channels == 0 && clip->owner) {
		config.channels = clip->owner->config.channels;
	}
	if (config.sampleRate == 0 && clip->owner) {
		config.sampleRate = clip->owner->config.sample_rate;
	}
	clip->analysis_pcm = (f32*)data;
	clip->analysis_frame_count = frame_count;
	clip->analysis_channels = config.channels;
	clip->analysis_sample_rate = config.sampleRate;
	return true;
}

static void se_audio_free_analysis_data(se_audio_clip* clip) {
	if (!clip || !clip->analysis_pcm) {
		clip->analysis_frame_count = 0;
		clip->analysis_channels = 0;
		clip->analysis_sample_rate = 0;
		return;
	}
	ma_free(clip->analysis_pcm, NULL);
	clip->analysis_pcm = NULL;
	clip->analysis_frame_count = 0;
	clip->analysis_channels = 0;
	clip->analysis_sample_rate = 0;
}

static void se_audio_compute_band_levels(const f32* samples, sz frame_count, u32 channels, u32 sample_rate, se_audio_band_levels* out_levels) {
	if (!samples || !out_levels || frame_count == 0 || channels == 0 || sample_rate == 0) {
		if (out_levels) {
			out_levels->low = 0.0f;
			out_levels->mid = 0.0f;
			out_levels->high = 0.0f;
		}
		return;
	}
	const f32 alpha_low = se_audio_lowpass_alpha(SE_AUDIO_LOW_CUTOFF_HZ, sample_rate);
	const f32 alpha_mid = se_audio_lowpass_alpha(SE_AUDIO_MID_CUTOFF_HZ, sample_rate);
	f32 low = 0.0f;
	f32 mid = 0.0f;
	f64 energy_low = 0.0;
	f64 energy_mid = 0.0;
	f64 energy_high = 0.0;
	for (sz frame = 0; frame < frame_count; frame++) {
		f32 mono = 0.0f;
		for (u32 ch = 0; ch < channels; ch++) {
			mono += samples[frame * channels + ch];
		}
		mono /= (f32)channels;
		low += alpha_low * (mono - low);
		f32 high_pass = mono - low;
		mid += alpha_mid * (high_pass - mid);
		f32 low_component = low;
		f32 mid_component = mid;
		f32 high_component = high_pass - mid_component;
		energy_low += (f64)(low_component * low_component);
		energy_mid += (f64)(mid_component * mid_component);
		energy_high += (f64)(high_component * high_component);
	}
	const f32 inv_frames = 1.0f / (f32)frame_count;
	out_levels->low = sqrtf((f32)(energy_low * inv_frames));
	out_levels->mid = sqrtf((f32)(energy_mid * inv_frames));
	out_levels->high = sqrtf((f32)(energy_high * inv_frames));
}

static f32 se_audio_lowpass_alpha(f32 cutoff_hz, u32 sample_rate) {
	if (cutoff_hz <= 0.0f || sample_rate == 0) {
		return 1.0f;
	}
	const f32 pi = 3.14159265359f;
	const f32 rc = 1.0f / (2.0f * pi * cutoff_hz);
	const f32 dt = 1.0f / (f32)sample_rate;
	return dt / (rc + dt);
}
