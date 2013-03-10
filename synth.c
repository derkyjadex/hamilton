#include <SDL.h>

#include "public.h"

int sine_wave_register();

static int run_audio(int duration)
{
	int error;

	error = audio_init();
	if (error)
		return error;

	audio_start();
	SDL_Delay(duration);

	audio_free();

	return 0;
}

static int run_console()
{
	const int LEN = 4000;
	int16_t buffer[LEN];
	band_run(buffer, LEN);
	for (int i = 0; i < LEN; i++) {
		printf("%hd ", buffer[i]);
	}

	return 0;
}

#undef main
int main(int argc, char *argv[])
{
	int error;

	error = band_init();
	if (error)
		return error;

	error = sine_wave_register();
	if (error)
		return error;

	int n;
	const SynthType *synths = lib_get_synths(&n);
	for (int i = 0; i < n; i++) {
		printf("%d: %s\n", i, synths[i].name);
	}

	error = band_set_synth(0, &synths[0]);
	if (error)
		return error;

	mq_note(   0, 0, 1, 67, 1.0);
	mq_note( 200, 0, 1, 69, 1.0);
	mq_note( 400, 0, 0, 69, 1.0);
	mq_note( 600, 0, 1, 67, 1.0);
	mq_note( 800, 0, 0, 67, 1.0);

	run_audio(1500);

	band_free();

	return 0;
}
