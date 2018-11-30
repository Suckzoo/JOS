#include <kern/container.h>

static int ipc_hold_cnt = 0;
static struct container_entry conts[CONTAINER_MAX_COUNT];
static LIST_HEAD(cont_free, container_entry) cont_free;
static LIST_HEAD(cont_active, container_entry) cont_active;

void init_container() {
	int i;
	// set all container entry to free set
	for(i=0; i<CONTAINER_MAX_COUNT; i++) {
		conts[i].cid = i;
		conts[i].active = 0;
		LIST_INSERT_HEAD(&cont_free, &conts[i], link);
	}
}

int add_container(char * root_str) {
	struct container_entry *c = LIST_FIRST(&cont_free);
	if (!c) {
		cprintf("no container available\n");
		return NO_CONT_ERR;
    }
    LIST_REMOVE(c, link);
    memcpy(c->root_str, root_str, CHROOT_LEN);
    c->balance = 0;
    c->ipc_backptr = NULL;
    c->active = 1;
    LIST_INSERT_HEAD(&cont_active, c, link);
    return 0;
}

void remove_container(int cid) {
	struct container_entry *c = &conts[cid];
	if(c->active) {
		LIST_REMOVE(c, link);
		c->active = 0;
		LIST_INSERT_HEAD(&cont_free, c, link);
	}
}

int add_ipc(int cid, int from_pid, int to_pid, int value, void * srcva) {
	struct container_entry *c = &conts[cid];
	struct ipc_entry *i = LIST_FIRST(&c->ipc_free);
	if (!c->active) {
		cprintf("no active container\n");
		return NO_ACTIVE_CONTAINER_ERR;
	}
	if(!i) {
		cprintf("no ipc available\n");
		return NO_IPC_ERR;
	}

	// 1. CHECK if OP is open, check access request
	//      IF it is not valid, return ERROR
	// 2. TRY reduce container credit, BY rule of IPC count / data size(in this case, also check OP and srcva to inspect how much it will take)
	//      IF it is not avalilable, return ERROR

	LIST_REMOVE(i, link);
	i->from = from_pid;
	i->to = to_pid;
	i->value = value;
	i->srcva = srcva;

	if(!c->ipc_backptr) 
		LIST_INSERT_HEAD(&c->ipc_active, i, link);
	else
		LIST_INSERT_AFTER(c->ipc_backptr, i, link);
	c->ipc_backptr = i;
	ipc_hold_cnt += 1;

	// TODO: wakeup worker thread For process_ipc

	return 0;
}

void process_ipc() {
	struct container_entry *c;
	struct ipc_entry *i;
	while(1) {
		LIST_FOREACH(c, &cont_active, link) {
			i = LIST_FIRST(&c->ipc_active);
			if(!i)
				continue;
			LIST_REMOVE(i, link);
			if(c->ipc_backptr == i)
				c->ipc_backptr = NULL;
			// TODO: send ipc based on i;
			LIST_INSERT_HEAD(&c->ipc_free, i, link);
			ipc_hold_cnt--;
				if(ipc_hold_cnt<1) {
				//TODO: set sleep

				// return need to be removed when sleep is implemented
				return;
			}
		}
	}
}

void test_container() {
	init_container();
	add_container("/test/1");
	add_container("/test/2");
	add_container("/test/3");
	add_container("/test/4");
	remove_container(2);
	remove_container(2);
	remove_container(3);
	add_ipc(1, 2, 3, 1, (void *)0);
	add_ipc(2, 2, 3, 2, (void *)0);
	add_ipc(4, 2, 3, 3, (void *)0);
	add_ipc(4, 2, 3, 4, (void *)0);
	add_ipc(4, 2, 3, 5, (void *)0);
	add_ipc(4, 2, 3, 6, (void *)0);
	printf("%d\n", ipc_hold_cnt);
	process_ipc();
	return;
}