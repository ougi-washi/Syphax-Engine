// Syphax-Engine - Ougi Washi

#include "se_audio.h"

#include <stdio.h>
#include <time.h>

typedef struct {
	se_audio_engine* engine;
	se_audio_clip* clip;
	se_audio_stream* stream;
	se_audio_capture* capture;
	se_audio_device_info devices[16];
	u32 device_count;
	i32 default_device;
} audio_demo_state;

static void sleep_milliseconds(u32 ms) {
	struct timespec req = {0};
	req.tv_sec = ms / 1000;
	req.tv_nsec = (ms % 1000) * 1000000ULL;
	nanosleep(&req, NULL);
}

static i32 audio_demo_find_default_device(const audio_demo_state* state) {
	for (u32 i = 0; i < state->device_count; i++) {
		if (state->devices[i].is_default) {
			return (i32)i;
		}
	}
	return -1;
}

static void audio_demo_print_devices(const audio_demo_state* state) {
	printf("5_audio :: detected %u capture devices\n", state->device_count);
	if (state->device_count == 0) {
		printf("  (none)\n");
		return;
	}
	for (u32 i = 0; i < state->device_count; i++) {
		printf("  [%u] %s%s\n",
		       i,
		       state->devices[i].name,
		       state->devices[i].is_default ? " (default)" : "");
	}
}

static void audio_demo_build_bar(f32 value, char* out, u32 length) {
	if (value < 0.0f) {
		value = 0.0f;
	}
	if (value > 1.0f) {
		value = 1.0f;
	}
	u32 filled = (u32)(value * (f32)length);
	if (filled > length) {
		filled = length;
	}
	for (u32 i = 0; i < length; i++) {
		out[i] = (i < filled) ? '#' : '.';
	}
	out[length] = '\0';
}

static void audio_demo_render_status(const se_audio_band_levels* output_levels, b8 has_output, const se_audio_band_levels* input_levels, b8 has_input) {
	char out_low[17];
	char out_mid[17];
	char out_high[17];
	char in_low[17];
	char in_mid[17];
	char in_high[17];
	if (has_output) {
		audio_demo_build_bar(output_levels->low, out_low, 16);
		audio_demo_build_bar(output_levels->mid, out_mid, 16);
		audio_demo_build_bar(output_levels->high, out_high, 16);
	} else {
		audio_demo_build_bar(0.0f, out_low, 16);
		audio_demo_build_bar(0.0f, out_mid, 16);
		audio_demo_build_bar(0.0f, out_high, 16);
	}
	if (has_input) {
		audio_demo_build_bar(input_levels->low, in_low, 16);
		audio_demo_build_bar(input_levels->mid, in_mid, 16);
		audio_demo_build_bar(input_levels->high, in_high, 16);
	} else {
		audio_demo_build_bar(0.0f, in_low, 16);
		audio_demo_build_bar(0.0f, in_mid, 16);
		audio_demo_build_bar(0.0f, in_high, 16);
	}
	printf("\rOutput L[%s] M[%s] H[%s] | Input L[%s] M[%s] H[%s]    ", out_low, out_mid, out_high, in_low, in_mid, in_high);
	fflush(stdout);
}

int main(void) {
	audio_demo_state state = {0};
	printf("5_audio :: initializing engine\n");
	state.engine = se_audio_init(NULL);

	state.device_count = se_audio_capture_list_devices(state.engine, state.devices, 16);
	state.default_device = audio_demo_find_default_device(&state);
	audio_demo_print_devices(&state);

	state.clip = se_audio_clip_load(state.engine, SE_RESOURCE_PUBLIC("audio/chime.wav"));
	se_audio_play_params clip_params = {
		.volume = 0.85f,
		.pan = 0.0f,
		.pitch = 1.0f,
		.looping = false,
		.bus = SE_AUDIO_BUS_SFX
	};
	se_audio_clip_play(state.clip, &clip_params);

	se_audio_play_params stream_params = {
		.volume = 0.35f,
		.pan = 0.0f,
		.pitch = 1.0f,
		.looping = true,
		.bus = SE_AUDIO_BUS_MUSIC
	};
	state.stream = se_audio_stream_open(state.engine, SE_RESOURCE_PUBLIC("audio/loop.wav"), &stream_params);

	if (state.device_count > 0) {
		se_audio_capture_config capture_config = {
			.device_index = (state.default_device >= 0) ? (u32)state.default_device : 0,
			.sample_rate = 48000,
			.channels = 1,
			.frames_per_buffer = 256
		};
		printf("5_audio :: using capture device [%u]\n", capture_config.device_index);
		state.capture = se_audio_capture_start(state.engine, &capture_config);
	} else {
		printf("5_audio :: microphone capture unavailable.\n");
	}

	printf("\nMonitoring low/mid/high bands for playback (output) and microphone (input).\n");
	const u32 iterations = 120;
	for (u32 i = 0; i < iterations; i++) {
		se_audio_update(state.engine);
		se_audio_band_levels output_levels = {0};
		se_audio_band_levels input_levels = {0};
		b8 has_output = se_audio_clip_get_bands(state.clip, &output_levels, 2048);
		b8 has_input = state.capture ? se_audio_capture_get_bands(state.capture, &input_levels) : false;
		audio_demo_render_status(&output_levels, has_output, &input_levels, has_input);
		sleep_milliseconds(40);
	}
	printf("\n");

	printf("5_audio :: shutting down\n");
	se_audio_capture_stop(state.capture);
	se_audio_stream_close(state.stream);
	se_audio_clip_unload(state.engine, state.clip);
	se_audio_shutdown(state.engine);
	return 0;
}
