
/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_CON_H
#define JOS_INC_CON_H

#include <inc/queue.h>
#include <inc/fs.h>
#include <kern/spinlock.h>
#include <inc/types.h>

#define CONTAINER_MAX_COUNT	5
#define MESSAGE_QUEUE_MAX_COUNT	10000	
#define CHROOT_LEN	100
#define MAX_CREDIT	10000

#define NO_CONT_ERR (-1)
#define NO_IPC_ERR (-2)
#define NO_ACTIVE_CONTAINER_ERR	(-3)

typedef uint32_t cid_t;

struct ipc_entry {
	int from;
	int to;
	int value;
	void * srcva;
	int perm;

	LIST_ENTRY(ipc_entry) link;
};
struct container_entry {
	int cid;
	int active;
	char root_str[CHROOT_LEN];
	int ref_cnt;
	int remaining_credit;

	struct ipc_entry ipcs[MESSAGE_QUEUE_MAX_COUNT];
	LIST_HEAD(ipc_free, ipc_entry) ipc_free;
	LIST_HEAD(ipc_active, ipc_entry) ipc_active;
	struct ipc_entry *ipc_backptr;
	// do not require send back ipc processing, because it goes directly to the process

    LIST_ENTRY(container_entry) link;

    // locks
    struct spinlock ipc_free_lk;
    struct spinlock ipc_active_lk;

};

#endif // !JOS_INC_CON_H
