# Functional Specification

## Overview

tix is an text editor focused on editing code in any programming language. has the next values:

- Correctness is the most important feature
- Speed is the second most important feature
- Low Memory footprint but speed first
- All the UI should be text like
- Based on vim modes
- The screen space should be optimized.
- Keyboard navigation first, mouse navigation second.
  - vim everywhere, vim first
  - All the UI element are vim-like.
- All the elements should keep the same font size
- Fuzzy matching for autocomplete

## Scenarios

### Scenario 1: Carlos
Carlos has not experience with vim or mode-based editors.

### Scenario 2: Andrew
Andrew has extended experience with neovim.

## Non Goals

This editor will not support
- Scripting language for extensions

## Docks

- The editor has the next visual blocks
  - Three docks:
    - Main dock: the files are open here.
    - Side dock: for the file tree and other tools
    - Bottom dock: for the terminal.
  - One title bar
    - menu
    - tabs for the active dock
    - root folder name
    - status icons
      - mode
      -
- All the docks are based on tabs
- The tabs of the active dock are showed on the title bar.
- docks could be hide
- Just the main dock could be split.
- There is a picker for the tabs in a dock
- Tabs can be pinned to a dock

## Actions

- Command palette: a picker
- Command prompt/pad:

## Pickers

- Just on picker: command prefix # for file, @ for fzf, etc

## Navigation

## Modes

### normal mode

- Tab should move the cursor like in a grid: increasing column first

### visual mode

### insert mode

### visual block mode

### multi selection mode
