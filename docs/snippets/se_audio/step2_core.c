#include "se_audio.h"

int main(void) {
	se_audio_engine* audio = se_audio_init(0);
	se_audio_update(audio);
	se_audio_clip* clip = se_audio_clip_load(audio, SE_RESOURCE_PUBLIC("audio/chime.wav"));
	se_audio_play_params play = {0};
	play.volume = 0.8f;
	play.bus = SE_AUDIO_BUS_SFX;
	se_audio_clip_play(clip, &play);
	return 0;
}
