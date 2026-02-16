// Syphax-Engine - Ougi Washi

#include "se_audio.h"
#include "se_graphics.h"
#include "se_window.h"

#include <stdio.h>

// What you will learn: start audio engine, play a clip, and control volume.
// Try this: switch audio files, tweak volumes, or change the loop bus.
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
	se_render_set_background_color(s_vec4(0.04f, 0.05f, 0.08f, 1.0f));

	se_audio_engine* audio = se_audio_init(NULL);
	se_audio_clip* chime = se_audio_clip_load(audio, SE_RESOURCE_PUBLIC("audio/chime.wav"));
	se_audio_stream* loop = se_audio_stream_open(audio, SE_RESOURCE_PUBLIC("audio/loop.wav"), &(se_audio_play_params){
		.volume = 0.30f,
		.pan = 0.0f,
		.pitch = 1.0f,
		.looping = true,
		.bus = SE_AUDIO_BUS_MUSIC
	});

	f32 master_volume = 1.0f;
	b8 music_on = true;
	printf("audio_basics controls:\n");
	printf("  Space: play chime\n");
	printf("  M: mute/unmute loop music\n");
	printf("  Up/Down: master volume\n");
	printf("  Esc: quit\n");

	while (!se_window_should_close(window)) {
		se_window_begin_frame(window);
		se_audio_update(audio);

		if (se_window_is_key_pressed(window, SE_KEY_SPACE) && chime) {
			se_audio_clip_play(chime, &(se_audio_play_params){ .volume = 0.9f, .pan = 0.0f, .pitch = 1.0f, .looping = false, .bus = SE_AUDIO_BUS_SFX });
		}
		if (se_window_is_key_pressed(window, SE_KEY_M)) {
			music_on = !music_on;
			if (loop) {
				se_audio_stream_set_volume(loop, music_on ? 0.30f : 0.0f);
			}
			printf("audio_basics :: music %s\n", music_on ? "on" : "off");
		}
		if (se_window_is_key_pressed(window, SE_KEY_UP)) {
			master_volume = s_min(1.0f, master_volume + 0.1f);
			se_audio_bus_set_volume(audio, SE_AUDIO_BUS_MASTER, master_volume);
		}
		if (se_window_is_key_pressed(window, SE_KEY_DOWN)) {
			master_volume = s_max(0.0f, master_volume - 0.1f);
			se_audio_bus_set_volume(audio, SE_AUDIO_BUS_MASTER, master_volume);
		}

		se_render_clear();
		se_window_end_frame(window);
	}

	se_audio_stream_close(loop);
	se_audio_clip_unload(audio, chime);
	se_audio_shutdown(audio);
	se_context_destroy(context);
	return 0;
}
