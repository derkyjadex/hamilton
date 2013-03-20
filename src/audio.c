/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <stdio.h>
#include <stdint.h>

#include "audio.h"
#include "band.h"

static const int BUFFER_SIZE = 256;

static void callback(void *_, Uint8 *output, int length)
{
	length /= 2;
	float buffer[length];

	band_run(buffer, length);

	int16_t *samples = (int16_t *)output;

	for (int i = 0; i < length; i++) {
		float x = buffer[i];
		x = (x < -1.0) ? -1.0 : (x < 1.0) ? x: 1.0;

		samples[i] = 32767.0 * x;
	}
}

int audio_init()
{
	int error;

	error = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (error) goto end;

	SDL_AudioSpec desired = {
		.freq = SAMPLE_RATE,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = BUFFER_SIZE,
		.callback = callback,
		.userdata = NULL
	}, obtained;

	error = SDL_OpenAudio(&desired, &obtained);

end:
	if (error) {
		fprintf(stderr, "Error opening audio: %s\n", SDL_GetError());
		audio_free();
	}

	return error;
}

void audio_free()
{
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void audio_start()
{
	SDL_PauseAudio(0);
}

void audio_pause()
{
	SDL_PauseAudio(1);
}
