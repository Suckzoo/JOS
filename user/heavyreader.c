
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int rfd, wfd;
	char buf[512];
	int n, r;

    int trial, num_file, repeat = 100;
    if (argc < 2) {
        cprintf("Usage: heavyreader [runner_name]\n");
        exit();
    }
    const char *runner = argv[1];
    for(trial = 1; trial <= 10; trial++) {
        unsigned int start = sys_time_msec();
        for (num_file = 1; num_file <= repeat; num_file++) {
            if ((rfd = open("/shark", O_RDONLY)) < 0)
                panic("open /shark: %e", rfd);
            if ((wfd = open("/shark.out", O_RDWR | O_CREAT)) < 0)
                panic("open /shark.out: %e", wfd);
            while ((n = read(rfd, buf, sizeof buf-1)) > 0) {
                if ((r = write(wfd, buf, n)) != n)
                    panic("write /motd: %e", r);
            }
            close(rfd);
            close(wfd);
        }
        unsigned int end = sys_time_msec();
        cprintf("[%s (%d/10)] %d msec elapsed for %d I/O ops.\n", runner, trial, end - start, repeat);
    }
}

