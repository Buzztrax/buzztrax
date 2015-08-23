# SimSyn alike

More synthesizers
- we'd like to have one with
  - dual-osc
    - with detune for the 2nd osc: coarse in semitones
      (factor for one semitone: pow(2,1/12) =~ 1.05946))
    - combine for the mixing
  - two ADSR envelopes for the osc-volume
    - maybe one even with initial delay
  - glide to use a curve-envelope to transition from previous note to new note
    - glide-time and glide-curve
  - another ADSR envelope for the filter-cutoff (also one for resonance?)
  - lfo?
- current simsyn is missing key-tracking for the filter
  - we could add a boolean to enable key-tracking (false by default)
  - on cut-off changes we need to base it on the currently played note
  - current cut-off range is 0.0->1.0 with 1.0 = srat/2.0
  - v1: we could map 1.0 -> note_freq*32.0 (5 harmonics) instead
  - v2: we could assume the current cut-off is related to C-4 (base), get freq
    for base and tone and:
    a) map for tone above and below base
    b) map for tone related to base and clamp against 1.0

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

# sfxr
https://github.com/grimfang4/sfxr/blob/master/sfxr/source/main.cpp

# synth components
The libgstbuzztrax already has a couple of components; we'd like to extend this
to make it dead easy writing new synths.

## component ideas
### osc-*
- non-linear sampling (phase distortion), right now the step that advances the
  phase is 'constant'. we can map the phase through a function. this creates
  more overtones. this could be a curve function that we also (like) to use on
  the envelope-d, so that the curve-param is editable. A curve=0.5 would be 
  linear.
- add a sync/cycle/trigger property
  - set: gstbt_osc_synth_trigger()
  - get:
  - notify: when a has a phase done (re-trig)
  this way we can use a GBinding to sync to osc's

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

### component interface
- most components would need to know e.g. the 'sampling-rate'
- most components need to restart envelopes on note-on, maybe per
 'trigger' property
- all synths can have a list of 'synth-components' and use that to configure,
  reset and release them
- components should implement propertymeta to supply a describe function for
  params

## open questions
- do we need to pass the data fmt and channels? right now all components are
  "mono s16"
- should we make the components GstElements (GstAudioSynth and GstAudioFilter)?
  - the actual elements would be bins, that ideally are configured through a 
    spec-file (check SynthEd and co.)
  - for this we need to handle the running time mapping from trigger for the
    components
  - pro: debug_dumps would show the layout of the synth/effect
