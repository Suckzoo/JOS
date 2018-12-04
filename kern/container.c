#include <kern/container.h>
#include <inc/lib.h>

struct container_entry conts[CONTAINER_MAX_COUNT];
static LIST_HEAD(cont_free, container_entry) cont_free;
static LIST_HEAD(cont_active, container_entry) cont_active;
int current_dequeue_idx = 0;
// credit allocation variables
unsigned int credit_reserves = 0;
int cont_cnt = 0;
int ipc_total_cnt = 0;
struct Env * jocker_env = NULL;

// locks
struct spinlock cont_free_lk;
struct spinlock cont_active_lk;

// setup memory structure for container allocation
void init_container() {
	int i, j;
	// set all container entry to free set
	for(i=0; i<CONTAINER_MAX_COUNT; i++) {
		conts[i].cid = i;
		conts[i].active = 0;
		LIST_INSERT_HEAD(&cont_free, &conts[i], link);
		for(j=0; j<MESSAGE_QEUEUE_MAX_COUNT; j++) {
			LIST_INSERT_HEAD(&(conts[i].ipc_free), &(conts[i].ipcs[j]), link);
		}
		spin_initlock(&conts[i].ipc_free_lk);
		spin_initlock(&conts[i].ipc_active_lk);
	}
	spin_initlock(&cont_free_lk);
	spin_initlock(&cont_active_lk);
}

// add container with root string, return cid or error(<0)
int add_container(char * root_str) {
	// get a container from free container set
	// LOCK free cont
	spin_lock(&cont_free_lk);
	struct container_entry *c = LIST_FIRST(&cont_free);
	if (!c) {
		// UNLOCK free cont
		spin_unlock(&cont_free_lk);
		cprintf("no container available\n");
		return NO_CONT_ERR;
    }
    LIST_REMOVE(c, link);
    // UNLOCK free cont
	spin_unlock(&cont_free_lk);

    // initalize container for use
    memcpy(c->root_str, root_str, CHROOT_LEN);
    c->remaining_credit = 0;
    c->ipc_backptr = NULL;
    c->active = 1;
    c->ref_cnt = 0;
    // attach to active container list
    // LOCK cont_active
	spin_lock(&cont_active_lk);
    LIST_INSERT_HEAD(&cont_active, c, link);
    cont_cnt ++;
    // UNLOCK cont_active
	spin_unlock(&cont_active_lk);

    // return container id
    return c->cid;
}

int incref_container(int cid) {
	if(cid < 0 || cid > CONTAINER_MAX_COUNT-1) {
		return -1;
	}
	// LOCK creat & ref_cnt
	if(!conts[cid].active) {
		// UNLOCK creat & ref_cnt
		return -1;
	}
	conts[cid].ref_cnt++;
	// UNLOCK creat & ref_cnt
	return 0;
}
// this will not automatically remove container.
int decref_container(int cid) {
	if(cid < 0 || cid > CONTAINER_MAX_COUNT-1) {
		return -1;
	}
	// LOCK creat & ref_cnt
	if(!conts[cid].active) {
		// LOCK creat & ref_cnt
		return -1;
	}
	conts[cid].ref_cnt--;
	if(conts[cid].ref_cnt < 1)
		remove_container(cid);
	// LOCK creat & ref_cnt
	return 0;
}


// remove container with cid
void remove_container(int cid) {
	struct container_entry *c;
	// error: out of cid boundary
	if(cid > CONTAINER_MAX_COUNT-1)
		return;
	c = &conts[cid];
	// error: container not active
	if(!c->active)
		return;
	if(c->ref_cnt != 0)
		return;
	// LOCK cont_cnt
	spin_lock(&cont_active_lk);
	LIST_REMOVE(c, link);
    cont_cnt -= 1;
	spin_unlock(&cont_active_lk);
	c->active = 0;
	// UNLOCK cont_cnt
    // LOCK cont free
	spin_lock(&cont_free_lk);
	LIST_INSERT_HEAD(&cont_free, c, link);
    // UNLOCK cont free
	spin_unlock(&cont_free_lk);
}

int isqueue(struct Env * e) {
	// LOCK IPC TOTAL CNT
	if(ipc_total_cnt < 1) {
		jocker_env = e;
		jocker_env->env_status = ENV_NOT_RUNNABLE;
		// UNLOCK IPC TOTAL CNT
		return 0;
	} else {
		// UNLOCK IPC TOTAL CNT
		return 1;
	}
}

