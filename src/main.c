#include <SDL/SDL.h>

#include "band.h"
#include "lib.h"
#include "audio.h"

int sine_wave_register();

static int run()
{
	int error;

	SDL_Surface *screen = NULL;

	error = SDL_Init(SDL_INIT_VIDEO);
	if (error) goto end;

	screen = SDL_SetVideoMode(800, 600, 0, 0);
	if (!screen) goto end;

	error = audio_init();
	if (error) goto end;

	audio_start();

	bool finished = false;
	SDL_Event event;
	SDLKey key;

	while (!finished) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					key = event.key.keysym.sym;
					if (key >= SDLK_0 && key <= SDLK_9) {
						band_send_note(0, 0, (event.type == SDL_KEYDOWN), key + 12, 1.0);
					}
					break;

				case SDL_QUIT:
					finished = true;
					break;
			}
		}

		SDL_Delay(4);
	}

end:
	audio_free();
	SDL_Quit();

	return error;
}

int main(int argc, char *argv[])
{
	int error;

	error = band_init();
	if (error) goto end;

	error = sine_wave_register();
	if (error) goto end;

	int n;
	const SynthType *synths = lib_get_synths(&n);

	error = band_set_channel_synth(0, &synths[0]);
	if (error) goto end;

	error = run();

end:
	band_free();

	return error;
}
