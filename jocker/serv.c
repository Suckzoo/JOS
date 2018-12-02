
/*
 * Container server main loop -
 * hijacks IPC requests from other environments.
 * This server performs two actions:
 * 1. chrooting the directories by hijacking FS
 * IPC requests
 * 2. Load balancing among containers by round-robin
 * mechanism
 */

#include <inc/x86.h>
#include <inc/string.h>

#include "jocker.h"


#define debug 0

#define IPC_ADDR 0xffff000

union Fsipc *fsreq = (union Fsipc*)IPC_ADDR;;

typedef int (*fshandler)(envid_t envid, union Fsipc *req);

void
serve(void)
{
	uint32_t req, whom;
	int perm, r;
	void *pg;
	while (1) {
		sys_page_alloc(thisenv->env_id, (void*)fsreq, PTE_U | PTE_W | PTE_P);
		perm = 0;
		req = ipc_recv((int32_t *) &whom, fsreq, &perm);

		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}

		pg = NULL;
		if (req == FSREQ_OPEN) {
			cprintf("open");
		} else if (req < __BOUND_FSREQ) {
			cprintf("handler logic");
			// You don't need to handle the logic!
			// Just redirect it and hold the redirect table!
			// r = handlers[req](whom, fsreq);
		} else {
			cprintf("Invalid request code %d from %08x\n", req, whom);
			r = -E_INVAL;
		}
		// Depends on your redirect table.
		// ipc_send(whom, r, pg, perm);
		if(debug)
			cprintf("JOCKER: Sent response %d to %x\n", r, whom);
		sys_page_unmap(thisenv->env_id, fsreq);
	}
}

void
draw_fancy_logo ()
{
	cprintf("                                 ,-\n");
	cprintf("                               ,'::|\n");
	cprintf("                              /::::|\n");
	cprintf("                            ,'::::o\\                                      _..\n");
	cprintf("         ____........-------,..::?88b                                  ,-' /\n");
	cprintf(" _.--"""". . . .      .   .  .  .  ""`-._                           ,-' .;'\n");
	cprintf("<. - :::::o......  ...   . . .. . .  .  .""--._                  ,-'. .;'\n");
	cprintf(" `-._  ` `\":`:`:`::||||:::::::::::::::::.:. .  ""--._ ,'|     ,-'.  .;'\n");
	cprintf("     \"\"\"_=--       //'doo.. ````:`:`::::::::::.:.:.:. .`-`._-'.   .;'\n");
	cprintf("         \"\"--.__     P(       \\               ` ``:`:``:::: .   .;'\n");
	cprintf("                \"\\\"\"--.:-.     `.                             .:/\n");
	cprintf("                  \\. /    `-._   `.\"\"-----.,-..::(--\"\".\\\"\"`.  `:\\\n");
	cprintf("                   `P         `-._ \\          `-:\\          `. `:\\\n");
	cprintf("                                   \"\"            \"            `-._)  -Seal\n");
}

void
umain(int argc, char **argv)
{
	// static_assert(sizeof(struct File) == 256);
	binaryname = "jocker";
	cprintf("Jocker shark is swimming\n");

	draw_fancy_logo();

	// ipc_queue_init();
	// redirect_table_init();

	serve();
}

