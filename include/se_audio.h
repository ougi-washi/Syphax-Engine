// Syphax-Engine - Ougi Washi

#ifndef SE_AUDIO_H
#define SE_AUDIO_H

#include "se_defines.h"
#include "syphax/s_types.h"

#define SE_AUDIO_DEVICE_NAME_MAX 128

typedef enum {
	SE_AUDIO_BUS_MASTER = 0,
	SE_AUDIO_BUS_MUSIC,
	SE_AUDIO_BUS_SFX,
	SE_AUDIO_BUS_COUNT
} se_audio_bus;

typedef struct {
	u32 sample_rate;
	u32 channels;
	u32 max_clips;
	u32 max_streams;
	u32 max_captures;
	b8 enable_capture : 1;
} se_audio_config;

#define SE_AUDIO_CONFIG_DEFAULTS ((se_audio_config){ .sample_rate = 48000, .channels = 2, .max_clips = 32, .max_streams = 8, .max_captures = 2, .enable_capture = true })

typedef struct {
	f32 volume;
	f32 pan;
	f32 pitch;
	b8 looping : 1;
	se_audio_bus bus;
} se_audio_play_params;

typedef struct {
	u32 device_index;
	u32 sample_rate;
	u32 channels;
	u32 frames_per_buffer;
} se_audio_capture_config;

typedef struct {
	char name[SE_AUDIO_DEVICE_NAME_MAX];
	u32 index;
	b8 is_default : 1;
} se_audio_device_info;

typedef struct se_audio_engine se_audio_engine;
typedef struct se_audio_clip se_audio_clip;
typedef struct se_audio_stream se_audio_stream;
typedef struct se_audio_capture se_audio_capture;

typedef struct {
	f32 low;
	f32 mid;
	f32 high;
} se_audio_band_levels;

extern se_audio_engine* se_audio_init(const se_audio_config* config);
extern void se_audio_shutdown(se_audio_engine* engine);
extern void se_audio_update(se_audio_engine* engine);
extern void se_audio_stop_all(se_audio_engine* engine);
extern void se_audio_bus_set_volume(se_audio_engine* engine, se_audio_bus bus, f32 volume);
extern f32 se_audio_bus_get_volume(se_audio_engine* engine, se_audio_bus bus);

extern se_audio_clip* se_audio_clip_load(se_audio_engine* engine, const char* relative_path);
extern void se_audio_clip_unload(se_audio_engine* engine, se_audio_clip* clip);
extern b8 se_audio_clip_play(se_audio_clip* clip, const se_audio_play_params* params);
extern void se_audio_clip_stop(se_audio_clip* clip);
extern void se_audio_clip_set_volume(se_audio_clip* clip, f32 volume);
extern void se_audio_clip_set_pan(se_audio_clip* clip, f32 pan);
extern b8 se_audio_clip_is_playing(const se_audio_clip* clip);

extern se_audio_stream* se_audio_stream_open(se_audio_engine* engine, const char* relative_path, const se_audio_play_params* params);
extern void se_audio_stream_close(se_audio_stream* stream);
extern void se_audio_stream_set_volume(se_audio_stream* stream, f32 volume);
extern void se_audio_stream_set_looping(se_audio_stream* stream, b8 looping);
extern void se_audio_stream_seek(se_audio_stream* stream, f64 seconds);
extern f64 se_audio_stream_get_cursor(const se_audio_stream* stream);

extern u32 se_audio_capture_list_devices(se_audio_engine* engine, se_audio_device_info* out_devices, u32 max_devices);
extern se_audio_capture* se_audio_capture_start(se_audio_engine* engine, const se_audio_capture_config* config);
extern void se_audio_capture_stop(se_audio_capture* capture);
extern sz se_audio_capture_read(se_audio_capture* capture, f32* dst_frames, sz max_frames);
extern b8 se_audio_capture_get_bands(se_audio_capture* capture, se_audio_band_levels* out_levels);

extern b8 se_audio_clip_get_bands(const se_audio_clip* clip, se_audio_band_levels* out_levels, sz max_frames);
extern b8 se_audio_buffer_analyze_bands(const f32* samples, sz frame_count, u32 channels, u32 sample_rate, se_audio_band_levels* out_levels);

#endif // SE_AUDIO_H
