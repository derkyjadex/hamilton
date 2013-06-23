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
#include "hamilton/core_synths.h"
#include "hamilton/cmds.h"
#include "albase/lua.h"
#include "albase/script.h"

static int run(HmBand *band)
{
	BEGIN()

	TRY(hm_midi_init());
	TRY(hm_audio_init(band));

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
						hm_band_send_note(band, 0, channel, false, data1, data2 / 127.0);
						break;

					case 0x9:
						hm_band_send_note(band, 0, channel, true, data1, data2 / 127.0);
						break;

					case 0xB:
						hm_band_send_cc(band, 0, channel, data1, data2 / 127.0);
						break;

					case 0xC:
						hm_band_send_patch(band, 0, channel, data1);
						break;
				}
			}
		}

		SDL_Delay(4);
	}

	PASS(
		hm_audio_free();
		hm_midi_free();
	)
}

#undef main
int main(int argc, char *argv[])
{
	BEGIN()

	HmBand *band = NULL;
	lua_State *L = NULL;

	TRY(hm_band_init(&band));
	TRY(al_script_init(&L));
	hm_load_cmds(L, band);

	TRY(sine_wave_register(band));
	TRY(mda_dx10_register(band));

	int n;
	const HmSynthType *synths = hm_lib_get_synths(hm_band_get_lib(band), &n);

	TRY(hm_band_set_channel_synth(band, 0, &synths[1]));

	TRY(run(band));

	PASS(
		hm_band_free(band);
		lua_close(L);
	)
}
