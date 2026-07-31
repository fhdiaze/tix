#include "app.h"

// Parsing

// ui: pane, gutter, tab, panel,
// entities: buffer, document, span/range, anchor, mark, selection
// structure: tree, lines
// primitives: chars, code_point, grapheme cluster, glyph, rune,

typedef struct Buffer {
	char tx_name[256];
	char tx_parts[256];
} Buffer;

static void buffer_split(Buffer *buffer)
{
}

void tix_init(Tix *tix)
{
	buffer_split(nullptr);
}
