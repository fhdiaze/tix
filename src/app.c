#include "app.h"

// Parsing

// ui: pane, gutter, tab, panel,
// entities: buffer, document, span/range, anchor, mark, selection
// structure: tree, lines
// primitives: chars, code_point, grapheme cluster, glyph, rune,

static void buffer_split(Buffer *buffer)
{
}

void app_init(Storage *storage)
{
	buffer_split(nullptr);
}
