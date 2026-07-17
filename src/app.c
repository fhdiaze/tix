#include "app.h"

// =============================================================================
// Blob of text
// =============================================================================

typedef struct CellPosition {
	uint32_t x;
	uint32_t y;
} CellPosition;

typedef struct Blob {
	char tx_name[256];
	char tx_parts[256];
} Blob;

static void blob_split(Blob *blob)
{
}

void tix_init(Tix *tix)
{
	blob_split(nullptr);
}
