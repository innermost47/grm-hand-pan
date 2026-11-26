# GRM Hand Pan

A handpan whose sound is solved, not sampled. Every number the sound needs (how each tone field rings and for how
long, how the air in the shell answers, how a fingertip strikes, how the fields sing together) comes out of a **GRM
network**: physical relations solved against published measurements, each with its uncertainty. No samples, no
filter tuned by ear. Free software under the AGPL-3.0.

This is a **proof of concept**: a D Kurd handpan, played from MIDI in a minimal VST3 / standalone plugin (JUCE), or
rendered to WAV files without a host.

## Audio examples

Rendered by the model alone, straight from `tools/` (no effect, no reverb, nothing edited), at 1 m from a D Kurd:

- **[A short piece](docs/audio/handpan_melody.mp3)** (50 s): the ding as a pulse, rolls between two fields, flams,
  a fast roll falling into an arpeggio, a sung phrase, a roll on the ding, a last chord left to ring. Listen for the
  neighbours that sing along and beat.
- **[The same piece, on the lap](docs/audio/handpan_melody_on_lap.mp3)**: the knees lengthen the Gu's neck, the air
  mode drops from 85 to 74 Hz, an octave below the ding.
- **[The scale](docs/audio/handpan_d_kurd.mp3)** (29 s): up and down the D Kurd, the ding soft, medium and loud (hear
  it grow brighter), then a chord.

## Download and play

The plugin is built from the sources (see below). In a DAW: load **GRM Hand Pan** (VST3) on an instrument track and
play it from a MIDI keyboard or a clip. Each note-on strikes the field tuned to that note, **D3 A3 Bb3 C4 D4 E4 F4 G4
A4**, at its velocity; note-offs are ignored (a handpan rings until it fades); other notes are silent. Two settings:
**On the lap** and **Stereo width**. The first time it loads, the instrument calibrates itself on its measurements: it
stays silent for about 15 seconds, then plays.

## Status

| Part | State |
| --- | --- |
| Tone fields: tuned and untuned modes, their losses, their radiation | done |
| The cavity's air mode (Helmholtz) and the Gu | done |
| The fingertip strike: a pulp that hardens as it is pressed | done |
| How a strike excites each mode, calibrated on recorded notes | done |
| Sympathetic fields: neighbours that sing and beat | done |
| The finger's click above the modes (plate theory) | done |
| Stereo: each field heard from where it sits on the instrument | done |
| Minimal VST3 / standalone plugin (MIDI in, generic editor) | done |
| A sharper finger attack, matched to finger recordings | next |
| The Hand Pan Roll: a circular sequencer, strike zones, pattern bank | after |

## How it sounds the way it does

