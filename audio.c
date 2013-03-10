#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <stdio.h>
#include <stdint.h>

#include "private.h"

static void callback(void *_, Uint8 *buffer, int length)
{
	band_run((int16_t *)buffer, length / 2);
}

int audio_init()
{
	SDL_AudioSpec desired = {
		.freq = SAMPLE_RATE,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = BUFFER_SIZE,
		.callback = callback,
		.userdata = NULL
	}, obtained;

	if (SDL_OpenAudio(&desired, &obtained) < 0) {
		fprintf(stderr, "Error opening audio: %s\n", SDL_GetError());
		return 1;
	}

	return 0;
}

void audio_free()
{
	SDL_CloseAudio();
}

void audio_start()
{
	SDL_PauseAudio(0);
}

void audio_pause()
{
	SDL_PauseAudio(1);
}
