# tix

tix is a text editor written in C focused on usability, performance and correctness.

## Functionality

- Its UI is text based
- It is formal specified using TLA+
- It is command based
- It does not use scripting languages

## Tech

- Modularized
- Separate state from actions
  - state: line, filename, column, modified, active_element, capabilities
  - actions: save, move, delete, etc

## Inspired by

- https://kakoune.org/why-kakoune/why-kakoune.html
- https://github.com/antirez/kilo
