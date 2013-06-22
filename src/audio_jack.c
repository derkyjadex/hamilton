/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <Jackmp/jack.h>
#include <Jackmp/midiport.h>
#include <stdio.h>
#include <stdint.h>

#include "hamilton/audio.h"
#include "hamilton/band.h"

static jack_client_t *client;
static jack_port_t *midiPort;
static jack_port_t *audioPort;

static int f = 0;

static int process(jack_nframes_t nframes, void *arg)
{
	HmBand *band = (HmBand *)arg;

	void *midi = jack_port_get_buffer(midiPort, nframes);
	float *audio = jack_port_get_buffer(audioPort, nframes);

	hm_band_reset_time(band, 0);

	jack_nframes_t numEvents = jack_midi_get_event_count(midi);
	for (int i = 0; i < numEvents; i++) {
		jack_midi_event_t event;
		jack_midi_event_get(&event, midi, i);

		uint8_t	status = event.buffer[0];
		uint8_t data1 = event.buffer[1];
		uint8_t data2 = event.buffer[2];

		uint8_t type = status >> 4;
		uint8_t channel = status & 0x0F;

		switch (type) {
			case 0x8:
				hm_band_send_note(band, event.time, channel, false, data1, data2 / 127.f);
				break;

			case 0x9:
				hm_band_send_note(band, event.time, channel, true, data1, data2 / 127.f);
				break;

			case 0xB:
				hm_band_send_cc(band, event.time, channel, data1, data2 / 127.f);
				break;

			case 0xC:
				hm_band_send_patch(band, event.time, channel, data1);
				break;

			default:
				break;
		}
	}

	hm_band_run(band, audio, nframes);

	f++;

	return 0;
}

int hm_audio_init(HmBand *band)
{
	int error;

	client = jack_client_open("hamilton", 0, NULL);
	if (!client) {
		error = 1;
		goto end;
	}

	jack_set_process_callback(client, process, band);

	audioPort = jack_port_register(client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	if (!audioPort) {
		error = 2;
		goto end;
	}

	midiPort = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (!midiPort) {
		error = 3;
		goto end;
	}

	error = jack_activate(client);

end:
	if (error) {
		hm_audio_free();
	}

	return error;
}

void hm_audio_free()
{
	jack_client_close(client);
}

void hm_audio_start()
{
}

void hm_audio_pause()
{
}
