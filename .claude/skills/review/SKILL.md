---
name: review
description: Run the code-reviewer agent on the tix author's recent work (uncommitted changes plus the last commit by default). Use when the user types /review or asks for a code review with repeated-mistake tracking.
---

Launch the `code-reviewer` agent with the Agent tool (`subagent_type: "code-reviewer"`).

- Pass along any scope the user gave in the arguments (files, a commit range). Otherwise leave it to the default.
- Do not review the code yourself and don't fix anything. Relay the agent's report to the user in full, keeping its
  three sections (Code links, Repeated mistakes, Improved).
- Mention that `doc/review-history.md` was updated.