The sound path is the one of [GRM Guitar](https://github.com/innermost47/grm-guitar-vst): networks are solved when
something changes, never in the audio callback; their results are plain data that a real-time engine plays.

- **A tone field** is a steel plate tuned to its note. Its network ties the note to the sheet's thickness and the
  steel's stiffness and density, and gives the plate's effective radius (a D3 field about 10.6 cm, an A4 about
  6.1 cm). Its three tuned modes stand in the ratios 1 : 2 : 3 within the 3 % spread measured on real fields; two
  untuned modes, with two nodal diameters, sit near 7.5 and 8.2 times the fundamental.
- **How long each mode rings** is a law solved from 84 decay times measured mode by mode on four handpans from three
  makers: it depends on the field (higher fields ring shorter), not on the mode's frequency, and the octave rings
  longest (about 2.9 s for an A4 field, against 2.3 s for its fundamental).
- **Radiation**: the fundamental and the octave radiate as monopoles, the compound fifth as a dipole, as measured
  with an intensity probe; the parts of a field moving in opposite phases are summed over all directions.
- **The cavity** is a Helmholtz resonator. Its geometry is not published, so the network takes the measured air
  mode (85 Hz played free, 74 Hz on the lap) and the published size of the vessel, and solves the effective neck of
  the Gu and the losses it radiates. The ding pushes air into the cavity; the plug of air in the Gu answers.
- **The strike**: a fingertip lands on its pulp, which is soft at first and hardens as it is pressed (measured by
  MRI). A faster finger presses harder for a shorter time: a loud note is brighter. The finger keeps 19 % of the
  energy and bounces back.
- **Which modes a strike excites** at the sweet spot is solved from recorded notes: the octave and the fifth well,
  the untuned modes about 30 dB less than if a strike excited every mode alike.
- **Sympathy**: modes of different fields exchange their energy (a D3's octave sets the D4 ringing, its fifth the
  A4), at a coupling rate solved from two measurements: how long a coupled mode takes to rise, and how much energy
  free neighbours add to a strike. Each instrument has its own small mistuning, drawn from measured beatings, so
  near-unisons beat (about 7 Hz).
- **The finger's click**: while it pushes, the fingertip accelerates the volume the sheet sweeps by F / ρs (Newton),
  which the shell radiates as a baffled source. Below a field's highest mode that is already in the modes; above it,
  the click is heard. A soft strike barely clicks, a hard one clearly does.
- **The level** is physics too: the reference tap on the ding gives 74 dB SPL at 1 m, without any gain set by ear.
- **Stereo**: the ding sits in the middle, the lowest field of the ring in front of the player (struck with the right
  hand), then the scale goes up left, right, left... the highest at the back. Each field, and its click, is heard
  from its side; the stereo width is a setting.

## Build

You need **CMake 3.22+**, **Git** and a **C++17 compiler**. CMake downloads everything else: the GRM engine
([cppgrm](https://github.com/innermost47/cppgrm), one header) and [JUCE](https://juce.com) 8 for the plugin. It has
been built and tested on **Windows** (Visual Studio 2022); JUCE also builds on macOS and Linux, but this project has
not been tried there yet.

### Windows

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community is free) with the **Desktop development
with C++** workload, [CMake](https://cmake.org/download/) and [Git](https://git-scm.com/). Then, in a terminal:

```bat
git clone https://github.com/innermost47/grm-hand-pan.git
cd grm-hand-pan
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

The first configure downloads JUCE (a few minutes). Then:

- the **VST3** is `build\GRMHandPan_artefacts\Release\VST3\GRM Hand Pan.vst3`: copy the whole `GRM Hand Pan.vst3`
  folder to `C:\Program Files\Common Files\VST3` and rescan your plugins;
- the **standalone** is `build\GRMHandPan_artefacts\Release\Standalone\GRM Hand Pan.exe`: open it, choose your audio
  device and MIDI input in its settings.

### macOS and Linux (untried)

```sh
git clone https://github.com/innermost47/grm-hand-pan.git
cd grm-hand-pan
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Linux, JUCE needs its usual development packages (ALSA, X11, Freetype... see JUCE's
[Linux dependencies](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md)). Reports of
how it goes are welcome.

### Tests and offline renders

Add `-DGRMH_BUILD_TESTS=ON` to the configure line to build the tests and the tools (`-DGRMH_BUILD_PLUGIN=OFF` skips
JUCE and the plugin):

```bat
cmake -B build -DGRMH_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release
```

Then render (the paths are those of a Visual Studio build; with a single-configuration generator, drop `Release\`):

```bat
mkdir renders
build\Release\render_scale renders\handpan_d_kurd.wav
build\Release\render_melody renders\handpan_melody.wav
build\Release\render_melody renders\handpan_melody_lap.wav --lap
```

- `render_scale`: a D Kurd up and down its scale, the ding soft to loud, a chord.
- `render_melody`: a short piece, the ding as a pulse, rolls, flams, a sung phrase; `--lap` holds the instrument on
  the lap (the air mode drops from 85 to 74 Hz).

The renderings are stereo. The other tools: `measure_recordings <folder> <notes...>` measures recorded notes against
the model with the meter of the tests, `probe_sympathy` shows how the sympathy network sees its unknown,
`probe_click` compares the attack of the recordings with the model's.

## Tests

Every network is checked against the measurements it comes from, and every test was seen failing on a deliberate
error before it was kept.

| ctest name | Checks |
| --- | --- |
| `grm_engine_solves` | the build and the GRM engine |
| `sourced_constants` | the published data and the spreads declared for them |
| `tone_field_network` | plate frequency equations (Bessel), the loss law against the measured decays, modes against measured fields |
| `cavity_network` | speed of sound, Helmholtz and radiation formulas, the lap lowering the air mode |
| `strike_network` | the impact, the hardening pulp, the force pulse's spectrum, louder is brighter |
| `sound_engine` | partials at the model's frequencies, decay, the air mode, the level, the lock-free mailbox |
| `mode_radiation` | monopole, dipole and quadrupole fields against their compact and incoherent limits |
| `mode_participation` | the calibrated instrument against the recorded notes |
| `sympathetic_fields` | energy kept by the exchange, the coupling against its measurements, the beatings |
| `finger_click` | the contact pulse, Newton over a baffle, the click left to the modes below them, louder is brighter |
| `field_layout` | where the fields sit and sound, constant-power stereo gains |
| `plugin_as_hosted` | the plugin called as a host would: silent while it solves, MIDI to sound, stereo, stop and play again, state |

## Layout

- `constants/`: the measured data, each value with its source and its spread (the spread is its sigma in the
  networks), and the choices that are not measurements (listening distance, full scale, MIDI velocity curve).
- `models/`: the structs that travel between the parts.
- `core/`: the networks, the model, the real-time engine (voices, cavity, sympathy), the offline renderer. No JUCE:
  everything is tested and rendered without a host.
- `plugin/`: the JUCE plugin, thin: parameters, MIDI, the solver thread, the engine in `processBlock`.
- `tools/`: renderers and measuring tools. `tests/`: one test per part.

## Sources

- F. Rohner, S. Schärer, "History, development and tuning of the HANG", Proc. ISMA 2007.
- A. Morrison, T. D. Rossing, "Modes of vibration and sound radiation from the Hang", Archives of Acoustics 32(3),
  551-560 (2007).
- D. Wessel, A. Morrison, T. D. Rossing, "Sound of the HANG", Proc. Mtgs. Acoust. 4, 035002 (2008).
- E. Alon, "Analysis and synthesis of the handpan sound", MSc by research, University of York (2015): the decay times,
  mode levels, beatings and coupling measurements this model is solved against.
- PANArt, the Hang book and the Hang Urgu product page (the air mode, the vessel's size).
- A. W. Leissa, "Vibration of plates", NASA SP-160 (1969). L. E. Kinsler, A. R. Frey et al., "Fundamentals of
  Acoustics" (2000). EN 1993-1-1 and EN 1991-1-1 (structural steel).
- A. Z. Hajian, R. D. Howe, "Identification of the mechanical impedance at the human finger tip", J. Biomech. Eng.
  (1997). D. L. Jindrich et al., "Non-linear viscoelastic models predict fingertip pulp force-displacement
  characteristics during voluntary tapping", J. Biomech. (2003), and "Finger joint impedance during tapping on a
  computer keyswitch" (2004). P. L. Kuo et al., "Finger joint coordination during tapping", J. Biomech. (2006).
- J. Dallard, "Modélisation du doigt dans un contexte de manipulation fine", PhD thesis, Université Claude Bernard
  Lyon 1 (2016): the fingertip pulp measured by MRI.
- C. Xu, "Modal Synthesis of the Handpan Sound", MUMT 307, McGill University (2022): the recorded notes the strike is
  calibrated on (their levels are in `constants/recordings.hpp`; the recordings themselves are not redistributed).

## The GRM family

- **[grm-core](https://github.com/innermost47/grm-core)**: the Global Relational Model and the research it comes
  from.
- **[cppgrm](https://github.com/innermost47/cppgrm)**: the GRM engine, one C++17 header with no dependency.
- **[pygrm](https://pypi.org/project/pygrm/)**: the same engine as a Python package.
- **[GRM Guitar](https://github.com/innermost47/grm-guitar-vst)**: a guitar built the same way, whose architecture
  this project follows.

## License

AGPL-3.0-or-later, see [LICENSE](LICENSE).

Hang® is a registered trademark of PANArt Hangbau AG. This project models the handpan family of instruments; it is
not affiliated with or endorsed by PANArt.
