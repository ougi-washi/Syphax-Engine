// Syphax-Engine - Ougi Washi

#include "se_audio.h"
#include "se_graphics.h"
#include "se_ui.h"
#include "se_window.h"
#include <math.h>
#include <stdio.h>

#define AUDIO_BASICS_CAPTURE_SCAN_MAX 8
#define AUDIO_BASICS_METER_SEGMENTS 16

// What you will learn: start audio, control volume, and react to default input capture.
// Try this: talk into your default mic, tap the desk, or swap the audio files.
static f32 audio_basics_meter(const f32 value) {
	return sqrtf(s_max(0.0f, s_min(1.0f, value * 18.0f)));
}

static void audio_basics_bar(c8* out_text, const sz out_size, const f32 level) {
	if (!out_text || out_size < 4) {
		return;
	}
	const i32 filled = (i32)(s_max(0.0f, s_min(1.0f, level)) * AUDIO_BASICS_METER_SEGMENTS + 0.5f);
	sz cursor = 0;
	out_text[cursor++] = '[';
	for (i32 i = 0; i < AUDIO_BASICS_METER_SEGMENTS && cursor + 2 < out_size; ++i) out_text[cursor++] = i < filled ? '#' : '.';
	out_text[cursor++] = ']';
	out_text[cursor] = '\0';
}

