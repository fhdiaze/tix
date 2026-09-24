# Code Style

## Developer Experience

### Naming Things

* Add units or qualifiers to variable names, and put the units or qualifiers last, sorted by descending significance, so that the variable starts with the most significant word, and ends with the least significant word. For example, latency_ms_max rather than max_latency_ms. This will then line up nicely when latency_ms_min is added, as well as group all variables that relate to latency.

* The usual suspects for off-by-one errors are casual interactions between an index, a count or a size. These are all primitive integer types, but should be seen as distinct types, with clear rules to cast between them. To go from an index to a count you need to add one, since indexes are 0-based but counts are 1-based. To go from a count to a size you need to multiply by the unit. Again, this is why including units and qualifiers in variable names is important.
    * We consistently use count whenever we talk about the number of items, and index to point to a particular item. The positive invariant is index < count.
    * The “count of bytes” is always called a size.
    * And offset is the bytewise counterpart of index.