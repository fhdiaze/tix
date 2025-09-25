typedef enum { tm_normal = 0, tm_view = 1 } TixMode;

typedef struct {
	TixMode tix_mode;
	char *tix_Path;
	size_t tix_line;
	size_t tix_column;
} TixState;

extern void editor_init();
