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