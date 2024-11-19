// nob.c
#define NOB_IMPLEMENTATION
#include "nob.h"

// Folders must end with forward slash /
#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0}; 
    const char *program_name = nob_shift(argv, argc);

    if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;
    nob_cmd_append(&cmd, "cc", "-ggdb", "-Wall", "-Wextra", "src/test.c", "-o", "build/test");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "clang", "-Wall", "-Wextra", "src/screvi.c", "-o", "build/screvi");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    return 0;
}