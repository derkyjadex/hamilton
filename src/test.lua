local hm = require 'hamilton'

local synths = hm.get_synths()
hm.set_synth(1, synths[2])
hm.set_synth(2, synths[2])

hm.add_note(1, 00000, 2000, 60, 0.5)
hm.add_note(1, 00020, 1880, 64, 0.5)
hm.add_note(1, 00040, 1760, 67, 0.5)

hm.add_note(1, 01800, 2200, 67, 0.5)
hm.add_note(1, 01900, 2100, 64, 0.5)
hm.add_note(1, 02000, 2000, 60, 0.5)

hm.add_set_patch(2, 0, 32)
hm.add_note(2, 00000, 200, 65, 0.7)
hm.add_note(2, 00500, 200, 58, 0.7)
hm.add_note(2, 01000, 200, 58, 0.7)
hm.add_note(2, 01500, 200, 58, 0.7)

hm.add_note(2, 02000, 200, 62, 0.7)
hm.add_note(2, 02500, 200, 58, 0.7)
hm.add_note(2, 03000, 200, 58, 0.7)
hm.add_note(2, 03500, 200, 58, 0.7)

hm.set_loop(00000, 04000)
hm.set_looping(true)

hm.play()

function frame()
	local state = hm.get_band_state()
	local messages = hm.get_seq_messages()
end
