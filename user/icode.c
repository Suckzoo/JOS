
#include <inc/lib.h>


#ifdef VMM_GUEST
#define MOTD "/motd_guest"
#else
#define MOTD "/motd"
#endif


void
umain(int argc, char **argv)
{
	int fd, n, r;
	char buf[512+1];
	int envNumber;

	binaryname = "icode";
	envNumber = sys_getenvid();
	cprintf("env #:%d\n", envNumber);
	cprintf("icode startup\n");

	cprintf("icode: open /motd\n");
	if ((fd = open(MOTD, O_RDONLY)) < 0)
		panic("icode: open /motd: %e", fd);

	cprintf("icode: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0) {
		cprintf("Reading MOTD\n");
		sys_cputs(buf, n);
	}
	cprintf("icode: close /motd\n");
	close(fd);

	if(envNumber % 2) {
		cprintf("icode: open /motd\n");
		if ((fd = open(MOTD, O_WRONLY)) < 0)
			panic("icode: open /motd: %e", fd);
		cprintf("icode: write /motd\n");
		memcpy(buf, "modified\n", 20);
		buf[9] = 0;
		write(fd, buf, 20);
		cprintf("icode: close /motd\n");
		close(fd);
	}
	cprintf("icode: open /motd\n");
	if ((fd = open(MOTD, O_RDONLY)) < 0)
		panic("icode: open /motd: %e", fd);

	cprintf("icode: read /motd\n");
	while ((n = read(fd, buf, sizeof buf-1)) > 0) {
		cprintf("Reading MOTD\n");
		sys_cputs(buf, n);
	}

	cprintf("icode: close /motd\n");
	close(fd);

	cprintf("icode: spawn /sbin/init\n");
	if ((r = spawnl("/sbin/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
		panic("icode: spawn /sbin/init: %e", r);
	cprintf("icode: exiting\n");
}
