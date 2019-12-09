# More synthesizers

## SimSyn alike
- we'd like to have one with
  - dual-osc
    - with detune for the 2nd osc: coarse in semitones
      (factor for one semitone: pow(2,1/12) =~ 1.05946))
    - combine for the mixing
  - glide to use a curve-envelope to transition from previous note to new note
    - glide-time and glide-curve
  - two ADSR envelopes
    - maybe one even with initial delay
    - mod-targets -> osc1-vol, osc2-vol, filter-cutoff, filter-resonance
  - lfo
    - wave-form, frequency
- current simsyn is missing key-tracking for the filter
  - we could add a boolean to enable key-tracking (false by default)
  - on cut-off changes we need to base it on the currently played note
  - current cut-off range is 0.0->1.0 with 1.0 = srat/2.0
  - v1: we could map 1.0 -> note_freq*32.0 (5 harmonics) instead
  - v2: we could assume the current cut-off is related to C-4 (base), get freq
    for base and tone and:
    a) map for tone above and below base
    b) map for tone related to base and clamp against 1.0

## WaveTabSynth
- slightly modulating the wave-offset could be interesting
  - we could e.g. let it progress during the tone or have an envelope for it

## BLoop (beat looper)
- parameters:
  - wavetable-ix
  - subdivide / slices (e.g. 4 for quarter)
    use the last value if not given, default=4
  - slice-ix
    trigger param, from 0 to (subdivide-1)
  - volume
  - backwards (boolean)
    only applied if slice ix is set
  - pitch (percent)
    0x00=0%, 0x7f=100%, 0xff=1000%
- algorithm
  - wavetable ix refers to a drum loop
  - code will always stetch one loop to one beat
  - multiple tracks?
    track params: slice-ix, volume, backwards, pitch
  - pitchshifter or grainsynth?
- example
  subdivide=4, pattern={0,1,2,3} plays the loop as is
  subdivide=4, pattern={0,3,0,1} plays segments in different order

## GrainSynth
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

## Drones
Simple Osc with multiple slightly detuned voices that are spread in the panorama
and where the detuning is set so that it matches the song tempo.

Need tables for each tone to calculate the detune factors so that the phases are
in sync after n-ticks. If this is hard, we can mybe cross-fade a reverse copy
using sin()² curves and reset the phase after n-ticks.

From ticks and samplerate we calculate the number of samples for desired resync.

TODO:
- can we disable resetting the phase on the osc component when triggering a note
  - gstbt_osc_synth_trigger(): self->accumulator = 0.0
  - we only call this when we change waves

Parameters:
- note: tone to play
- wave: classic tonal osc (sin, tri, saw, sqr)
- ticks: when the phases should be in sync again
- sync: when plaing a note - should we sync the phases or not?
- voices: 3,5,7
- stereo spread: 0 ... 100%
  - example: 5 voice, spread=100%
  - voice 1 -> middle
  - voice 2 -> 50% right
  - voice 3 -> 50% left
  - voice 4 -> 100% right
  - voice 5 -> 100% left

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
  linear. (see e.g. http://www.electricdruid.net/index.php?page=info.pdsynthesis)
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

### fancy transitions
http://soledadpenades.com/2012/07/03/tween-audio/
- e.g. bouncy

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
- components should implement propertymeta:
  - to supply a describe function for params
- preview images:
  - add api to supply a 'preview' image (e.g. the wave-form of an osc, or the
    transfer-function of a filter)
    - for combine it might be tricky to get the inputs
    - for the filter, we'd like to edit the cut-off+reso in the image
  - in the UI we'd like to show the image per component
    - osc: the images changes when wave-form, shape, ... changes
    - filter: the image changes when filter, cut-off and resonance changes
- could we also generate a block diagramm and request it from the component
  interface
  - maybe just sent as a structure - a list of rectangles, lines and
    labels - and render it on the ui side, or generate dot files and pre-layout
    as xdot during build.
  - we need a way to draw the preview-images into it
    - the boxes have a tag that refereces the preview image)
  - if the image is in portrait format, we can draw it on the left side of the
    machine-prop window
  - when tabbing through the ui, we'd like to hightlight the related block
    - this can be several of them (e.g. for an envelope length
    - for envelopes it would be nice to highlight the point or segment
    - for filters we'd like to hightlight the cut-off, but resonance is hard


## open questions
- do we need to pass the data fmt and channels? right now all components are
  "mono s16"
- should we make the components GstElements (GstAudioSynth and GstAudioFilter)?
  - the actual elements would be bins, that ideally are configured through a
    spec-file (check SynthEd and co.)
  - for this we need to handle the running time mapping from trigger for the
    components
  - pro: debug_dumps would show the layout of the synth/effect
- for polyphonic elements can we have the voice properties also on the global
  element?
  - when using those we would do voice-allocation.
  - it would not work with the controller though
