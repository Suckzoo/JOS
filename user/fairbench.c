#include <inc/lib.h>
#include <inc/elf.h>
#include <inc/stdio.h>

void
usage() {
    cprintf("Usage: jocker [num_child <= 10]\n");
}

int
atoi (char *num) {
    int len = strlen(num);
    int i;
    int res = 0;
    for(i=0;i<len;i++) {
        res *= 10;
        res += (num[i] - '0');
    }
    return res;
}

char child_argv[2][20] = {"/bin/heavyreader\0", "horse1\0"};

void
umain(int argc, char **argv) {
    int i;
    if (argc < 2) {
        usage();
        exit();
    }
    int num_child = atoi(argv[1]);

    const char* root = "/jocker";
	
	cprintf("fairness evaluator with envid %08x\n", thisenv->env_id);
    int r[10];
    const char* c_argv[] = {child_argv[0], child_argv[1], NULL};
    for (i = 0; i < num_child; i++) {
        child_argv[1][5] = i + '0';
        if ((r[i] = spawnc((const char*)(child_argv[0]), root, (const char**)c_argv)) < 0)
            panic("spawn failed: %e", r);
    }
    for (i = 0; i < num_child; i++) {
        wait(r[i]);
    }
}


