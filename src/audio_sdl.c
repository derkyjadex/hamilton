/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <stdio.h>
#include <stdint.h>

#include "hamilton/audio.h"
#include "hamilton/band.h"

static const int BUFFER_SIZE = 256;

static bool waitingForStart;
static SDL_sem *started;

static void callback(void *data, Uint8 *output, int length)
{
	if (waitingForStart) {
		waitingForStart = false;
		SDL_SemPost(started);
	}

	HmBand *band = (HmBand *)data;
	length /= 2;
	float buffer[length];

	hm_band_run(band, buffer, length);

	int16_t *samples = (int16_t *)output;

	for (int i = 0; i < length; i++) {
		float x = buffer[i];
		x = (x < -1.0) ? -1.0 : (x < 1.0) ? x: 1.0;

		samples[i] = 32767.0 * x;
	}
}

int hm_audio_init(HmBand *band)
{
	int error;

	started = SDL_CreateSemaphore(0);
	if (!started) {
		error = 1;
		goto end;
	}

	error = SDL_InitSubSystem(SDL_INIT_AUDIO);
	if (error) goto end;

	SDL_AudioSpec desired = {
		.freq = HM_SAMPLE_RATE,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = BUFFER_SIZE,
		.callback = callback,
		.userdata = band
	}, obtained;

	error = SDL_OpenAudio(&desired, &obtained);

end:
	if (error) {
		fprintf(stderr, "Error opening audio: %s\n", SDL_GetError());
		hm_audio_free();
	}

	return error;
}

void hm_audio_free()
{
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	SDL_DestroySemaphore(started);
}

void hm_audio_start()
{
	waitingForStart = true;
	SDL_PauseAudio(0);
	SDL_SemWait(started);
}

void hm_audio_pause()
{
	SDL_PauseAudio(1);
}
