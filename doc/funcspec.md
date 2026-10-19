# Functional Specification

## Overview

tix is a text editor focused on editing code in any programming language. It has the next values in order of importance:

- Correctness
- Reliability
- Performance:low Memory footprint
- All the UI should be text like
    - Keyboard navigation first, mouse navigation second.
    - Based on vim modes: vim everywhere, vim first
- The screen space should be optimized.
- All the elements should keep the same font size

## Scenarios

### Scenario 1: Carlos
Carlos has not experience with vim or mode-based editors.

### Scenario 2: Andrew
Andrew has extended experience with neovim.

## Goals

- Syntax highlighting
- Multi-color fonts
- All of Unicode, including combining characters and right-to-left text like Arabic
- Glyphs that can take up several cells
- Line wrapping
- Re-flowing line wrapping on window resize

## Non Goals

This editor will not support
- Scripting language for extensions

## Screens Specification

### Docks

- The editor has the next visual blocks
  - Three docks:
    - Main dock(mdock): the files are open here.
    - Right dock(rdock): for the file tree and other tools
    - Bottom dock(bdock): for the terminal.
  - One title bar
    - menu
    - tabs for the active dock
    - root folder name
    - status icons
      - mode
  - Command pad: you see the command to be executed in on line
  - Picker: a pop up with a search area
- All the docks are based on tabs
- The tabs of the active dock are showed on the title bar.
- docks could be hide
- Just the main dock could be split.
- There is a picker for the tabs in a dock
- Tabs can be pinned to a dock

### Actions

- Command palette: a picker
- Command prompt/pad: you can run the commands

### Pickers

- Just one picker with prefixes:
  - File: <no prefix>
  - Command: >
  - Fzf: @

### Navigation

## Modes

### Normal mode

- among docks: Tab should move the cursor like in a grid: increasing column first
- inside docks:
    - tab move the cursor like in a grid between elements: increasing column first
    - follow vim conventions: l move right, k move up, j move down, h move left
