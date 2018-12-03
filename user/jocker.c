#include <inc/lib.h>
#include <inc/elf.h>
#include <inc/stdio.h>

void
usage() {
    cprintf("Usage: jocker [root] [target_binary] [args...]\n");
}

void
umain(int argc, char **argv) {
    int i;
    if (argc < 3) {
        usage();
        exit();
    }

    const char* root = argv[1];
    char** target_binary = &argv[2];
	
	cprintf("i am jocker hatchery with envid %08x\n", thisenv->env_id);
    int r;
	if ((r = spawnc((const char*)(*target_binary), root, (const char **)target_binary)) < 0)
        panic("spawn(%s) failed: %e", target_binary, r);
    wait(r);
}


