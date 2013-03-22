/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL/SDL.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "hamilton/audio.h"
#include "hamilton/midi.h"

int sine_wave_register(void);
int mda_dx10_register(void);

static int run()
{
	int error;

	error = hm_midi_init();
	if (error) goto end;

	error = hm_audio_init();
	if (error) goto end;

	hm_audio_start();

	bool finished = false;

	while (!finished) {
		{
			uint8_t status, data1, data2;
			while (hm_midi_read(&status, &data1, &data2) > 0) {
				uint8_t type = status >> 4;
				uint8_t channel = status & 0x0F;

				printf("%x, %x, %x, %x\n", type, channel, data1, data2);

				switch (type) {
					case 0x8:
						hm_band_send_note(0, channel, false, data1, data2 / 127.0);
						break;

					case 0x9:
						hm_band_send_note(0, channel, true, data1, data2 / 127.0);
						break;

					case 0xB:
						hm_band_send_cc(0, channel, data1, data2 / 127.0);
						break;

					case 0xC:
						hm_band_send_patch(0, channel, data1);
						break;
				}
			}
		}

		SDL_Delay(4);
	}

end:
	hm_audio_free();
	hm_midi_free();

	return error;
}

#undef main
int main(int argc, char *argv[])
{
	int error;

	error = hm_band_init();
	if (error) goto end;

	error = sine_wave_register();
	if (error) goto end;

	error = mda_dx10_register();
	if (error) goto end;

	int n;
	const HmSynthType *synths = hm_lib_get_synths(&n);

	error = hm_band_set_channel_synth(0, &synths[1]);
	if (error) goto end;

	error = run();

end:
	hm_band_free();

	return error;
}
