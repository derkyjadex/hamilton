/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <SDL2/SDL.h>

#include "hamilton/band.h"
#include "hamilton/lib.h"
#include "hamilton/audio.h"
#include "hamilton/midi.h"
#include "hamilton/core_synths.h"
#include "hamilton/cmds.h"
#include "albase/lua.h"
#include "albase/script.h"

#undef main
int main(int argc, char *argv[])
{
	BEGIN()

	HmBand *band = NULL;
	lua_State *L = NULL;

	TRY(hm_band_init(&band));

	TRY(sine_wave_register(band));
	TRY(mda_dx10_register(band));

	TRY(hm_audio_init(band));
	hm_audio_start();

	TRY(al_script_init(&L));
	hm_load_cmds(L, band);

	for (int i = 1; i < argc; i++) {
		TRY(al_script_run_file(L, argv[i]));
	}

	while (true) {
		hm_seq_process_messages(hm_band_get_seq(band));

		lua_getglobal(L, "frame");
		if (!lua_isnil(L, -1)) {
			lua_call(L, 0, 0);
		} else {
			lua_pop(L, 1);
		}

		SDL_Delay(10);
	}

	PASS(
		hm_audio_free();
		hm_band_free(band);
		lua_close(L);
	)
}
