#include <SDL/SDL.h>

#include "band.h"
#include "lib.h"
#include "audio.h"
#include "portmidi.h"

int sine_wave_register();
int mda_dx10_register();

static PmStream *midi;

static int run()
{
	int error;

	SDL_Surface *screen = NULL;

	error = SDL_Init(SDL_INIT_VIDEO);
	if (error) goto end;

	screen = SDL_SetVideoMode(300, 200, 0, 0);
	if (!screen) goto end;

	Pm_Initialize();

    int count = Pm_CountDevices();
    for (int i = 0; i < count; i++) {
        const PmDeviceInfo *device = Pm_GetDeviceInfo(i);
        if (device->input) {
            Pm_OpenInput(&midi, i, NULL, 265, NULL, NULL);
            Pm_SetFilter(midi,  ~PM_FILT_NOTE & ~PM_FILT_CONTROL);
            PmEvent event;
            while (Pm_Poll(midi)) {
                Pm_Read(midi, &event, 1);
            }

            break;
        }
    }

	error = audio_init();
	if (error) goto end;

	audio_start();

	bool finished = false;

	while (!finished) {
		{
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
					case SDL_KEYDOWN:
					case SDL_KEYUP:
						{
							SDLKey key = event.key.keysym.sym;
							if (key == SDLK_ESCAPE) {
								finished = true;

							} else if (key >= SDLK_0 && key <= SDLK_9) {
								band_send_note(0, 0, (event.type == SDL_KEYDOWN), key + 12, 1.0);
							}
						}
						break;

					case SDL_QUIT:
						finished = true;
						break;
				}
			}
		}

		{
			PmEvent event;
			while (Pm_Read(midi, &event, 1) > 0) {
				uint8_t status = Pm_MessageStatus(event.message);
				uint8_t type = status >> 4;
				uint8_t channel = status & 0x0F;
				uint8_t data1 = Pm_MessageData1(event.message);
				uint8_t data2 = Pm_MessageData2(event.message);

				printf("%x, %x, %x, %x\n", type, channel, data1, data2);

				switch (type) {
					case 0x8:
						band_send_note(0, channel, false, data1, data2 / 127.0);
						break;

					case 0x9:
						band_send_note(0, channel, true, data1, data2 / 127.0);
						break;

					case 0xB:
						band_send_cc(0, channel, data1, data2 / 127.0);
						break;
				}
			}
		}

		SDL_Delay(4);
	}

end:
	audio_free();
	Pm_Terminate();
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

	error = mda_dx10_register();
	if (error) goto end;

	int n;
	const SynthType *synths = lib_get_synths(&n);

	error = band_set_channel_synth(0, &synths[1]);
	if (error) goto end;

	error = run();

end:
	band_free();

	return error;
}
