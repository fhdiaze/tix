# Review history

Maintained by the `code-reviewer` agent. Add entries below, newest last.

| category | first seen | last seen | reviews with it | status |
|---|---|---|---|---|
| missing-bounds-assert | 2026-10-11 | 2026-10-11 | 1 | active |
| unsigned-underflow | 2026-10-11 | 2026-10-11 | 1 | active |
| pointer-past-end | 2026-10-11 | 2026-10-11 | 1 | active |
| ignored-error-return | 2026-10-11 | 2026-10-11 | 1 | active |
| unit-suffix-naming | 2026-10-11 | 2026-10-11 | 1 | active |
| extracted-without-duplication | 2026-10-11 | 2026-10-11 | 1 | active |

## 2026-10-11, uncommitted
- scope: `mem_copy_rect` in src/sys_win.c:110-139 (selection only), plus its single caller at :184
- findings: `missing-bounds-assert` src/sys_win.c:117-121; `unsigned-underflow` src/sys_win.c:134-135; `pointer-past-end` src/sys_win.c:134-135; `ignored-error-return` src/sys_win.c:115,138 and :184; `unit-suffix-naming` src/sys_win.c:110-113; `extracted-without-duplication` src/sys_win.c:110
- improved: none (first review)