int cont_credit_use(int cid, int token_used) {
	struct container_entry *c = &conts[cid];
	// LOCK cont CREDIT
	if(c->remaining_credit - token_used < 0) {
		// UNLOCK cont CREDIT
		return -1;
	}
	else {
		c->remaining_credit -= token_used;
		// UNLOCK cont CREDIT
		return 0;
	}
}

// attach ipc message to message queue inside a container
int enqueue_cont_ipc(int cid, int from_pid, int to_pid, int value, void * srcva, int perm) {
	struct container_entry *c = &conts[cid];
	struct ipc_entry *i;
#ifdef RW_BYTE_BASED_CREDIT
	int req_n=0;
	struct Fsreq_read* req_rd;
	struct Fsreq_write* req_wrt;
#endif
	if (!c->active) {
		cprintf("no active container\n");
		return NO_ACTIVE_CONTAINER_ERR;
	}

	// LOCK ipc_free
	spin_lock(&c->ipc_free_lk);
	i = LIST_FIRST(&c->ipc_free);
	if(!i) {
		// UNLOCK ipc_free
		spin_unlock(&c->ipc_free_lk);
		//cprintf("no ipc available\n");
		return -E_IPC_NOT_RECV;
	}

#ifndef RW_BYTE_BASED_CREDIT
	/* CASE 1: reduction per ipc */
	if(cont_credit_use(cid, 1)<0) {
		// UNLOCK ipc_free
		spin_unlock(&c->ipc_free_lk);
		//cprintf("no credit\n");
		return -E_IPC_NOT_RECV;
	}
#else
	/* CASE 2: reduction per byte if READ/WRITE */
	if(value == FSREQ_READ || value == FSREQ_WRITE) {
		if(value == FSREQ_READ) {
			req_rd = (struct Fsreq_read*) srcva;
			req_n = req_rd->req_n;
		} else if(value == FSREQ_WRITE) {
			req_wrt = (struct Fsreq_write*) srcva;
			req_n = req_wrt->req_n;
		}
		if(req_n < 1) {
			// UNLOCK ipc_free
			spin_unlock(&c->ipc_free_lk);
			return -E_IPC_NOT_RECV;
		}
		if(cont_credit_use(cid, req_n)<0) {
			// UNLOCK ipc_free
			spin_unlock(&c->ipc_free_lk);
			//cprintf("no credit\n");
			return -E_IPC_NOT_RECV;
		}	
	}
#endif
	
	
	LIST_REMOVE(i, link);
	// UNLOCK ipc_free
	spin_unlock(&c->ipc_free_lk);
	
	spin_lock(&c->ipc_active_lk);
	i->from = from_pid;
	i->to = to_pid;
	i->value = value;
	i->srcva = srcva;
	i->perm = perm;
    if(!c->ipc_backptr) {
		LIST_INSERT_HEAD(&c->ipc_active, i, link);
	}
	else {
		LIST_INSERT_AFTER(c->ipc_backptr, i, link);
		//LIST_INSERT_HEAD(&c->ipc_active, i, link);
	}
	c->ipc_backptr = i;
	spin_unlock(&c->ipc_active_lk);

	// LOCK TOTAL IPC
	ipc_total_cnt++;
	// UNLOCK TOTAL IPC

	if(jocker_env && jocker_env->env_status == ENV_NOT_RUNNABLE)
		jocker_env->env_status = ENV_RUNNABLE;
	return 0;
}

// return EMPTY_CONT_QUEUE for empty situation
// return value for normal message
int dequeue_cont_ipc(int cid, envid_t * from_pid_ptr, envid_t * to_pid_ptr, uint32_t * value_ptr, void ** srcva_ptr, unsigned * perm_ptr) {
	// pick one ipc_entry, copy [from, srcva, perm] to [from_env_store, pg, perm_store]
	struct container_entry *c = &conts[cid];
	struct ipc_entry *i;
	if(!c->active)
		return -1;
	// LOCK IPC LIST
	spin_lock(&c->ipc_active_lk);
	i = LIST_FIRST(&c->ipc_active);
	if(!i) {
		spin_unlock(&c->ipc_active_lk);
		return -1;
	}
	else {
		// LOCK TOTAL IPC
		ipc_total_cnt--;
		// UNLOCK TOTAL IPC
		LIST_REMOVE(i, link);
		if(c->ipc_backptr == i) {
			c->ipc_backptr = NULL;
		}
    	*from_pid_ptr = i->from;
		*to_pid_ptr = i->to;
		*value_ptr = i->value;
		*srcva_ptr = i->srcva;
		*perm_ptr = i->perm;
		// LOCK ipc_free
		spin_lock(&c->ipc_free_lk);
		LIST_INSERT_HEAD(&c->ipc_free, i, link);
		// UNLOCK ipc_free
		spin_unlock(&c->ipc_free_lk);
		// UNLOCK IPC LIST
		spin_unlock(&c->ipc_active_lk);
		return 0;
	}

}

