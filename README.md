# tix

tix is a text editor written in c

## Fun

- it is text based
- it is formal specified
- it is command based
- it does not use scripting languages

## Definitions

- Structure: tree, lines, symbols, files, group, buffers
- primitives: chars

## Rules

- Plugin architecture

- Separate state from actions
    - state: line, filename, column, modified, active_element, capabilities
    - actions: save, move, delete, etc
