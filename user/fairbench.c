#include <inc/lib.h>
#include <inc/elf.h>
#include <inc/stdio.h>

void
usage() {
    cprintf("Usage: jocker [-mbs] [num_child <= 10]\n");
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

const char* root = "/jocker";
char child_argv[2][20] = {"/bin/heavyreader\0", "horse1\0"};
char heavy_argv[2][20] = {"/bin/heavyreader\0", "heavy\0"};
char light_argv[2][20] = {"/bin/lightreader\0", "light\0"};

void
test_multiple_container (int num_child) {
    int i;
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

void
test_byte_budget () {
    int r1, r2;
    //heavyreader spawn
    const char* h_argv[] = {heavy_argv[0], heavy_argv[1], NULL};
    if ((r1 = spawnc((const char*)(heavy_argv[0]), root, (const char**)h_argv)) < 0)
        panic("spawn failed: %e", r1);

    //lightreader spawn
    const char* l_argv[] = {light_argv[0], light_argv[1], NULL};
    if ((r2 = spawnc((const char*)(light_argv[0]), root, (const char**)l_argv)) < 0)
        panic("spawn failed: %e", r2);
        
    wait(r1);
    wait(r2);
}

void
test_container_sharing () {
    // create two container
	int r, i;
    cid_t c1, c2;
	if ((r = sys_create_container(root)) < 0) {
        panic("failed creating a new container: %e", r);
	}
    c1 = (cid_t)r;
	if ((r = sys_create_container(root)) < 0) {
        panic("failed creating a new container: %e", r);
	}
    c2 = (cid_t)r;

    const int c1_child = 2;
    const int c2_child = 3;

    int child[c1_child + c2_child];


    char c_label[20] = "container-01\0";
    const char* c_argv[] = {child_argv[0], c_label, NULL};
    for (i = 0; i < c1_child; i++) {
        child_argv[1][5] = i + '0';
        if ((child[i] = spawncid((const char*)(child_argv[0]), c1, (const char**)c_argv)) < 0)
            panic("spawn failed: %e", child[i]);
    }

    c_label[11] = '2';
    for (i = 0; i < c2_child; i++) {
        child_argv[1][5] = i + '0';
        if ((child[c1_child + i] = spawncid((const char*)(child_argv[0]), c2, (const char**)c_argv)) < 0)
            panic("spawn failed: %e", child[c1_child + i]);
    }

    for(i=0;i<c1_child + c2_child;i++) {
        wait(child[i]);
    }
}

void
umain(int argc, char **argv) {
    if (argc < 2) {
        usage();
        exit();
    }
    
	cprintf("fairness evaluator with envid %08x\n", thisenv->env_id);

    if (!strcmp(argv[1], "-m") && argc > 2) {
        int num_child = atoi(argv[2]);
        test_multiple_container(num_child);
    } else if (!strcmp(argv[1], "-b")) {
        test_byte_budget();
    } else if (!strcmp(argv[1], "-s")) {
        test_container_sharing();
    } else {
        usage();
    }

}


