# Code Style

## Developer Experience

### Naming Things

* Add units or qualifiers to variable names, and put the units or qualifiers last, sorted by descending significance, so that the variable starts with the most significant word, and ends with the least significant word. For example, latency_ms_max rather than max_latency_ms. This will then line up nicely when latency_ms_min is added, as well as group all variables that relate to latency.

* The usual suspects for off-by-one errors are casual interactions between an index, a count or a size. These are all primitive integer types, but should be seen as distinct types, with clear rules to cast between them. To go from an index to a count you need to add one, since indexes are 0-based but counts are 1-based. To go from a count to a size you need to multiply by the unit. Again, this is why including units and qualifiers in variable names is important.
    * We consistently use count whenever we talk about the number of items, and index to point to a particular item. The positive invariant is index < count.
    * The “count of bytes” is always called a size.
    * And offset is the bytewise counterpart of index.

## Error Handling

* Treat errors as data, not as exceptional control flow. Each `if (!ok)` or `assert` adds another codepath, and the number of paths multiplies as checks stack up. Where you can, write code so the error case goes down the same path as the success case.

* Zero is initialisation. Design every type so that its all-zero value is valid and means "empty" or "do nothing". A zeroed `{ .buf = 0, .size = 0 }` is an empty buffer, and a loop over `count == 0` simply doesn't run. Then a failed read or allocation can return a zeroed struct, and callers don't need a special case.

* Prefer AND over OR in results. Return the data and the error information together in one struct, not a tagged "either success or error". The caller can write straight-line code and only look at the errors when it needs to, for example `ReadFileResult { buf, size, errors }`. Name these `<Action>Result`.

* Use nil structs instead of NULL for read-only lookups. For linked or tree-shaped data, a failed lookup returns a pointer to a static, read-only `nil_<type>` whose own pointers point back to itself. Chained lookups then never crash and never need a NULL check. Check for the nil value only at the point where it matters.
    * Only use this for reads: code must never write to a nil struct.

* Fail early, in shallow stack frames. Get and check the resources you need (memory, files, window, fonts) as close to the entry point as possible, before any real work starts. Once a guarantee is made, for example "the arena was reserved", code further down relies on it and doesn't check again.

* Push error details into a side channel, not return codes. Don't use `errno`-style single integers. Add messages to a per-thread (or per-app) message list that records the source location, severity and text. Code keeps running, all errors are kept rather than only the last one, and you can set a breakpoint on the logging function to catch any error when it happens.

* Assertions are for programmer errors, not runtime conditions. Use `assert` to check invariants that should never break, like `index < count`. Things that can really happen at runtime, like a missing file or a failed allocation, should be handled as data using the rules above.