/*
 * Copyright (c) 2013 James Deery
 * Released under the MIT license <http://opensource.org/licenses/MIT>.
 * See COPYING for details.
 */

#include <stdlib.h>

#include "hamilton/midi.h"
#include "portmidi.h"

static PmStream *midi = NULL;

int hm_midi_init()
{
	int error;

	error = Pm_Initialize();
	if (error) goto end;

	int count = Pm_CountDevices();
    for (int i = 0; i < count; i++) {
        const PmDeviceInfo *device = Pm_GetDeviceInfo(i);
        if (device->input) {
            error = Pm_OpenInput(&midi, i, NULL, 265, NULL, NULL);
			if (error) goto end;

            error = Pm_SetFilter(midi, ~PM_FILT_NOTE & ~PM_FILT_CONTROL & ~PM_FILT_PROGRAM);
			if (error) goto end;

            PmEvent event;
            while (Pm_Poll(midi)) {
                Pm_Read(midi, &event, 1);
            }

            break;
        }
    }

end:
	if (error) {
		hm_midi_free();
	}

	return error;
}

void hm_midi_free(void)
{
	midi = NULL;
	Pm_Terminate();
}

int hm_midi_read(uint8_t *status, uint8_t *data1, uint8_t *data2)
{
	PmEvent event;
	int result = Pm_Read(midi, &event, 1);

	if (result > 0) {
		*status = Pm_MessageStatus(event.message);
		*data1 = Pm_MessageData1(event.message);
		*data2 = Pm_MessageData2(event.message);
	}

	return result;
}
