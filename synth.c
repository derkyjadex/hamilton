#include <SDL.h>

#include "public.h"

#undef main
int main(int argc, char *argv[])
{
	int error;

	error = audio_init();
	if (error)
		return error;

	audio_start();

	int time = 2 * 1000;
	while (time > 0) {
		SDL_Delay(10);
		time -= 10;
	}

	audio_free();

	return 0;
}
