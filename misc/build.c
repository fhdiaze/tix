#include <stdlib.h>

#define BUILD_MODE "debug"
#define ARCHITECTURE "x64"
#define LIVE_BUILD "0"

#define APP_FILE_NAME "app"
#define SYS_FILE_NAME "sys_win"

#define OUT_DIR "./bin"
#define DATA_DIR "./data"

#define APP_FILE_PATH "./src/" APP_FILE_NAME ".c"
#define SYS_FILE_PATH "./src/" SYS_FILE_NAME ".c"

#define OUT_APP_FILE_NAME "tix_app"
#define OUT_SYS_FILE_NAME "tix_win"
#define OUT_SYS_FILE_PATH "./bin/tix_win.exe"
#define OUT_APP_FILE_PATH "./bin/tix_app.dll"

#define FLAGS_FILE "./compile_flags.txt"
#define DEBUG_FLAGS "-g -gcodeview -O0 -DDEBUG -Wl,/DEBUG:FULL -fms-runtime-lib=static_dbg"
// #define DEBUG_FLAGS "-g -gcodeview -O0 -DDEBUG -Wl,/DEBUG:FULL -fms-runtime-lib=static_dbg -fsanitize=address -fno-omit-frame-pointer"
#define RELEASE_FLAGS "-O3 -DNDEBUG -flto -Wl,/opt:ref -Wl,/opt:icf -fms-runtime-lib=static"
#define FLAGS ""
#define APP_FLAGS "-shared -Wl,/MAP:./bin/tix_app.map,/MAPINFO:EXPORTS -Wl,/PDB:./bin/tix_app.pdb"
#define SYS_FLAGS \
	"-mavx2 -luser32 -lgdi32 -lwinmm -ldwmapi -Wl,/subsystem:windows -Wl,/MAP:./bin/tix_win.map,/MAPINFO:EXPORTS"

int main(void)
{
	return EXIT_SUCCESS;
}
