#include <kern/container.h>
#include <inc/lib.h>

static int ipc_hold_cnt = 0;								// 
struct container_entry conts[CONTAINER_MAX_COUNT];
static LIST_HEAD(cont_free, container_entry) cont_free;
static LIST_HEAD(cont_active, container_entry) cont_active;
// credit allocation variables
unsigned int credit_reserves = 0;
int credit_cont_num = 0;

// setup memory structure for container allocation
void init_container() {
	int i;
	// set all container entry to free set
	for(i=0; i<CONTAINER_MAX_COUNT; i++) {
		conts[i].cid = i;
		conts[i].active = 0;
		LIST_INSERT_HEAD(&cont_free, &conts[i], link);
	}
}

// add container with root string, return cid or error(<0)
int add_container(const char * root_str) {
	// get a container from free container set
	struct container_entry *c = LIST_FIRST(&cont_free);
	if (!c) {
		cprintf("no container available\n");
		return NO_CONT_ERR;
    }
    LIST_REMOVE(c, link);

    // initalize container for use
    memcpy(c->root_str, root_str, strnlen(root_str, CHROOT_LEN));
    c->remaining_credit = 0;
    c->ipc_backptr = NULL;
    c->active = 1;
    // attach to active container list
    LIST_INSERT_HEAD(&cont_active, c, link);

    // for credit calculator function
    credit_cont_num += 1;
    if(credit_cont_num == 1) {
    	// start calc_credit timer
    }

    // return container id
    return c->cid;
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

	LIST_REMOVE(c, link);
	c->active = 0;
	LIST_INSERT_HEAD(&cont_free, c, link);
    credit_cont_num -= 1;
    if(credit_cont_num < 0) {
    	// stop calc_credit timer
    	credit_cont_num = 0;
    }
}

/*

// attach ipc message to message queue inside a container
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

// icp message queue processing loop
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
*/

void test_container() {
	init_container();
	add_container("/test/1");
	add_container("/test/2");
	add_container("/test/3");
	add_container("/test/4");
	remove_container(2);
	remove_container(2);
	remove_container(3);
	//add_ipc(1, 2, 3, 1, (void *)0);
	//process_ipc();
	return;
}

/* ----------------------- credit allocation ----------------------- */

// not finished
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




	if(ipc_hold_cnt == 0)
		goto out;

	// 1. get each credit port and calculate remained_credit
	if(credit_reserves > 0)
		credit_total += credit_reserves;
	//////////////////////////////////////////////////////////////////////////////////spin_lock_bh(&bca->credit_port_list_lock);
	list_for_each_entry_safe(temp_p, next_p, &bca->credit_port_list, cp_list) {
		// a. setting fair distribution of credit_total
		credit_fair = (credit_total + (ipc_hold_cnt-1) )/ ipc_hold_cnt;
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