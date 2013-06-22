/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <Jackmp/jack.h>
#include <Jackmp/midiport.h>
#include <stdlib.h>
#include <stdio.h>

#include "hamilton/midi.h"

jack_client_t *client;
jack_port_t *inputPort;

static int process(jack_nframes_t nframes, void *arg)
{
	jack_midi_event_t *buffer = jack_port_get_buffer(inputPort, nframes);
	jack_nframes_t numEvents = jack_midi_get_event_count(buffer);

	for (int i = 0; i < numEvents; i++) {
		jack_midi_event_t event;
		jack_midi_event_get(&event, buffer, i);

		printf("%d\n", event.time);
	}

	return 0;
}

int hm_midi_init()
{
	int error = 0;

/*	client = jack_client_open("hamilton_midi", 0, NULL);
	if (!client) {
		error = 1;
		goto end;
	}

	jack_set_process_callback(client, process, NULL);

	inputPort = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (!inputPort) {
		error = 2;
		goto end;
	}

	error = jack_activate(client);
*/
end:
	if (error) {
		hm_midi_free();
	}

	return error;
}

void hm_midi_free(void)
{
//	jack_client_close(client);
}

int hm_midi_read(uint8_t *status, uint8_t *data1, uint8_t *data2)
{
	return 0;
}
