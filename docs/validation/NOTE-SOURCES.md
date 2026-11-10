# Note pool and accepted positions

All pitches are concert pitches in scientific pitch notation: middle C is C4
(MIDI 60). Switching clef changes the engraved clef only; it does not transpose.
The pool contains every chromatic pitch from B-flat 2 through D4. New rounds
default to the inclusive B-flat 2 through B-flat 3 octave (MIDI 46–58).

| Pitch | Picker | MIDI | Lomse token | Reference position | Also accepted |
| --- | --- | ---: | --- | ---: | ---: |
| D4 | D4 | 62 | `d4` | 1 | 4 |
| C-sharp 4 | C#4 | 61 | `+c4` | 2 | 5 |
| C4 | C4 | 60 | `c4` | 3 | 6 |
| B3 | B3 | 59 | `b3` | 4 | 7 |
| B-flat 3 | Bb3 | 58 | `-b3` | 5 | 1 |
| A3 | A3 | 57 | `a3` | 6 | 2 |
| A-flat 3 | Ab3 | 56 | `-a3` | 7 | 3 |
| G3 | G3 | 55 | `g3` | 4 | — |
| F-sharp 3 | F#3 | 54 | `+f3` | 5 | — |
| F3 | F3 | 53 | `f3` | 1 | 6 |
| E3 | E3 | 52 | `e3` | 2 | 7 |
| E-flat 3 | Eb3 | 51 | `-e3` | 3 | — |
| D3 | D3 | 50 | `d3` | 4 | — |
| C-sharp 3 | C#3 | 49 | `+c3` | 5 | — |
| C3 | C3 | 48 | `c3` | 6 | — |
| B2 | B2 | 47 | `b2` | 7 | — |
| B-flat 2 | Bb2 | 46 | `-b2` | 1 | — |

Position references use the ordinary second through sixth partials of a straight
B-flat tenor trombone, without valves. The original D4 through A-flat 3
references retain their descending fifth-partial sequence. Lower references
use the chart's main open-slide position: B-flat 2 is on the second partial;
B2 through F3 use the third; F-sharp 3 and G3 use the fourth. F3 in sixth and
E3 in seventh are fourth-partial alternates. A player may use any listed
accepted position; the game does not measure intonation.

Primary sources checked on 2026-11-10:

- [Matt Waters's concert-pitch slide position chart](https://basstrombone.co.uk/resources/trombone-slide-position-chart/),
  specifically its bass-clef “Low register E2 to E3” and “Middle register F3 to F4”
  tables. These give the lower additions and all ordinary alternates in the table
  above. F-sharp 3 and C-sharp 3 are the chart's enharmonic G-flat 3 and D-flat 3.
  Only its open-slide entries are used; its separate trigger and B-flat treble
  transposition charts do not apply. The source charts are available under
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/); credit: Matt Waters,
  basstrombone.co.uk. The game's naming and reference preferences are stated here.
- [Norlan Bewley's position charts](https://norlanbewley.com/bewleymusic/trombone-slide-position-chart/)
  and his [F-attachment chart](https://www.norlanbewley.com/music/trombone-chart-fnew.pdf).
  These remain the original upper-pool references, checked on 2026-11-09. The
  latter's G-sharp/A-flat through D row lists 3/7, 2/6, 1/5, 4/7, 3/6, 2/5,
  and 1/4/7 respectively; only ordinary, untriggered positions are used.
- [Yamaha's explanation of slide positions and partials](https://www.yamaha.com/en/musical_instrument_guide/trombone/mechanism/mechanism004.html)
  supplies the seven-position and multiple-pitches-per-position context.

D4 in seventh position is deliberately excluded: it relies on the flat seventh
partial and requires a substantial tuning adjustment. No seventh partial,
trigger, upper harmonic, or microtonal answer is included. The accepted ordinary
partials still require the usual small tuning corrections on a real instrument.

Lomse tokens in the table specify each octave and accidental explicitly.
The native note-data tests independently list every MIDI pitch, token,
reference, and complete accepted-position set from the source table.

Each picker toggle controls one exact pitch, including its octave. A pitch is
eligible only when its toggle is enabled and its MIDI number lies within the
inclusive selected range. Toggles retain their values when the range changes;
an enabled pitch outside the range stays excluded. Internally, bit `i` of
`Settings::enabledNotes` corresponds to `kNotes[i]` in descending D4–B-flat 2
order. The default mask enables all 17 toggles, giving 13 eligible default
pitches. Unknown mask bits are discarded when settings are normalized. An
empty intersection keeps the quiz at settings and cannot start or replay a round.
