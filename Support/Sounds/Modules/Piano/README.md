# Piano Keys

Sounds for audio feedback playing notes, chosing the fourth octave, including sharps - in three different velocities.

Rendered in the style of an MT-32 (ROM version 1.07) by [`mt32emu_smf2wav`](https://github.com/munt/munt/tree/master/mt32emu_smf2wav), MIDIs produced by `tool module pianokeys midi generate`. Processed by `ffmpeg` to reduce to mono.

## Generation Example

```
~/mt32/mt32emu-smf2wav -p 48000 --src-quality=3 --rom-dir=~/mt32 "file.mid"

Munt MT32Emu MIDI to Wave Conversion Utility. Version 1.9.3
  Copyright (C) 2009, 2011 Jerome Fisher <re_munt@kingguppy.com>
  Copyright (C) 2012-2026 Jerome Fisher, Sergey V. Mikayev
Using Munt MT32Emu Library Version 2.8.3, libsmf Version 1.3 (with modifications)
Using ROMs for machine mt32_1_07.
Using output sample rate 48000 Hz
format: 0 (single track); number of tracks: 1; division: 480 PPQN.
Metadata: Tempo: 500000 microseconds per quarter note, 120.00 BPM
Metadata: End Of Track
Elapsed time: 0.040817 sec
```