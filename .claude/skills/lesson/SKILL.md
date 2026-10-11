---
name: lesson
description: Start or continue a learning session with the professor agent. Use when the user types /lesson or asks to study, learn, or be taught a topic for tix.
---

Launch the `professor` agent with the Agent tool (`subagent_type: "professor"`).

- If the user gave a topic in the arguments, pass it along. Otherwise tell the professor to pick the next lesson from
  the review history, `doc/todo.md` and the curriculum, and explain the choice.
- The professor teaches one question at a time. If it comes back with a question for the user, relay it, and
  continue the same agent with SendMessage when they answer.
- Don't teach or answer the lesson yourself. Don't edit the Learning log in `CLAUDE.md` without the user approving the
  exact entries.
