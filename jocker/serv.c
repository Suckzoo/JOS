
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
	uint32_t r, value;
	envid_t from_pid, to_pid;
	unsigned perm;
	void *srcva;

	while (1) {
		perm = 0;
		// try to deqeue, if empty, sleep
		if(sys_cont_isqueue_sleep()) {
			// dequeue ipc_entry
			r = sys_cont_dequeue_ipc((int32_t *) &from_pid, &to_pid, &value, &srcva, &perm);
			// send ipc as if it is sent from from_pid
			if(r==0) {
				cont_ipc_send(from_pid, to_pid, value, srcva, perm);
			}
		}
	}
}

void
draw_fancy_logo ()
{
	cprintf("¯\\_(ツ)_/¯\n");
	// cprintf("                                 ,-\n 
	//                                ,'::|\n 
	//                               /::::|\n 
	//                             ,'::::o\\                                      _..\n 
	//          ____........-------,..::?88b                                  ,-' /\n 
	//  _.--"""". . . .      .   .  .  .  ""`-._                           ,-' .;'\n 
	// <. - :::::o......  ...   . . .. . .  .  .""--._                  ,-'. .;'\n 
	//  `-._  ` `\":`:`:`::||||:::::::::::::::::.:. .  ""--._ ,'|     ,-'.  .;'\n 
	//      \"\"\"_=--       //'doo.. ````:`:`::::::::::.:.:.:. .`-`._-'.   .;'\n 
	//          \"\"--.__     P(       \\               ` ``:`:``:::: .   .;'\n 
	//                 \"\\\"\"--.:-.     `.                             .:/\n 
	//                   \\. /    `-._   `.\"\"-----.,-..::(--\"\".\\\"\"`.  `:\\\n 
	//                    `P         `-._ \\          `-:\\          `. `:\\\n 
	//                                    \"\"            \"            `-._)  -Seal\n");
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

