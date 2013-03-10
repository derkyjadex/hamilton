#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <stdio.h>
#include <stdint.h>

#include "private.h"

static uint32_t time = 0;
static Message *message = NULL;

static void process_message()
{
	Synth *synth = synths[message->channel];

	switch (message->type) {
		case NOTE_OFF:
			synth->stopNote(synth, message->data.note.num);
			break;

		case NOTE_ON:
			synth->startNote(synth, message->data.note.num, message->data.note.velocity);
			break;

		case PITCH:
			break;

		case CONTROL:
			*message->data.control.ptr = message->data.control.value;
			break;
	}
}

static void process_audio(int16_t *buffer, int length)
{
	do {
		if (!message) {
			message = mq_pop();
		}

		while (message && message->time >= time) {
			process_message();
			message = mq_pop();
		}

		int duration, samples;
		if (message) {
			duration = message->time - time;
			samples = duration * SAMPLE_RATE;
		}

		if (!message || samples > length) {
			samples = length;
			duration = samples / SAMPLE_RATE;
		}

		for (int i = 0; i < NUM_CHANNELS; i++) {
			Synth *synth = synths[i];
			if (synth) {
				synth->generate(synth, buffer, samples);
			}
		}

		time += duration;
		length -= samples;

	} while (length);
}

static void audio_callback(void *_, Uint8 *buffer, int length)
{
	process_audio((int16_t *)buffer, length / 2);
}

int audio_init()
{
	int error;

	error = band_init();
	if (error)
		return error;

	error = mq_init();
	if (error)
		return error;

	SDL_AudioSpec desired = {
		.freq = SAMPLE_RATE,
		.format = AUDIO_S16SYS,
		.channels = 1,
		.samples = BUFFER_SIZE,
		.callback = audio_callback,
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
	mq_free();
	band_free();
}

void audio_start()
{
	SDL_PauseAudio(0);
}

void audio_pause()
{
	SDL_PauseAudio(1);
}
