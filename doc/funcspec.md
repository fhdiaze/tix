# Functional Specification

## Overview

tix is a text editor focused on editing code in any programming language. It has the next values in order of importance:

- Correctness
- Speed
- Low Memory footprint
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

### Visual mode

### Insert mode

### Visual block mode

### Multi selection mode

## Autocomplete

- Fuzzy matching for autocomplete