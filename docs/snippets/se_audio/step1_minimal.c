#include "se_audio.h"

int main(void) {
	se_audio_engine* audio = se_audio_init(0);
	se_audio_update(audio);
	return 0;
}