- buffer:
    - follow vim conventions: l move right, k move up, j move down, h move left
    - all buffer commands are defined in [Vim Command Specification](#vim-command-specification)

### Visual mode

### Insert mode

### Visual block mode

### Multi selection mode

## Vim Command Specification

Behaviour of the editing commands inside a buffer. Docks, tabs, pickers and the title bar are specified above and are not part of this section.

### Reference and conformance

- **Reference implementation**: Neovim (nvim) 0.12.6 started with `--clean`, so only built-in defaults. Every example in this section was run against it. Text marked *(docs)* comes from the Vim or nvim documentation and was not run.
- **Conformance order** when vi, Vim and nvim disagree:
  1. A behaviour that fixes a bug or an inconsistency wins, whoever made the fix (nvim, or a newer Vim).
  2. A deliberate nvim default wins over the Vim default (see [Where tix follows nvim or Vim over vi](#where-tix-follows-nvim-or-vim-over-vi)).
  3. Otherwise the Vim behaviour wins over vi.
- A command that is not listed here is not supported. An unsupported key sequence is a failed command (see [Command grammar and failure](#command-grammar-and-failure)).

### Notation used in examples

| Symbol | Meaning |
|---|---|
| `«x»` | The cursor is on the character `x`. |
| `«»` | The cursor is on an empty line. |
| `⏎` | Line break. Text after the last `⏎` is one more line, so `a⏎` is the two lines `a` and `` (empty). |
| `→` | A tab character. |
| `␠` | A space, where it would be hard to see (inside a register value). |
| `reg` | The unnamed register after the command: `text`(c) characterwise, `text`(L) linewise. |
| *fails* | The command fails: nothing changes. |

Example tables read as: **before** → **keys** → **after**.

### Data model

- **Buffer**: one or more lines. An empty buffer is one empty line. Deleting the last remaining line leaves one empty line.
- **Line**: a sequence of characters without the line terminator. An **empty line** has zero characters. A line with only blanks is **not** empty.
- **Character**: one grapheme cluster. `h`, `l`, `x`, `r`, `~` and the like act on whole clusters. Example: `e` + U+0301 is one character, so `x` deletes both, `l` steps over both, `b` lands on the `e`, `~` gives `É`.
- **Cursor**: a line and a character. In Normal mode it rests on a character, or on column 0 of an empty line. After every command it is clamped so it is never past the last character of the line. (Insert mode, and operator-pending `l`, may be one past the end.)
- **Screen column**: a tab advances to the next multiple of 8, a wide (East Asian) character takes 2 cells, everything else takes 1. The desired column (below), `|` and `<<` / `>>` work in screen columns. When a screen column falls inside a tab or a wide character, the cursor rests on that character.
- **Desired column** (`curswant`): the screen column `j`, `k`, `G`, `gg`, `dd` and the like try to keep. Any horizontal motion or edit sets it. `$` sets it to "end of line" and it stays that way until the next horizontal motion.
- **Blank**: space, tab, and the Unicode space separators. Verified: no-break space U+00A0 and ideographic space U+3000 are blanks.
- **First non-blank**: the first character that is not a blank, or the last character if the line is all blanks.
- **Character class** (word motions): blank; punctuation; keyword (letters, digits, `_`, and non-ASCII letters such as `é`); and separate classes for CJK ideographs, hiragana, katakana, hangul, emoji and CJK punctuation. A character of one class next to a character of another class is a word boundary. Verified: `foo漢字bar` is three words, `かなカナ` is two, `foo😀bar` is three, `foo。bar` is three, `a·b` is three.
- **Word**: a maximal run of characters of the same non-blank class on one line.
- **WORD**: a maximal run of non-blank characters on one line (every non-blank has the same class).
- **Word start**: the first character of a word, or an empty line.
- **Register**: a named slot holding text and a type: characterwise (c), linewise (L) or blockwise.

### Command grammar and failure

```
["{register}] [count] operator [count] motion | text-object
["{register}] [count] command
```

1. **Counts multiply**: `2d3w` is `d6w`. A `0` after a nonzero digit is part of the count (`10x`). A `0` that starts a count is the motion `0`.
2. **Doubling an operator** applies it to the current line, linewise: `dd`, `cc`, `yy`, `>>`, `<<`, `g~~` (or `g~g~`), `guu` (or `gugu`), `gUU` (or `gUgU`). A count `N` covers the current line and the `N-1` below it.
3. **`Esc`** cancels the pending count, register and operator and does nothing else. Verified: `3<Esc>x` and `d3<Esc>x` both delete exactly one character.
4. **A command fails** when it cannot do what was asked. A failed command changes nothing: no buffer change, no register write, no undo entry, no jumplist entry, the cursor stays. It cancels the pending operator and stops a running macro, including the remaining repeats. A failure shows a message and never aborts the editor.
5. **Count policy**. Every counted command follows one of two policies:
   - **Clamp**: go as far as possible. Fails only if it cannot move at all. Used by `h l w b e ge W B E gE j k + - _ $ G x X ~ J` (J with count 3 or more), and by the linewise doubled operators.
   - **Exact**: fails if the count cannot be met in full. Used by `f F t T`, `r`, `}`, `{`, `Ctrl+O` and `Ctrl+I`.
   - Moving "`N-1` lines down" (`N$`, `N_`, `Ndd`, `N>>`, `NJ` with N of 2 or less) fails only if `N` is more than 1 and there is no line below.

| Before | Keys | After |
|---|---|---|
| `«a»bc` | `5l` | `ab«c»` |
| `«a»⏎b` | `5j` | `a⏎«b»` |
| `«a»⏎b` | `5-` | *fails* |
| `«a»bc` | `5$` | *fails* (one line, nothing below) |
| `«a»bc⏎def⏎g` | `9$` | `abc⏎def⏎«g»` |
| `«a»bcabc` | `3fc` | *fails* (only two `c`) |
| `a⏎b⏎«c»` | `2dd` | *fails* (nothing below) |
| `a⏎«b»⏎c` | `3dd` | `«a»` (clamps, deletes `b` and `c`) |
| `a«b»c` | `3rx` | *fails* (only two characters) |
| `a«b»cde` | `9X` | `«b»cde` (deletes `a`) |
| `a«b»c` | `5x` | `«a»` (deletes `bc`) |

### Motions

A motion has a type. **E**: characterwise exclusive, the end character is not included. **I**: characterwise inclusive. **L**: linewise.

#### Left and right within a line

| Key | Motion | Type |
|---|---|---|
| `h` | `count` characters left. Never crosses to the previous line. | E |
| `l` | `count` characters right. Never crosses to the next line. After an operator it may go one past the last character, so `dl` and `x` can delete the last one. | E |
| `Backspace` | *(docs)* Like `h`, but crosses to the end of the previous line. | E |
| `Space` | *(docs)* Like `l`, but crosses to the start of the next line. | E |
| `0` | First character of the line. | E |
| `^` | First non-blank. | E |
| `$` | Last character, `count-1` lines down. On an empty line it stays. | I |
| `g_` | Last non-blank, `count-1` lines down. | I |
| `\|` | Screen column `count` (column 1 if omitted), clamped to the line. | E |

#### Up and down, and the desired column

| Key | Motion | Cursor column | Type |
|---|---|---|---|
| `j`, `Down` | `count` lines down. | desired column | L |
| `k`, `Up` | `count` lines up. | desired column | L |
| `+`, `Enter` | `count` lines down. | first non-blank | L |
| `-` | `count` lines up. | first non-blank | L |
| `_` | `count-1` lines down. | first non-blank | L |
| `G` | Line `count`, or the last line. | desired column | L |
| `gg` | Line `count`, or the first line. | desired column | L |
| `:{N}` | Line `N`. Out of range goes to the last line. `:0` goes to line 1. | desired column | L |
| `H` / `M` / `L` | *(docs)* Line `count` from the top of the window (first if omitted) / middle line / line `count` from the bottom (last if omitted). | desired column | L |

nvim has `nostartofline` on by default, so `G`, `gg` and `:{N}` keep the desired column instead of jumping to the first non-blank (as Vim does). tix follows nvim. Commands that always go to the first non-blank are `+ - _ Enter ^ I` and the mark motion `'`.

| Before | Keys | After |
|---|---|---|
| `abc«d»⏎ab⏎abcdef` | `jj` | `abcd⏎ab⏎abc«d»ef` (the short line does not lose the column) |
| `abc«d»⏎ab⏎abcdef` | `j` | `abcd⏎a«b»⏎abcdef` |
| `«a»bc⏎d⏎efghi` | `$jj` | `abc⏎d⏎efgh«i»` (`$` sticks to the line end) |
| `abc«d»⏎ab⏎abcdef` | `jhj` | `abcd⏎ab⏎«a»bcdef` (`h` resets the column) |
| `→«x»⏎abcdefghijklmnop` | `j` | `→x⏎abcdefgh«i»jklmnop` (the tab is 8 screen columns) |
| `abcdefg«h»⏎→x` | `j` | `abcdefgh⏎«→»x` (column 7 is inside the tab) |
| `abc«d»⏎漢字漢字` | `j` | cursor on the first `字` (it covers columns 2 and 3) |
| `«a»⏎  b⏎  c` | `G` | `a⏎  b⏎« » c` (column 0 is kept, not the first non-blank) |
| `«a»⏎  b` | `2_` | `a⏎  «b»` |
| `a⏎«b»⏎  c` | `+` | `a⏎b⏎  «c»` |

#### Word motions

`w`, `b`, `e`, `ge` work on words. `W`, `B`, `E`, `gE` are identical but use WORDs.

Each is defined as a step. A counted motion repeats the step. If a step cannot move, the repetition stops, and the command fails only if the very first step could not move (clamp policy).

**`w`: next word start.**
1. If the cursor is on a non-blank, move right to the end of its run (the first character with another class, or the line end).
2. Move right over blanks and line breaks. Stop at the first non-blank character, **or on an empty line reached by crossing a line break**.
3. If the buffer ends first, move to the last character of the buffer (or the empty last line). If the cursor is already there, the step fails.

**`b`: previous word start.** The mirror of `w`.
1. Move left one position, crossing a line break if needed.
2. Move left over blanks and line breaks. Stop at a non-blank character, or on an empty line.
3. If on a non-blank, move left to the start of its run.
4. If the buffer start comes first, go to line 1 column 0. If already there, the step fails.

**`e`: next word end.**
1. Move right one position, crossing a line break if needed.
2. Move right over blanks and line breaks, **including empty lines** (an empty line is not a word end).
3. Move right to the last character of the run.
4. If the buffer ends first, move to the last position of the buffer. If already there, the step fails.

**`ge`: previous word end.**
1. If on a non-blank, move left out of its run.
2. Keep moving left (first one position, if step 1 did not run) over blanks and line breaks. Stop on a non-blank, or on an empty line (an empty line **is** a stop for `ge`).
3. If the buffer start comes first, go to line 1 column 0. If already there, the step fails.

Consequences, all verified:

- **Every empty line is one stop** for `w`, `b` and `ge`, so `n` consecutive empty lines take `n` steps. `e` skips them.
- **A line with only blanks is never a stop.** It is crossed like any other blank run.
- The step never stops on a blank, except at the buffer start or end.

| Before | Keys | After |
|---|---|---|
| `«a»⏎b` | `w` | `a⏎«b»` |
| `«a»⏎⏎b` | `w` | `a⏎«»⏎b` |
| `«a»⏎⏎⏎b` | `w` | `a⏎«»⏎⏎b` |
| `«a»⏎⏎⏎b` | `2w` | `a⏎⏎«»⏎b` |
| `«a»⏎⏎⏎b` | `3w` | `a⏎⏎⏎«b»` |
| `«a»⏎⏎⏎b` | `9w` | `a⏎⏎⏎«b»` (clamps) |
| `a⏎«»⏎⏎b` | `w` | `a⏎⏎«»⏎b` |
| `a⏎«»⏎b` | `w` | `a⏎⏎«b»` |
| `a⏎«»⏎  x` | `w` | `a⏎⏎  «x»` |
| `«a»⏎  ⏎b` | `w` | `a⏎  ⏎«b»` (blank-only line skipped) |
| `«a»⏎  ⏎  ⏎b` | `w` | `a⏎  ⏎  ⏎«b»` |
| `«a»  ⏎b` | `w` | `a  ⏎«b»` |
| `foo«.»bar` | `w` | `foo.«b»ar` |
| `«f»oo.bar` | `w` | `foo«.»bar` |
| `«f»oo.bar` | `W` | `foo.ba«r»` (end of buffer) |
| `«f»oo..bar` | `2w` | `foo..«b»ar` |
| `«»⏎a` | `w` | `⏎«a»` |
| `«»⏎⏎a` | `w` | `⏎«»⏎a` |
| `a  «b»` | `w` | *fails* (already on the last character) |
| `«a»  ` | `w` | `a « »` (last character, a blank) |
| `a  «b»⏎` | `w` | `a  b⏎«»` (the empty last line) |
| `«»` | `w` | *fails* |
| `«a» b` | `5w` | `a «b»` |
| `a b «c»` | `b` | `a «b» c` |
| `a b «c»` | `9b` | `«a» b c` |
| `a⏎⏎⏎«b»` | `b` | `a⏎⏎«»⏎b` |
| `a⏎⏎⏎«b»` | `2b` | `a⏎«»⏎⏎b` |
| `a⏎⏎⏎«b»` | `3b` | `«a»⏎⏎⏎b` |
| `a⏎  ⏎  ⏎«b»` | `b` | `«a»⏎  ⏎  ⏎b` |
| `a⏎«»⏎b` | `b` | `«a»⏎⏎b` |
| `  «a»` | `b` | `« » a` (buffer start) |
| `«a»` | `b` | *fails* |
| `foo.«b»ar` | `b` | `foo«.»bar` |
| `foo.«b»ar` | `B` | `«f»oo.bar` |
| `«a»b cd` | `e` | `a«b» cd` |
| `a«b» cd` | `e` | `ab c«d»` |
| `«a»⏎⏎b` | `e` | `a⏎⏎«b»` (empty lines skipped) |
| `a«b»⏎  ⏎c` | `e` | `ab⏎  ⏎«c»` |
| `a  ⏎«»⏎c` | `e` | `a  ⏎⏎«c»` |
| `a «b»` | `e` | *fails* |
| `a «b»⏎` | `e` | `a b⏎«»` |
| `«f»oo.bar` | `e` | `fo«o».bar` |
| `«f»oo.bar` | `2e` | `foo«.»bar` |
| `«f»oo.bar` | `E` | `foo.ba«r»` |
| `a b «c»` | `ge` | `a «b» c` |
| `a⏎⏎«b»` | `ge` | `a⏎«»⏎b` |
| `a⏎⏎⏎«b»` | `2ge` | `a⏎«»⏎⏎b` |
| `a⏎  ⏎«b»` | `ge` | `«a»⏎  ⏎b` |
| `a⏎«»⏎b` | `ge` | `«a»⏎⏎b` |
| `a.«b»` | `ge` | `a«.»b` |
| `a.«b»` | `gE` | `«a».b` |
| `  «a»` | `ge` | `« » a` |
| `«a»b` | `ge` | *fails* |

#### Find on the line

| Key | Motion | Type |
|---|---|---|
| `f{c}` | To the `count`-th `{c}` right of the cursor, on this line. | I |
| `F{c}` | To the `count`-th `{c}` left of the cursor. | E |
| `t{c}` | To the character before the `count`-th `{c}` to the right. | I |
| `T{c}` | To the character after the `count`-th `{c}` to the left. | E |
| `;` | Repeat the last `f F t T`. | as repeated |
| `,` | Repeat it in the opposite direction. | as repeated |

- The search never leaves the line. No match, or fewer than `count` matches, fails (exact policy).
- **`;` and `,` after `t` / `T` do not get stuck.** If the target would be the character next to the cursor, it is skipped and the next match is used. The skip applies only when the count is 1. (Vim default. vi stays put.)
- `{c}` is one character. Digraphs and keymaps are not supported.

| Before | Keys | After |
|---|---|---|
| `«a»bcabc` | `fc` | `ab«c»abc` |
| `«a»bcabc` | `2fc` | `abcab«c»` |
| `«a»bcabc` | `fc;` | `abcab«c»` |
| `«a»bcabc` | `fc;;` | `abcab«c»` (the second `;` fails, the cursor stays) |
| `«a»bcabc` | `fc;,` | `ab«c»abc` |
| `abc«a»bc` | `Fa` | `«a»bcabc` |
| `«a»bcabc` | `tc` | `a«b»cabc` |
| `«a»bcabc` | `tc;` | `abca«b»c` |
| `«a»bcabcabc` | `tc2;` | `abca«b»cabc` (with a count the adjacent `c` is counted) |
| `abca«b»c` | `Tb` | `ab«c»abc` (stops just after the `b` at index 1) |

#### Paragraphs, sentences and brackets

| Key | Motion | Type |
|---|---|---|
| `}` / `{` | `count` paragraphs forward / backward. | E |
| `)` / `(` | *(docs)* `count` sentences forward / backward. | E |
| `%` | Jump to the match of the first `( ) [ ] { }` at or after the cursor on this line. With a count `N` (1 to 100) it goes to the line `N`% of the way through the buffer instead: line `(N * line_count + 99) / 100`. Fails if there is no bracket or no match. | I |

**`}` and `{`** (paragraph = lines separated by empty lines; a blank-only line is not a separator):
1. Skip empty lines (so a run of empty lines counts as one separator).
2. Skip non-empty lines.
3. Stop on the next empty line, at column 0. If the buffer ends first, stop on the last character (`}`) or at line 1 column 0 (`{`).
4. Repeat `count` times. **If the buffer end is reached while count remains, the command fails** (exact policy). Landing on the buffer end with the last count is fine.

| Before | Keys | After |
|---|---|---|
| `«a»⏎b⏎⏎c⏎⏎d` | `}` | `a⏎b⏎«»⏎c⏎⏎d` |
| `«a»⏎b⏎⏎c⏎⏎d` | `2}` | `a⏎b⏎⏎c⏎«»⏎d` |
| `«a»⏎b⏎⏎c⏎⏎d` | `9}` | *fails* |
| `«a»⏎b⏎⏎c` | `2}` | `a⏎b⏎⏎«c»` |
| `«a»⏎b⏎⏎c` | `3}` | *fails* |
| `a⏎«»⏎⏎b` | `}` | `a⏎⏎⏎«b»` (starts by skipping the empty run) |
| `«a»⏎  ⏎b` | `}` | `a⏎  ⏎«b»` (a blank-only line is not a separator) |
| `a⏎b⏎⏎c⏎⏎«d»` | `{` | `a⏎b⏎⏎c⏎«»⏎d` |
| `«(»a [b] c)` | `%` | `(a [b] c«)»` |
| `(a [«b»] c)` | `%` | `(a «[»b] c)` (the first bracket at or after the cursor is `]`) |
| `«a» (b)` | `%` | `a (b«)»` |
| `(a«)»` | `%` | `«(»a)` |
| `(a) «b»` | `%` | *fails* |
| `«a»⏎b⏎c⏎d` | `50%` | `a⏎«b»⏎c⏎d` |

#### Search and marks

| Key | Motion | Type |
|---|---|---|
| `/{pattern}` `Enter` | Next match forward, wrapping at the buffer end. | E |
| `?{pattern}` `Enter` | Next match backward, wrapping. | E |
| `n` / `N` | Repeat the last search in the same / the opposite direction of the last `/` or `?`. | E |
| `*` / `#` | Search forward / backward for the whole keyword under the cursor, or the first keyword after it on the line. | E |
| `` `{m} `` | The exact position of mark `{m}`. | E |
| `'{m}` | The first non-blank of the line of mark `{m}`. | L |

See [Search](#search) and [Marks and the jumplist](#marks-and-the-jumplist).

#### Scrolling *(docs)*

The view moves. The cursor stays on the same text unless it would leave the window.

| Key | Action |
|---|---|
| `Ctrl+E` / `Ctrl+Y` | Scroll `count` lines down / up. |
| `Ctrl+D` / `Ctrl+U` | Scroll half a window down / up. The cursor moves the same number of lines. `count` sets the amount for later uses. |
| `Ctrl+F` / `Ctrl+B` | Scroll a window forward / backward. |
| `zt` / `zz` / `zb` | Put the cursor line at the top / middle / bottom of the window. The cursor does not move. |

### Operators

An operator takes a **range** from a motion or a text object and acts on it.

| Key | Operator |
|---|---|
| `d` | Delete the range into a register. |
| `c` | Delete the range into a register, then enter Insert mode. |
| `y` | Yank the range. Does not change the buffer. |
| `>` / `<` | Indent / unindent the lines of the range. |
| `g~` / `gu` / `gU` | Toggle / lower / upper case of the range. |

| Key | Same as | Notes |
|---|---|---|
| `x` / `X` | `dl` / `dh` | Fail on an empty line / at column 0. `5x` clamps to the line end. |
| `D` | `d$` | A count `N` also deletes the next `N-1` lines (`2D`: to the end of this line and all of the next). |
| `C` | `c$` | Same count rule. |
| `s` | `cl` | |
| `S` | `cc` | |
| `Y` | `y$` | nvim. vi and Vim: `yy`. |

#### How a range is built

When an operator is followed by a motion, the range goes from the cursor to the motion's target and is adjusted by these rules, in this order.

**R1. A failed motion does nothing.** No change, no register, no undo entry.

**R2. `w` / `W` does not join lines.** If the last `w` step starts on a non-blank and the run ends at the last character of its line, the range ends at the end of that line. The lines below are not touched, whatever they contain.

**R3. An exclusive end at column 0 moves up.** If the motion is exclusive and its end is at column 0 of a line below the start line, the end moves to the end of the previous line and the motion becomes inclusive. If the start is also at or before the first non-blank of its line, the motion becomes **linewise**.

**R4. `d` only: a multi-line characterwise delete of whole lines becomes linewise.** After R3, if the range spans more than one line, only blanks come before the start on its line and only blanks come after the end on its line, the delete is linewise. This removes the empty line that would be left behind. It does not apply to `c` or `y`, and (verified) not to `%`.

**R5. Registers** follow the rules in [Registers](#registers).

**R6. Cursor after the operator**:
- Characterwise `d` and `c`: the start of the range, clamped to the line.
- Linewise `d`: on the line that took the place of the first deleted line (or the new last line), at the **desired column**, clamped. nvim keeps the column here too (`nostartofline`).
- `y`: characterwise, the start of the range. Linewise, the first line of the range, same column. So `yk`, `yb`, `y0`, `yiw` (from the end of a word) move the cursor. `yj`, `yy`, `y$` do not.
- `>` and `<`: the first line of the range, at the desired column, clamped.

| Before | Keys | After | reg |
|---|---|---|---|
| `«a»a bb` | `dw` | `«b»b` | `aa `(c) |
| `aa «b»b` | `dw` | `aa« »` | `bb`(c) |
| `aa «b»b⏎c` | `dw` | `aa« »⏎c` | `bb`(c) (R2) |
| `aa «b»b  ⏎c` | `dw` | `aa« »⏎c` | `bb  `(c) (R3, the blanks are included) |
| `«a»a⏎⏎b` | `dw` | `«»⏎⏎b` | `aa`(c) (R2) |
| `«a»a⏎  ⏎b` | `dw` | `«»⏎  ⏎b` | `aa`(c) (R2) |
| `a «b»⏎  ⏎c` | `dw` | `a« »⏎  ⏎c` | `b`(c) (R2) |
| `«»⏎a` | `dw` | `«a»` | `⏎`(L) (R3: deletes the empty line) |
| `a⏎«»⏎b` | `dw` | `a⏎«b»` | `⏎`(L) |
| `a⏎«»⏎⏎b` | `dw` | `a⏎«»⏎b` | `⏎`(L) |
| `«a»⏎⏎⏎b` | `d2w` | `«»⏎b` | `a⏎⏎`(L) |
| `«a»⏎⏎⏎b` | `d3w` | `«b»` | `a⏎⏎⏎`(L) |
| `«a»a⏎bb` | `d2w` | `«»` | `aa⏎bb⏎`(L) (R4) |
| `a «b»⏎c d` | `d2w` | `a «d»` | `b⏎c `(c) |
| `a «b»⏎c d` | `dW` | `a« »⏎c d` | `b`(c) (R2) |
| `a «b»⏎c d` | `d3w` | `a« »` | `b⏎c d`(c) |
| `«a» b c` | `d5w` | `«»` | `a b c`(c) (at the buffer end the operator succeeds) |
| `aa«a»` | `de` | `a«a»` | `a`(c) (bare `e` would fail here) |
| `a  ⏎«»` | `dw` | *fails* (empty last line, nothing to delete) | |
| `aa⏎«b»b` | `db` | `«b»b` | `aa⏎`(L) (R3) |
| `a«a» bb` | `de` | `«a»` | `a bb`(c) |
| `aa b«b»` | `dge` | `«a»` | `a bb`(c) (`ge` is inclusive) |
| `aa⏎⏎«b»` | `dge` | `«a»a` | `⏎b⏎`(L) (R4) |
| `a⏎«b»⏎⏎c` | `d}` | `a⏎«»⏎c` | `b⏎`(L) (R3) |
| `a⏎«b»⏎c⏎⏎d` | `d}` | `a⏎«»⏎d` | `b⏎c⏎`(L) |
| `a⏎b⏎«c»` | `d{` | `«c»` | `a⏎b⏎`(L) |
| `a⏎⏎«b»` | `d{` | `a⏎«b»` | `⏎`(L) |
| `«a»a⏎bb` | `d}` | `«»` | `aa⏎bb⏎`(L) (R4) |
| `a «a»⏎bb` | `d}` | `a« »` | `a⏎bb`(c) (the start is not in the indent) |
| `«a» b⏎c d` | `d/c<CR>` | `«c» d` | `a b⏎`(L) (R3) |
| `a «b»⏎  c` | `d/c<CR>` | `a «c»` | `b⏎  `(c) (the end is not at column 0) |
| `«a»a⏎bb⏎cc` | `d2e` | `«c»c` | `aa⏎bb⏎`(L) (R4) |
| `a «a»⏎bb⏎cc` | `d3e` | `a« »` | `a⏎bb⏎cc`(c) |
| `«a»a⏎bb⏎cc` | `y2e` | unchanged | `aa⏎bb`(c) (R4 is for `d` only) |
| `«a»a⏎bb⏎cc` | `c3e<Esc>` | `«»` | `aa⏎bb⏎cc`(c) |
| `«(»⏎a⏎)` | `d%` | `«»` | `(⏎a⏎)`(c) (`%` is exempt from R4) |

`yk` from `a⏎«b»` yanks `a⏎b⏎`(L) and the cursor moves to line 1 (`«a»⏎b`). `yj`, `yy` and `y$` leave the cursor where it is.

#### `c`

- **`cw` / `cW` on a non-blank behaves like `ce` / `cE`**: it changes up to the end of the word and keeps the blanks after it. **If the cursor is on the last character of the word, only that character changes** (`de` there would go on to the next word).
- **`cw` on a blank behaves like `dw`**: it changes the whole run of blanks (R2 applies).
- **`cw` on an empty line behaves like `cc`.**
- **`cc` / `S` keep the indent** of the first line (autoindent), and an indent-only line is emptied when you press `Esc`. On an empty line they only enter Insert mode, but still write `⏎`(L) to the register.

| Before | Keys | After | reg |
|---|---|---|---|
| `«a»a bb` | `cw<Esc>` | `« »bb` | `aa`(c) |
| `«a»a bb` | `cwX<Esc>` | `«X» bb` | `aa`(c) |
| `«a»a  bb` | `ce<Esc>` | `« » bb` | `aa`(c) |
| `a«a»  bb` | `cw<Esc>` | `«a»  bb` | `a`(c) |
| `«a»a  bb` | `c2w<Esc>` | `«»` | `aa  bb`(c) |
| `a« » bb` | `cw<Esc>` | `«a»bb` | `␠␠`(c) |
| `aa«.»bb` | `cw<Esc>` | `a«a»bb` | `.`(c) |
| `«a»a⏎bb` | `cw<Esc>` | `«»⏎bb` | `aa`(c) |
| `«»⏎bb` | `cw<Esc>` | `«»⏎bb` | `⏎`(L) |
| `a «b»` | `cw<Esc>` | `a« »` | `b`(c) |
| `  «a»` | `cc<Esc>` | `«»` | `  a⏎`(L) |
| `  «a»⏎b` | `ccX<Esc>` | `  «X»⏎b` | `  a⏎`(L) |
| `«»` | `ccX<Esc>` | `«X»` | `⏎`(L) |
| `a«b»c` | `SX<Esc>` | `«X»` | `abc⏎`(L) |
| `a«b»cd` | `2sX<Esc>` | `a«X»d` | `bc`(c) |
| `a«b»c⏎d⏎e` | `2CX<Esc>` | `a«X»⏎e` | `bc⏎d`(c) |

#### `d` with a count or a line motion

Linewise ranges follow the count policy: `dj` and `dd` with a count fail only if there is no line below. `dk` fails only on line 1.

| Before | Keys | After | reg |
|---|---|---|---|
| `«a»⏎b⏎c` | `dd` | `«b»⏎c` | `a⏎`(L) |
| `a⏎b⏎«c»` | `dd` | `a⏎«b»` | `c⏎`(L) |
| `«a»` | `dd` | `«»` | `a⏎`(L) |
| `a⏎«b»⏎  c⏎d` | `dd` | `a⏎« » c⏎d` | `b⏎`(L) (the column 0 is kept) |
| `«a»⏎b⏎c` | `9dd` | `«»` | `a⏎b⏎c⏎`(L) |
| `a⏎b⏎«c»` | `dj` | *fails* | |
| `«a»⏎b⏎c` | `d5j` | `«»` | `a⏎b⏎c⏎`(L) |
| `«a»⏎b⏎c` | `d5k` | *fails* | |
| `a⏎«b»⏎c⏎d` | `dG` | `«a»` | `b⏎c⏎d⏎`(L) |
| `a⏎«b»⏎c⏎d` | `dgg` | `«c»⏎d` | `a⏎b⏎`(L) |
| `a⏎«b»⏎c⏎d` | `d3G` | `a⏎«d»` | `b⏎c⏎`(L) |
| `a«b»c` | `d$` | `«a»` | `bc`(c) |
| `a«b»c` | `d0` | `«b»c` | `a`(c) |
| `  a«b»c` | `d^` | `  «b»c` | `a`(c) |
| `«a»bc⏎def⏎g` | `d2$` | `«g»` | `abc⏎def⏎`(L) (R4) |
| `a«b»c⏎d⏎e` | `2D` | `«a»⏎e` | `bc⏎d`(c) |
| `abc«d»e` | `d3\|` | `ab«d»e` | `c`(c) (`\|` is exclusive) |
| `abc«d»e` | `dg_` | `ab«c»` | `de`(c) |
| `«a»bcabc` | `dfc` | `«a»bc` | `abc`(c) |
| `«a»bcabc` | `dtc` | `«c»abc` | `ab`(c) |
| `abcabc«d»` | `dFa` | `abc«d»` | `abc`(c) |
| `abcabc«d»` | `dTa` | `abca«d»` | `bc`(c) |
| `a«b»c` | `x` | `a«c»` | `b`(c) |
| `ab«c»` | `x` | `a«b»` | `c`(c) (the cursor is clamped) |
| `«»` | `x` | *fails* | |
| `a«b»c` | `X` | `«b»c` | `a`(c) |
| `«a»bc` | `X` | *fails* | |
| `a«b»cde` | `3x` | `a«e»` | `bcd`(c) |

#### Indent and case

- `>` adds one shiftwidth of indent to every non-empty line of the range. `<` removes up to one shiftwidth. Empty lines are never indented. Default shiftwidth is 8 and the indent is made of tabs (nvim defaults, `noexpandtab`); see [Open decisions](#open-decisions).
- In Normal mode a count never changes the amount: it is the number of lines (`>>`) or it multiplies the motion. **In Visual mode a count is the number of shifts**: `Vj3>` indents 3 times.
- The cursor goes to the first line of the range at the desired column (screen column, clamped).
- `g~`, `gu`, `gU` leave the cursor at the start of the range.

| Before | Keys | After |
|---|---|---|
| `«a»` | `>>` | `«→»a` (column 0 is kept, and that is the tab) |
| `«a»⏎b` | `2>>` | `«→»a⏎→b` |
| `«a»⏎b⏎c` | `2>2j` | `«→»a⏎→b⏎→c` (the 2 multiplies the motion: one shift, 3 lines clamped) |
| `«a»⏎⏎c` | `3>>` | `«→»a⏎⏎→c` |
| `«a»⏎b⏎c` | `Vj3>` | `«→»→→a⏎→→→b⏎c` |
| `  «a»` | `<<` | `«a»` |
| `    «a»` | `<<` | `«a»` (removes less than a shiftwidth if there is less) |
| `a«b»c` | `2>>` | *fails* (nothing below) |
| `«a»Bc` | `g~~` | `«A»bC` |
| `«a»Bc d` | `gUw` | `«A»BC d` |
| `«a»Bc d` | `guu` | `«a»bc d` |
| `«a»bc` | `~` | `A«b»c` |
| `«a»bc` | `9~` | `AB«C»` (`~` takes no motion, toggles `count` characters, clamps) |

### Text objects

Used after an operator, or in Visual mode. `i` is "inner" (without the surrounding blanks or delimiters), `a` is "a" (with them).

| Keys | Object |
|---|---|
| `iw` / `iW` | A word / WORD, or the run of blanks if the cursor is on blanks. |
| `aw` / `aW` | A word with the blanks after it, or the blanks before it if there are none after (end of line). On blanks: the blanks and the next word. |
| `is` / `as` | *(docs)* A sentence, without / with the blanks after it. |
| `ip` / `ap` | A paragraph (linewise). `ap` adds the empty lines after it, or before it if none follow. On an empty line, `ip` is the run of empty lines and `ap` that run plus the next paragraph. |
| `i(` `i)` `ib` / `a(` `a)` `ab` | Inside / including a `( )` pair. |
| `i[` `i]` / `a[` `a]` | Same for `[ ]`. |
| `i{` `i}` `iB` / `a{` `a}` `aB` | Same for `{ }`. |
| `i<` `i>` / `a<` `a>` | Same for `< >`. |
| `i"` `i'` `` i` `` / `a"` `a'` `` a` `` | Inside / including a quoted string on this line. `a` takes the blanks after the closing quote, or before the opening one if none. `\"` is skipped. |

- A **count** on `iw` / `aw` counts words **and** blanks between them for `iw` (`3iw` is word, blanks, word), and words with their blanks for `aw` (`2aw`). A count on a bracket object selects the `count`-th enclosing pair.
- **Brackets**: the cursor on either bracket of a pair counts as inside that pair. **If the cursor is not inside any pair, the object is the next pair after the cursor, even on a later line** (nvim 0.12, new in recent Vim; vi fails). If there is none, the command fails.
- **`i{` is linewise when the braces are alone on their lines**: it keeps the brace lines.
- **Quotes**: if the cursor is inside a quoted string, that string. If the cursor is before one, the next one on the line. If the cursor is between two strings, the text between the two quotes next to the cursor is taken as the string (a Vim quirk, kept). After the last quote of the line, or when the line has none, the command fails. It never looks at other lines.

| Before | Keys | After | reg |
|---|---|---|---|
| `a «b»b c` | `diw` | `a « »c` | `bb`(c) |
| `a b«b» c` | `daw` | `a «c»` | `bb `(c) |
| `a «b»b` | `daw` | `«a»` | ` bb`(c) |
| `a «b»b⏎c` | `daw` | `«a»⏎c` | ` bb`(c) |
| `aa« » bb` | `diw` | `aa«b»b` | `␠␠`(c) |
| `aa« » bb` | `daw` | `a«a»` | `␠␠bb`(c) |
| `«a»a bb cc` | `d2aw` | `«c»c` | `aa bb `(c) |
| `«a»a bb cc` | `d3iw` | `« »cc` | `aa bb`(c) |
| `«a».b` | `diw` | `«.»b` | `a`(c) |
| `«a».b` | `diW` | `«»` | `a.b`(c) |
| `a (b «c» d) e` | `di(` | `a («)» e` | `b c d`(c) |
| `a (b «c» d) e` | `da(` | `a « »e` | `(b c d)`(c) |
| `a (b (c«)» d) e` | `di(` | `a (b («)» d) e` | `c`(c) |
| `(a (b «c» d) e)` | `d2i(` | `(«)»` | `a (b c d) e`(c) |
| `«a» (b)` | `di(` | `a («)»` | `b`(c) (not inside: the next pair) |
| `«a»⏎(b)` | `di(` | `a⏎(«)»` | `b`(c) (a later line too) |
| `(a) «b» (c)` | `di(` | `(a) b («)»` | `c`(c) |
| `f(«a»)` | `ci(X<Esc>` | `f(«X»)` | `a`(c) |
| `{⏎  «a»⏎}` | `di{` | `{⏎«}»` | `  a⏎`(L) |
| `{⏎  «a»⏎}` | `da{` | `«»` | `{⏎  a⏎}⏎`(L) (R4) |
| `{⏎  «a»⏎}` | `ci{X<Esc>` | `{⏎  «X»⏎}` | `  a⏎`(L) |
| `x «"»a b" y` | `di"` | `x "«"» y` | `a b`(c) |
| `x "a «b»" y` | `da"` | `x «y»` | `"a b" `(c) |
| `x «y» "a b"` | `di"` | `x y "«"»` | `a b`(c) (the next string) |
| `x "a \"«b»" y` | `di"` | `x "«"» y` | `a \"b`(c) |
| `a "b" «c» "d"` | `di"` | `a "b"«"»d"` | ` c `(c) (the quirk) |
| `a "b" «c»` | `di"` | *fails* | |
| `«a»⏎"b"` | `di"` | *fails* | |
| `«a» b⏎c⏎⏎d` | `dip` | `«»⏎d` | `a b⏎c⏎`(L) |
| `«a» b⏎c⏎⏎d` | `dap` | `«d»` | `a b⏎c⏎⏎`(L) |
| `a⏎⏎«b»⏎c` | `dap` | `«a»` | `⏎b⏎c⏎`(L) (the empty line before it) |
| `a⏎«»⏎b` | `dip` | `a⏎«b»` | `⏎`(L) |
| `a⏎«»⏎b` | `dap` | `«a»` | `⏎b⏎`(L) |

### Normal-mode edit commands

#### Entering Insert mode

| Key | Action |
|---|---|
| `i` / `a` | Insert before / after the cursor character. |
| `I` / `A` | Insert at the first non-blank / at the end of the line. |
| `gI` | Insert at column 0. |
| `o` / `O` | Open a line below / above and insert on it. The new line copies the indent of the current one. |
| `gi` | Insert where Insert mode was last stopped. |

- A count repeats the typed text when `Esc` is pressed: `3ix<Esc>` types `xxx`. For `o` / `O` it repeats the whole new line.
- **`Esc` moves the cursor one character left** (not from column 0), even if nothing was typed.
- A line that holds only autoindent is emptied when you leave Insert mode.

| Before | Keys | After |
|---|---|---|
| `a«b»c` | `ix<Esc>` | `a«x»bc` |
| `a«b»c` | `ax<Esc>` | `ab«x»c` |
| `a«b»c` | `3ax<Esc>` | `abxx«x»c` |
| `ab«c»` | `ax<Esc>` | `abc«x»` |
| `a«b»c` | `i<Esc>` | `«a»bc` |
| `  a«b»` | `Ix<Esc>` | `  «x»ab` |
| `  a«b»` | `gIx<Esc>` | `«x»  ab` |
| `  a«b»` | `Ax<Esc>` | `  ab«x»` |
| `  a«b»` | `ox<Esc>` | `  ab⏎  «x»` |
| `  a«b»` | `Ox<Esc>` | `  «x»⏎  ab` |
| `  a«b»` | `o<Esc>` | `  ab⏎«»` (the indent is removed) |
| `a«b»` | `3ox<Esc>` | `ab⏎x⏎x⏎«x»` |
| `abc«d»` | `i<CR><Esc>` | `abc⏎«d»` |
| `a«b»c` | `ix<Esc>ay<Esc>gix<Esc>` | `axy«x»bc` |
| `a«b»c` | `ixy<Esc>.` | `axx«y»ybc` |

#### Changing text

| Key | Action |
|---|---|
| `r{c}` | Replace `count` characters with `{c}`. Fails if fewer than `count` remain on the line. `r Enter` replaces them with **one** line break and leaves the cursor on the first character of the new line. |
| `R` | Enter Replace mode. |
| `~` | Toggle the case of `count` characters and move right (clamped). Takes no motion. |
| `J` | Join `count` lines (at least 2). |
| `gJ` | Join without changing blanks. |
| `Ctrl+A` / `Ctrl+X` | Add / subtract `count` to the number at or after the cursor on this line. |
| `&` | Repeat the last `:s` on this line **with the same flags** (nvim). `g&` repeats it on all lines. |
| `.` | Repeat the last change. See [Undo, redo and repeat](#undo-redo-and-repeat). |

**`J` joins lines `i` and `i+1` like this**: remove the line break and the leading blanks of the next line, then insert one space, except when the current line ends with a blank, the next line (after its blanks) is empty, or it starts with `)`. One space only after `.`, `!`, `?` (nvim and Vim default, not two). The cursor goes on the inserted space, or if none was inserted, on the first character of the joined text (clamped). A count of 3 or more clamps, a count of 2 or less on the last line fails.

**`Ctrl+A` / `Ctrl+X`**: the number is the first number at or after the cursor on the line (*docs*: a cursor inside a number uses that whole number). Forms: decimal, `0x` hex, `0b` binary. Leading zeros keep the width, and `007` is **decimal**: `008`. Hex keeps the case and width. A `-` right before the digits is the sign, even right after a letter (`a-1` + 1 is `a0`). `Ctrl+X` can go negative. The cursor ends on the last digit. No number on the line fails.

| Before | Keys | After |
|---|---|---|
| `«a»bc` | `rx` | `«x»bc` |
| `«a»bc` | `2rx` | `x«x»c` |
| `a«b»c` | `3rx` | *fails* |
| `«a»bc` | `r<CR>` | `⏎«b»c` |
| `a«b»c` | `2r<CR>` | `a⏎«»` |
| `«a»⏎b` | `J` | `a« »b` |
| `«a»⏎  b` | `J` | `a« »b` |
| `«a» ⏎b` | `J` | `a «b»` |
| `«a»⏎)b` | `J` | `a«)»b` |
| `«a»⏎⏎b` | `J` | `«a»⏎b` |
| `«»⏎b` | `J` | `«b»` |
| `«a».⏎b` | `J` | `a.« »b` |
| `«a»⏎b⏎c` | `3J` | `a b« »c` |
| `«a»⏎b⏎c` | `9J` | `a b« »c` |
| `a⏎«b»` | `J` | *fails* |
| `«a»⏎  b` | `gJ` | `a« » b` |
| `x «9»` | `Ctrl+A` | `x 1«0»` |
| `«x» 5 6` | `Ctrl+A` | `x «6» 6` |
| `«x» 007` | `Ctrl+A` | `x 00«8»` |
| `«x» 0x0f` | `Ctrl+A` | `x 0x1«0»` |
| `«x» 0b101` | `Ctrl+A` | `x 0b11«0»` |
| `«x» -1` | `Ctrl+A` | `x «0»` |
| `«x» 5` | `9Ctrl+X` | `x -«4»` |
| `«x»` | `Ctrl+A` | *fails* |

#### Put

| Key | Action |
|---|---|
| `p` / `P` | Put the register `count` times after / before the cursor (characterwise), or below / above the line (linewise). Fails if the register is empty. |
| `gp` / `gP` | Like `p` / `P`, but the cursor ends just after the pasted text. |

**Cursor after `p` / `P`**: characterwise on one line: the last pasted character. Characterwise on several lines: the start of the pasted text. Linewise: the first non-blank of the first pasted line. **After `gp` / `gP`**: just after the text, clamped to the last character; linewise, on the line after the pasted lines, or on the last pasted line if that is the end of the buffer.

| Before | Keys | After |
|---|---|---|
| `«a»b` | `yl$p` | `ab«a»` |
| `«a»b` | `ylP` | `«a»ab` |
| `«a»b` | `yl3p` | `aaa«a»b` |
| `«a»b` | `ylgp` | `aa«b»` |
| `«a»b` | `xp` | `b«a»` |
| `«a»⏎b` | `yyjp` | `a⏎b⏎«a»` |
| `«a»⏎b` | `yyjP` | `a⏎«a»⏎b` |
| `«a»⏎b` | `yy2p` | `a⏎«a»⏎a⏎b` |
| `«a»⏎b` | `yj$p` | `a⏎«a»⏎b⏎b` |
| `a⏎«b»` | `yykgp` | `a⏎b⏎«b»` |
| `a⏎«b»⏎c` | `ddp` | `a⏎c⏎«b»` |
| `a«b»⏎c` | `vjy$p` | `ab«b»⏎c⏎c` |
| `a«b»⏎c` | `vjyP` | `a«b»⏎cb⏎c` |
| `«a»⏎  b` | `jyyP` | `a⏎  «b»⏎  b` (first non-blank) |
| `«a»` | `p` | *fails* (empty register) |

### Insert mode

| Key | Action |
|---|---|
| `Esc`, `Ctrl+[` | Leave Insert mode. The cursor moves one character left unless it is in column 0. |
| `Backspace`, `Ctrl+H` | Delete the character before the cursor. It crosses line breaks and the start of the insert, and in leading blanks it goes back to the previous shiftwidth stop (nvim `backspace` and `smarttab` defaults). |
| `Ctrl+W` | Delete the word before the cursor. |
| `Ctrl+U` | Delete the text typed on this line before the cursor. |
| `Enter` | Break the line. The new line copies the indent of the current one. There is no smart indent (a `{` does not add indent). |
| `Tab` | Insert a tab character. |
| `Ctrl+R {reg}` | Insert the contents of a register as if typed. |
| `Ctrl+O` | Run one Normal-mode command, then return to Insert mode. |
| Arrow keys | Move the cursor and start a new undo step. |

All text typed in one Insert session is one undo step and one dot-repeat unit.

| Before | Keys | After |
|---|---|---|
| `«a»b` | `A  x<C-w><Esc>` | `ab « »` |
| `«a»b` | `Afoo bar<C-w><Esc>` | `abfoo« »` |
| `«a»b` | `Afoo bar<C-u><Esc>` | `a«b»` |
| `«a»b` | `Afoo<CR><BS><BS>x<Esc>` | `abfo«x»` |
| `  «a»` | `o<BS>x<Esc>` | `  a⏎«x»` (one Backspace removes the whole 2-column indent) |
| `«a»b` | `Ax<C-o>0y<Esc>` | `«y»abx` |
| `a⏎«b»` | `I<BS>x<Esc>` | `a«x»b` (the line break was deleted) |
| `«a»` | `i<Tab>x<Esc>` | `→«x»a` |

### Replace mode

- `R` enters it. Each typed character overwrites the character under the cursor. At the end of the line it appends.
- `Enter` inserts a line break and does **not** overwrite a character.
- `Backspace` restores the original character and moves left. It does nothing before where Replace mode started.
- `Esc` leaves it. A count repeats the typed text.

| Before | Keys | After |
|---|---|---|
| `«a»bc` | `Rxy<Esc>` | `x«y»c` |
| `«a»bc` | `Rxyzw<Esc>` | `xyz«w»` |
| `«a»bc` | `Rxy<BS><Esc>` | `«x»bc` |
| `«a»bc` | `Rxy<CR>z<Esc>` | `xy⏎«z»` |
| `«a»bc` | `2Rxy<Esc>` | `xyx«y»` |

### Visual mode

`v` characterwise, `V` linewise, `Ctrl+V` blockwise (see Visual block mode).

- The selection is between the **anchor** (where it started) and the cursor, **inclusive**.
- Every motion and text object extends the selection. A text object on a selection of one character selects the object. On a larger selection it extends it (`viwiw` selects the word and the blanks after it).
- `$` in `v` mode puts the cursor on the end of the line **and the selection includes the line break**, except on the last line. So `v$d` joins the next line, and `v$y` yanks `bc⏎`.
- `o` swaps the cursor and the anchor. `v`, `V`, `Ctrl+V` switch the mode, or leave it if already in that mode. `Esc` leaves it. `gv` reselects the last selection.
- Operators act on the selection without a motion: `d x X D c s C S R y Y > < ~ u U g~ gu gU J gJ r{c} p P`.
  - `X`, `D`, `Y`, `S`, `C`, `R` work on whole lines (`vlD` deletes the whole line).
  - `r{c}` replaces every selected character.
  - `J` joins the selected lines (at least 2). A count is ignored.
  - `p` replaces the selection with the register, and the replaced text goes to the unnamed register. **`P` does the same but leaves the unnamed register unchanged.**
- The cursor after an operator is at the start of the selection.
- `:` opens the command line with the range `'<,'>`.
- A repeated (`.`) Visual operation acts on the same amount of text starting at the cursor (`V>.` shifts one line twice).

| Before | Keys | After | reg |
|---|---|---|---|
| `«a»bc` | `vld` | `«c»` | `ab`(c) |
| `a«b»c` | `vhd` | `«c»` | `ab`(c) |
| `«a»bc⏎def` | `vjd` | `«e»f` | `abc⏎d`(c) |
| `«a»bc⏎def` | `Vd` | `«d»ef` | `abc⏎`(L) |
| `a«b»c⏎d` | `v$d` | `a«d»` | `bc⏎`(c) |
| `a«b»c` | `v$y` | unchanged | `bc`(c) (last line: no line break) |
| `«a»bc⏎d` | `vj$d` | `«»` | `abc⏎d`(c) |
| `«a»b cd` | `vawd` | `«c»d` | `ab `(c) |
| `«a»bcd` | `vlly<Esc>gvd` | `«d»` | `abc`(c) |
| `«a»bc` | `vlrx` | `«x»xc` | |
| `«a»bc` | `vl~` | `«A»Bc` | |
| `«a»b⏎c⏎d` | `vjJ` | `ab« »c⏎d` | |
| `«a»b⏎c⏎d` | `V3J` | `ab« »c⏎d` | (the count is ignored) |
| `«a»⏎b⏎c` | `Vj3>` | `«→»→→a⏎→→→b⏎c` | |
| `«a»b⏎c` | `yyjVp` | `ab⏎«a»b` | `c⏎`(L) |
| `«a»b c` | `ywwvep` | `ab ab« »` | `c`(c) |
| `«a»b c` | `ywwvePP` | `ab abab« » ` | `ab `(c) (`P` kept `reg`) |

### Registers

| Register | Meaning |
|---|---|
| `""` | Unnamed. The default for yank, delete and put. It always points to the register written last. |
| `"0` | The last yank (without a named register). |
| `"1` .. `"9` | The last line-or-more deletes, newest in `"1`. A new write to `"1` shifts the others down. `"9` is lost. |
| `"-` | The last delete inside one line (small delete). |
| `"a` .. `"z` | Named. `"A` .. `"Z` append. |
| `"_` | Black hole. A write is discarded and does not touch any other register. Reading it gives nothing. |
| `"+` `"*` | System clipboard, through the `sys.h` seam. |
| `".` | The last inserted text. Read only. |
| `":` | The last command line, without the `:`. Read only. |
| `"/` | The last search pattern. Read only here. |
| `"%` | The current file name. Fails when the buffer has no name. Read only. |

**What a command writes** (verified):
- **Yank**: `""`, and `"0` unless a register was named.
- **Delete or change** (without a named register):
  - Inside one line: `""` and `"-`.
  - Linewise, or over more than one line: `""` and `"1` (shifting `"1`..`"9`).
  - With one of the motions `/`, `?`, `n`, `N`, `(`, `)`, `{`, `}` or a jump to a mark with the backtick: always `"1` (shifting), and also `"-` if the delete is inside one line.
  - **`%` is not in that list in nvim 0.12.6**: a single-line `d%` writes only `"-`. (The Vim documentation lists `%`. tix follows nvim.)
- **With a named register** (`"a`): `"a` and `""` are written. `"1` (with the shift) is **still** written for a linewise, multi-line or special-motion delete. `"-` is not written.
- **`"A`..`"Z` append.** If the old text or the appended text is linewise, the result is linewise and each part is on its own line. If both are characterwise the text is joined directly.
- `.` after `"1p` uses `"2`, and after that `"3`: `"1pu.u.` walks through the numbered registers.

| Before | Keys | Result |
|---|---|---|
| `«a»⏎b⏎c` | `dddd"1p` | `c⏎«b»` |
| `«a»⏎b⏎c⏎d` | `dddddd"2p` | `d⏎«b»` |
| `«a»⏎b⏎c⏎d` | `dddddd"1pu.u.` | `d⏎«a»` |
| `«a»b c` | `dlx"-p` | ` «b»c` (small deletes replace `"-`, they do not shift) |
| `«a»b c` | `dwx"1p` | `«»` (the small delete never reached `"1`) |
| `«a»b⏎c` | `yy"_ddP` | `«a»b⏎c` |
| `«a»b⏎c` | `yyjdd"0P` | `«a»b⏎ab` |
| `«a»b c` | `"_x"-p` | `«b» c` |
| `«a»b⏎c` | `"ayl"Ayy"ap` | `ab⏎«a»⏎ab⏎c` (linewise after the append) |
| `«a»b⏎c` | `"ayy"Ayl"ap` | `ab⏎«a»b⏎a⏎c` |
| `«a»b c` | `"ayw"Ayw"ap` | `aab ab« »b c` |
| `«a»b c` | `ix<Esc>".p` | `x«x»ab c` |
| `«a»b c` | `:s/a/z/<CR>":p` | `zs/a/z«/»b c` |

### Undo, redo and repeat

| Key | Action |
|---|---|
| `u` | Undo `count` changes. |
| `Ctrl+R` | Redo `count` changes. |
| `U` | Undo all the latest changes on the last changed line. It works on the last changed line even when the cursor is on another line, and moves the cursor there. `U` is itself undone by `u` or `U`. |
| `.` | Repeat the last change. |

- **A change** is one Normal-mode command that changed the buffer, or one whole Insert or Replace session. A cursor key typed in Insert mode ends the session's undo step (`ixy<Left>z<Esc>u` removes only the `z`).
- **The history is linear.** A new change after an undo drops the redo branch. (No undo tree.)
- **Cursor after undo**: on the first changed line. If that is the line the cursor was on before the change, its column is restored. Otherwise it is at the start of that line's change. After redo: at the start of the change.
- **`.`** repeats the last *change*: operators, `x`, `p`, `J`, `r`, `~`, `Ctrl+A`, and whole Insert sessions (`o`, `A`, `cw`...). It never repeats motions, yanks, `u`, `Ctrl+R`, scrolling or `:` commands.
  - A new count replaces the old one: `2x3.` deletes 3. A repeated Insert keeps its count: `3ox<Esc>.` adds 3 more lines. `ix<Esc>3.` types `x` three times.
  - It repeats the same register, and for `"1p` the next numbered one.
  - A repeated `cw` / `ciw` changes the same kind of range from the new cursor position.
  - A repeated Visual operation acts on the same amount of text from the cursor.

| Before | Keys | After |
|---|---|---|
| `«a»bc` | `xx3u` | `«a»bc` (3 is more than there is: clamps) |
| `«a»bc` | `xxuu<C-r>` | `«b»c` |
| `«a»bc` | `xxuu2<C-r>` | `«c»` |
| `«a»bc` | `ixy<Esc>lixy<Esc>u` | `xy«a»bc` |
| `a «b» c` | `wxu` | `a b «c»` |
| `abc⏎d«e»f⏎ghi` | `x$u` | `abc⏎d«e»f⏎ghi` |
| `abc⏎d«e»f⏎ghi` | `jjxggu` | `abc⏎def⏎g«h»i` (the column comes back) |
| `«a»bc` | `x.` | `«c»` |
| `«a»bcdef` | `2x3.` | `«f»` |
| `«a»b cd ef` | `dw.` | `«e»f` |
| `«a»b cd ef` | `cwX<Esc>w.` | `X «X» ef` |
| `«a»b⏎c⏎d` | `J.` | `ab c« »d` |
| `«a»b⏎c` | `Vj>.` | `«→»→ab⏎→→c` |

### Macros

| Key | Action |
|---|---|
| `q{a-z}` | Start recording into the register. `q` ends it. `q{A-Z}` appends to it. |
| `@{a-z}` | Run the register as typed keys, `count` times. |
| `@@` | Run the last executed register. |
| `@:` | Repeat the last command-line command. |

A failed command stops the macro, **including the remaining `count` repeats**. This is what makes `5@a` safe: it runs until something fails.

| Before | Keys | After |
|---|---|---|
| `«a»⏎b⏎c⏎d` | `qaxjq2@a` | `⏎⏎⏎«d»` |
| `«a»a⏎b⏎c` | `qaxjq5@a` | `a⏎⏎«»` (the last `j` failed, the rest was skipped) |
| `«a»b⏎c` | `qaA!<Esc>jq@a` | `ab!⏎c«!»` |

### Marks and the jumplist

- `m{a-z}` sets a mark at the cursor in this buffer. `` `{m} `` goes to its exact position and `'{m}` to the first non-blank of its line.
- **Marks follow the text.** Lines inserted or deleted above move them. Deleting the line a mark is on deletes the mark. Using an unset mark fails.
- Special marks: two backticks or `''` the position before the latest jump; `` `. `` the last change; `` `[ `` and `` `] `` the start and end of the last change or yank; `` `< `` and `` `> `` the start and end of the last Visual selection.
- **A jump** is any of: `G gg H M L % ( ) { } / ? n N * #`, `` ` `` and `'` to a mark, `:{N}`, and opening a file. A jump saves the position it left in the jumplist.
- `Ctrl+O` goes `count` entries back and `Ctrl+I` goes `count` forward. At the newest end, `Ctrl+O` first saves the current position so that `Ctrl+I` can come back. A count beyond the list fails and the cursor stays (exact policy).
- **Same-line entries merge.** An entry on the same line as a newer one is dropped, so a jump inside one line (`/b` on the same line) leaves nothing to go back to.
- `Ctrl+I` is a different key from `Tab`. In Normal mode `Tab` moves among docks and elements (see Normal mode). This split is deliberate. In a terminal both are the same byte, in a Win32 window they are not.

| Before | Keys | After |
|---|---|---|
| `«a»⏎b⏎c⏎d` | `3GG<C-o>` | `a⏎b⏎«c»⏎d` |
| `«a»⏎b⏎c⏎d` | `3GG<C-o><C-o>` | `«a»⏎b⏎c⏎d` |
| `«a»⏎b⏎c⏎d` | `3GG<C-o><C-o><C-i><C-i>` | `a⏎b⏎c⏎«d»` |
| `«a»⏎b⏎c⏎d` | `2G3G4G2<C-o>` | `a⏎«b»⏎c⏎d` |
| `«a»⏎b⏎c⏎d` | `2G3G4G9<C-o>` | `a⏎b⏎c⏎«d»` (fails, no move) |
| `«a»⏎b⏎c⏎d` | `jGkgg` then two backticks | `a⏎b⏎«c»⏎d` (back to the position before the last jump) |
| `«a»⏎b⏎c⏎d` | `jjmaggdd'a` | `b⏎«c»⏎d` (the mark followed its line) |
| `«a» b a⏎b` | `/b<CR><C-o>` | `a «b» a⏎b` (same line: nothing to go back to) |

### Search

- `/` and `?` open the command line. `Enter` runs the search and `Esc` cancels it. An empty pattern reuses the last one.
- The search wraps around the buffer end. No match is a failed command with a message (`E486`), and the cursor does not move.
- `n` and `N` follow the direction of the last `/` or `?`.
- `*` and `#` search for the whole keyword under the cursor, or the first keyword after it on the line. With no keyword they fail. `*` on `foo` skips `xfoo`.
- Every search updates `"/` and counts as a jump.
- As a motion after an operator it is exclusive and follows R3: `d/c<CR>` from `«a»bc⏎d` deletes `ab`.

| Before | Keys | After |
|---|---|---|
| `«a» b a b` | `/b<CR>nn` | `a «b» a b` (wraps) |
| `«a» b a b` | `?b<CR>` | `a b a «b»` |
| `«f»oo foo xfoo` | `*` | `foo «f»oo xfoo` |
| `foo xfoo «f»oo` | `*` | `«f»oo xfoo foo` (wraps) |
| `«a» b` | `/z<CR>` | *fails* |

### Command line

`:` opens the command line. `Enter` runs it, `Esc` cancels it, `Backspace` on an empty line cancels it, `Up` / `Down` walk the history.

| Command | Action |
|---|---|
| `:{N}` | Go to line `N` (see Up and down). |
| `:[range]d [count]` | Delete lines. Same register rules as `dd`. The cursor goes to the line that took the place. |
| `:[range]y [count]` | Yank lines. The cursor does not move. |
| `:[range]s/{pattern}/{replacement}/[flags]` | Substitute. Flags: `g` all matches on the line, `i` ignore case. The cursor goes to the last line where a substitution was made. No match fails. |
| `:w`, `:w {file}` | *(docs)* Write the buffer. Without a range, all lines. |
| `:q`, `:q!` | *(docs)* Close the buffer. `:q` fails if there are unsaved changes. `:q!` discards them. |
| `:wq`, `:x`, `ZZ` | *(docs)* Write and close. `:x` and `ZZ` write only when the buffer changed. |
| `ZQ` | *(docs)* Same as `:q!`. |
| `:e {file}`, `:e!` | *(docs)* Open a file (fails on unsaved changes) / reload and discard changes. |
| `:noh` | *(docs)* Clear the search highlight. |

- **Ranges**: `N`, `.` the cursor line, `$` the last line, `%` all lines, `N,M`, `'<,'>`, `'{m}`, and `+N` / `-N` offsets. No range means the current line. A range past the end of the buffer is clamped for `:{N}` and an error for the others. A backwards range (`:3,2d`) is an error and changes nothing (nvim asks for confirmation, tix does not ask).
- Errors are shown in the command pad. They are not fatal.

| Before | Keys | After | reg |
|---|---|---|---|
| `«a»⏎b⏎c` | `:2d<CR>` | `a⏎«c»` | `b⏎`(L) |
| `«a»⏎b⏎c` | `:2,3d<CR>` | `«a»` | `b⏎c⏎`(L) |
| `«a»⏎b⏎c` | `:$d<CR>` | `a⏎«b»` | `c⏎`(L) |
| `«a»⏎b⏎c` | `:.,+1d<CR>` | `«c»` | `a⏎b⏎`(L) |
| `a⏎«b»⏎c` | `:-1,.d<CR>` | `«c»` | `a⏎b⏎`(L) |
| `a⏎«b»⏎c` | `:d 2<CR>` | `«a»` | `b⏎c⏎`(L) |
| `«a»⏎b⏎c` | `:2y<CR>p` | `a⏎«b»⏎b⏎c` | |
| `«a»a⏎aa` | `:%s/a/b/<CR>` | `ba⏎«b»a` | |
| `«a»a⏎aa` | `:%s/a/b/g<CR>` | `bb⏎«b»b` | |
| `«a»a⏎aa` | `:s/A/b/i<CR>` | `«b»a⏎aa` | |
| `«a»a⏎aa` | `:9<CR>` | `aa⏎«a»a` | |
| `«a»a⏎aa` | `:s/z/y/<CR>` | *fails* | |
| `«a» a a⏎a a a` | `:s/a/b/g<CR>j&` | `b b b⏎«b» b b` (`&` keeps `g`) | |

### Where tix follows nvim or Vim over vi

| Command | vi | Vim | nvim 0.12 | tix |
|---|---|---|---|---|
| `Y` | `yy` | `yy` | `y$` | nvim |
| `&` | `:s`, no flags | same | `:&&`, keeps flags | nvim |
| `Ctrl+A` on `007` | octal | octal | decimal `008` | nvim |
| `Backspace` in Insert | stops at the insert start | crosses it in `defaults.vim` | crosses indent, line break and start | nvim |
| `o`, `O`, `Enter` indent | none | off by default | autoindent on | nvim |
| `J` after `.` | two spaces | one (`nojoinspaces`) | one | Vim and nvim |
| `;` after `t` | stuck | skips to the next match | skips | Vim and nvim |
| Undo | one level, `u` undoes itself | multi-level, `Ctrl+R` redoes | same | Vim and nvim |
| `P` in Visual | — | does not overwrite `""` in Vim 9 | does not overwrite | Vim 9 and nvim |
| `G`, `gg`, `:N`, `dd`, `>>` column | first non-blank | first non-blank (`startofline`) | keeps the column (`nostartofline`) | nvim |
| `di(` when not inside a pair | fails | fails | takes the next pair | nvim |
| Delete registers with a named register | — | `"1` still written for line deletes | same as Vim | Vim and nvim |
| `d%` | — | register `"1`, linewise promotion by the docs | `"-` only, no promotion | nvim (observed) |

### Open decisions

- **Regex flavour** for `/`, `?`, `*`, `#` and `:s`: literal text, a Vim-compatible subset, or something else. It decides the replacement escapes (`&`, `\1`) too.
- **Indent options**: shiftwidth 8 and tabs are the nvim defaults. Code editing may want 4 spaces. `smarttab` Backspace depends on it.
- **Exact character classes** for Unicode word motions: the list above names the classes that were verified. The full table (nvim's `utf_class`) is still to be chosen for scripts not listed.
- **Line terminators**: whether a CRLF file is edited as CRLF or converted, and how a missing final newline shows up.
- **`H`, `M`, `L`, `Ctrl+D/U/F/B`** and the sentence commands were not verified. Check them against nvim once the window height and wrapping rules exist.

### Out of scope

Scripting, mappings and abbreviations, `:normal`, `:source`, `:set` options, folds, vim windows and tab pages (tix has docks and tabs), digraphs, insert-mode completion (see Autocomplete), `=`, `gq` and other formatting operators, `!` filters, tag and quickfix commands, the undo tree (`g-`, `g+`), the changelist (`g;`, `g,`) and Ex mode (`Q`, `gQ`).

## Autocomplete

- Fuzzy matching for autocomplete