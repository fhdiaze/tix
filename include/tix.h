#define TIX_PATH_MAX 4096

typedef enum { tm_normal = 0, tm_view = 1 } TixMode;

typedef struct {
        TixMode tix_mode;
        char tix_path[TIX_PATH_MAX];
        size_t tix_line;
        size_t tix_column;
} TixState;

extern void tix_editor_init(void);

// Parsing

// structure: tree, lines
// primitives: chars