static void audio_basics_default_capture_name(se_audio_engine* audio, c8* out_name, const sz out_name_size) {
	if (!out_name || out_name_size == 0) {
		return;
	}
	snprintf(out_name, out_name_size, "system default");
	if (!audio) {
		return;
	}
	se_audio_device_info devices[AUDIO_BASICS_CAPTURE_SCAN_MAX] = {0};
	const u32 device_count = se_audio_capture_list_devices(audio, devices, AUDIO_BASICS_CAPTURE_SCAN_MAX);
	for (u32 i = 0; i < device_count; ++i) {
		if (devices[i].is_default && devices[i].name[0] != '\0') {
			snprintf(out_name, out_name_size, "%s", devices[i].name);
			return;
		}
	}
	if (device_count > 0 && devices[0].name[0] != '\0') snprintf(out_name, out_name_size, "%s", devices[0].name);
}
int main(void) {
	se_context* context = se_context_create();
	se_window_handle window = se_window_create("Syphax - Audio Basics", 960, 540);
	if (window == S_HANDLE_NULL) {
		printf("audio_basics :: window unavailable (%s)\n", se_result_str(se_get_last_error()));
		se_context_destroy(context);
		return 1;
	}

	se_window_set_exit_key(window, SE_KEY_ESCAPE);
	se_window_set_target_fps(window, 60);
	se_render_set_background(s_vec4(0.04f, 0.05f, 0.08f, 1.0f));

	se_ui_handle ui = se_ui_create(window, 12);
	se_ui_widget_handle root = se_ui_create_root(ui);
	se_ui_vstack(ui, root, 0.010f, se_ui_edge_all(0.03f));
	se_ui_add_text(root, { .text = "Audio Basics", .size = s_vec2(0.30f, 0.08f) });
	se_ui_add_text(root, { .text = "Space play chime | M mute loop | Up/Down master | Esc quit", .size = s_vec2(0.82f, 0.06f) });
	se_ui_widget_handle master_label = se_ui_add_text(root, { .text = "Master volume: 100%", .size = s_vec2(0.36f, 0.06f) });
	se_ui_widget_handle music_label = se_ui_add_text(root, { .text = "Loop music: on", .size = s_vec2(0.28f, 0.06f) });
	se_ui_widget_handle capture_label = se_ui_add_text(root, { .text = "Default input: system default", .size = s_vec2(0.82f, 0.06f) });
	se_ui_widget_handle input_label = se_ui_add_text(root, { .text = "Input [................] L00 M00 H00", .size = s_vec2(0.72f, 0.06f) });

	se_audio_engine* audio = se_audio_init(NULL);
	if (!audio) {
		printf("audio_basics :: audio unavailable (%s)\n", se_result_str(se_get_last_error()));
	}
	se_audio_clip* chime = audio ? se_audio_clip_load(audio, SE_RESOURCE_PUBLIC("audio/chime.wav")) : NULL;
	se_audio_stream* loop = audio ? se_audio_stream_open(audio, SE_RESOURCE_PUBLIC("audio/loop.wav"), &(se_audio_play_params){
		.volume = 0.30f, .pan = 0.0f, .pitch = 1.0f, .looping = true, .bus = SE_AUDIO_BUS_MUSIC
	}) : NULL;
	c8 capture_name[SE_AUDIO_DEVICE_NAME_MAX] = "system default";
	audio_basics_default_capture_name(audio, capture_name, sizeof(capture_name));
	se_audio_capture* capture = audio ? se_audio_capture_start(audio, NULL) : NULL;
	if (audio && !chime) printf("audio_basics :: chime load failed (%s)\n", se_result_str(se_get_last_error()));
	if (audio && !loop) printf("audio_basics :: loop open failed (%s)\n", se_result_str(se_get_last_error()));
	if (audio && !capture) printf("audio_basics :: default input unavailable (%s)\n", se_result_str(se_get_last_error()));
	c8 line[160] = {0};
	if (!audio) se_ui_widget_set_text(ui, capture_label, "Audio engine unavailable");
	else if (!capture) se_ui_widget_set_text(ui, capture_label, "Default input unavailable");
	else {
		snprintf(line, sizeof(line), "Default input: %s", capture_name);
		se_ui_widget_set_text(ui, capture_label, line);
	}

	f32 master_volume = 1.0f;
	f32 input_level = 0.0f;
	b8 music_on = true;
	printf("audio_basics controls:\n  Space: play chime\n  M: mute/unmute loop music\n  Up/Down: master volume\n  Esc: quit\n");
	if (audio) printf("  Default input meter: %s\n", capture ? capture_name : "unavailable");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_audio_update(audio);

		if (audio && se_window_is_key_pressed(window, SE_KEY_SPACE) && chime) {
			se_audio_clip_play(chime, &(se_audio_play_params){ .volume = 0.9f, .pan = 0.0f, .pitch = 1.0f, .looping = false, .bus = SE_AUDIO_BUS_SFX });
		}
		if (se_window_is_key_pressed(window, SE_KEY_M)) {
			music_on = !music_on;
			if (loop) se_audio_stream_set_volume(loop, music_on ? 0.30f : 0.0f);
		}
		if (audio && se_window_is_key_pressed(window, SE_KEY_UP)) {
			master_volume = s_min(1.0f, master_volume + 0.1f);
			se_audio_bus_set_volume(audio, SE_AUDIO_BUS_MASTER, master_volume);
		}
		if (audio && se_window_is_key_pressed(window, SE_KEY_DOWN)) {
			master_volume = s_max(0.0f, master_volume - 0.1f);
			se_audio_bus_set_volume(audio, SE_AUDIO_BUS_MASTER, master_volume);
		}

		se_audio_band_levels bands = {0};
		f32 low = 0.0f, mid = 0.0f, high = 0.0f;
		if (capture && se_audio_capture_get_bands(capture, &bands)) {
			low = audio_basics_meter(bands.low);
			mid = audio_basics_meter(bands.mid);
			high = audio_basics_meter(bands.high);
		}
		input_level += (s_max(low, s_max(mid, high)) - input_level) * 0.18f;

		c8 meter[32] = {0};
		audio_basics_bar(meter, sizeof(meter), input_level);
		snprintf(line, sizeof(line), "Master volume: %d%%", (i32)(master_volume * 100.0f + 0.5f));
		se_ui_widget_set_text(ui, master_label, line);
		se_ui_widget_set_text(ui, music_label, music_on ? "Loop music: on" : "Loop music: muted");
		snprintf(line, sizeof(line), "Input %s L%02d M%02d H%02d", meter, (i32)(low * 100.0f + 0.5f), (i32)(mid * 100.0f + 0.5f), (i32)(high * 100.0f + 0.5f));
		se_ui_widget_set_text(ui, input_label, line);

		se_ui_tick(ui);
		se_render_clear();
		se_ui_draw(ui);
		se_window_end_frame(window);
	}

	se_audio_capture_stop(capture);
	se_audio_stream_close(loop);
	se_audio_clip_unload(audio, chime);
	se_audio_shutdown(audio);
	se_ui_destroy(ui);
	se_context_destroy(context);
	return 0;
}