// return EMPTY_CONT_QUEUE for empty situation
int dequeue_ipc(envid_t * from_pid_ptr, envid_t * to_pid_ptr, uint32_t * value_ptr, void ** srcva_ptr, unsigned * perm_ptr) {
	int r;
	int i = current_dequeue_idx;
	if(cont_cnt == 0)
		return -1;
	do {
		r = dequeue_cont_ipc(i, from_pid_ptr, to_pid_ptr, value_ptr, srcva_ptr, perm_ptr);
		if(r > -1) {
			current_dequeue_idx = (i+1)%CONTAINER_MAX_COUNT;
			return 0;
		}
		i = (i+1)%CONTAINER_MAX_COUNT;
	} while(i != current_dequeue_idx);
	return -1;
}

/* ----------------------- credit allocation ----------------------- */

void calc_credit() {
	struct container_entry * c;
	unsigned int credit_total = MAX_CREDIT;
	unsigned int credit_fair;
	unsigned int credit_left = 0;
	int container_count = cont_cnt;
	// LOCK cont_active
	spin_lock(&cont_active_lk);
	if(cont_cnt == 0) {
		// UNLOCK cont_active
		spin_unlock(&cont_active_lk);
		return;
	}
	if(credit_reserves > 0)
		credit_total += credit_reserves;
	LIST_FOREACH(c, &cont_active, link) {
		credit_fair = (credit_total + (cont_cnt-1))/cont_cnt;
		c->remaining_credit += credit_fair;

		if(c->remaining_credit >= MAX_CREDIT) {
			credit_left += (c->remaining_credit - credit_fair);
			if(container_count > 1) {
				c->remaining_credit = credit_fair;
			} else {
				c->remaining_credit = MAX_CREDIT;
			}
		}
	}
	// UNLOCK cont_active
	spin_unlock(&cont_active_lk);
	credit_reserves += credit_left;
}


/*
void calc_credit() {
	// 0. setting before credit distribution
	struct bridge_credit_allocator *bca = NULL;
	struct net_bridge_port *temp_p, *next_p;
	int total;
	unsigned int weight_left;
	unsigned int credit_left = 0;
	unsigned int credit_total = MAX_CREDIT ;
	unsigned int credit_fair;
	int credit_extra = 0;
	struct timer_list* ptimer;
	temp_p = next_p = NULL;




	if(cont_cnt == 0)
		goto out;

	// 1. get each credit port and calculate remaining_credit
	if(credit_reserves > 0)
		credit_total += credit_reserves;
	//////////////////////////////////////////////////////////////////////////////////spin_lock_bh(&bca->credit_port_list_lock);
	list_for_each_entry_safe(temp_p, next_p, &bca->credit_port_list, cp_list) {
		// a. setting fair distribution of credit_total
		credit_fair = (credit_total + (cont_cnt-1) )/ cont_cnt;
		temp_p->remaining_credit += credit_fair;
		// b. checking given credit

		if(temp_p->remaining_credit < MAX_CREDIT) {
			credit_extra = 1;
		} else {
			credit_left += (temp_p->remaining_credit - credit_fair);

			if(weight_left != 0U){
				credit_total += ((credit_left*total)+(weight_left - 1))/weight_left;
				credit_left=0;
			}
			if(credit_extra){
				list_del(&temp_p->cp_list);
				list_add(&temp_p->cp_list, &bca->credit_port_list);
			}
			
			if(bca->credit_port_num > 1)
				temp_p->remaining_credit = credit_fair;
			else
				temp_p->remaining_credit = MAX_CREDIT;
		}
skip:
	}
	spin_unlock_bh(&bca->credit_port_list_lock);
	credit_reserves = credit_left;
out:
	// 2. set recursion timer
	mod_timer(&bca->credit_timer, jiffies + msecs_to_jiffies(10));
}
*/