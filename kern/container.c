#include <inc/queue.h>

#define CONTAINER_MAX_COUNT	5
#define MESSAGE_QEUEUE_MAX_COUNT	10	
#define CHROOT_LEN	20

#define NO_CONT_ERR (-1)
#define NO_IPC_ERR (-2)
#define NO_ACTIVE_CONTAINER_ERR	(-3)

struct ipc_entry {
	int from;
	int to;
	int value;
	void * srcva;

	LIST_ENTRY(ipc_entry) link;
};
struct container_entry {
	int cid;
	int active;
	char root_str[CHROOT_LEN];
	int balance;

	struct ipc_entry ipc_fronts[MESSAGE_QEUEUE_MAX_COUNT];
	LIST_HEAD(ipc_free, ipc_entry) ipc_free;
	LIST_HEAD(ipc_active, ipc_entry) ipc_active;
	struct ipc_entry *ipc_backptr;
	// do not require send back ipc processing, because it goes directly to the process

    LIST_ENTRY(container_entry) link;
};
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
			}
		}
	}
}