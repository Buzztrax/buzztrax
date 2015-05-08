# SimSyn alike

More synthesizer
- we'd like to have one with
  - dual-osc (with detune for the 2nd)
  - two ADSR envelopes
  - each envelope can modulate volume and filter-cutoff/resonance
- current simsyn is missing key-tracking for the filter
  - we could add a boolean to enable key-tracking (false by default)
  - on cut-off changes we need to base it on the currently played note
  - current cut-off range is 0.0->1.0 with 1.0 = srat/2.0
  - v1: we could map 1.0 -> note_freq*32.0 (5 harmonics) instead
  - v2: we could assume the current cut-off is related to C-4 and rebase accordingly

# WaveTabSynth
- slightly modulating the wave-offset could be interesting
  - we could e.g. let it progress during the tone or have an envelope for it

# GrainSynth
- a grain synth takes snippets from the one wave-table slot, applies a
  fft-window function and mixes it into the output
- using a triangle-window and a grain-ratio=1.0 would replicate the waveform
  (triangles with 50% overlap)
- depending on the ration of note-lnegth and wave-length the overlap and the
  grain-ratio would change. The overlap is implicit.
  - if note_len>wave_len : add more grains
  - else                 : skip grains
- until now it's merely a form of tiemstreching, to make it more interesting we
  can add:
  - extra grains at a different volume before and after the actual grain
    - volume would be a percentage of the original, spread would be in ms
  - grain size, this determines the width of the window, might be fun to make
    extra grains smaller too
  - panning for extra grains
- one possible way to implement is to have n-'voices' as below
  - for this little overlap and spread we already need 7 voices though
  - the wider a voice spreads and the smaller the overlap is, the more voices we
    need (voices = 1 + (spread_range_ms / overlap_ms)
```
       /\
0 /\  /  \  /\
         /\
1   /\  /  \  /\
           /\
2     /\  /  \  /\
             /\
3       /\  /  \  /\
               /\
4         /\  /  \  /\
                 /\
5           /\  /  \  /\
                   /\
6             /\  /  \  /\
```
spread_range_ms = 12
overlap_ms = 2
voices = 1 + (12 / 2) = 7

# EBeatz
- a versatile percussion sound generator
- dual osc
  - peak-vol is trigger param
  1) tonal
     - osc with sin, saw, sqr, tri
       - start and stop frequency
       - linear fade = d-envelope for frequency?
     - adsr-envelope
       - peak-vol relative to main-peak-vol
     - filter?
  2) noise
     - osc with various noise flavours
     - d-envelope
       - peak-vol relative to main-peak-vol
     - filter
       - adsr/d-envelope for cut-off?
- have presets for kick, snare, {open,closed}-hihat, claps, cymbal

# sfxr
https://github.com/grimfang4/sfxr/blob/master/sfxr/source/main.cpp

# synth components
The libgstbuzztrax already has a couple of components; we'd like to extend this
to make it dead easy writing new synths.

## component ideas
### osc-*
- a goolean 'mix' property to not overwrite the buffer
- have osc-synth variants: one for tonal waveforms and one for noises?
  - we could just subclass and have override the 'Wave' enum?

### osc-wave (wavetable oscillator)
- we might need to add some extra caps fields
  (root-note, loop, loop-start, loop-end)
- do we want it to play loops?
- what if samples are for e.g. 44100 but the audio plays at 48000?
- we could resample all wave-table entries when loading a song (keeping original
  waves in the files)

## design
After adding more components we need to extract common interfaces. Each
component would have a couple of properties and vmethods (start, process, stop).
When using them the element would proxy a few properties and ev. configure
others manually. We can use GBinding (since glib-2.26) for the proxy properties.

## open questions
- do we need to pass the data fmt? What about mono/stereo.
- most components would need to know e.g. the sampling-rate. need an easy way to
  pass it to all of them

