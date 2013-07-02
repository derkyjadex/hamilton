local hm = require 'hamilton'

local synths = {hm.get_synths()}
hm.set_synth(0, synths[2])
hm.set_synth(1, synths[2])

hm.add_note(0, 000000, 20000, 60, 0.5)
hm.add_note(0, 000200, 18800, 64, 0.5)
hm.add_note(0, 000400, 17600, 67, 0.5)

hm.add_note(0, 018000, 22000, 67, 0.5)
hm.add_note(0, 019000, 21000, 64, 0.5)
hm.add_note(0, 020000, 20000, 60, 0.5)

hm.add_set_patch(1, 0, 31)
hm.add_note(1, 000000, 2000, 62, 0.7)
hm.add_note(1, 005000, 2000, 58, 0.7)
hm.add_note(1, 010000, 2000, 58, 0.7)
hm.add_note(1, 015000, 2000, 58, 0.7)

hm.add_note(1, 020000, 2000, 62, 0.7)
hm.add_note(1, 025000, 2000, 58, 0.7)
hm.add_note(1, 030000, 2000, 58, 0.7)
hm.add_note(1, 035000, 2000, 58, 0.7)

hm.set_loop(000000, 040000)
hm.set_looping(true)

hm.play()

function frame()
	local state = hm.get_band_state()
	local messages = hm.get_seq_messages()
end
