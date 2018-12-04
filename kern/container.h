#include <inc/container.h>
#include <inc/env.h>

void init_container();
int add_container(char * root_str);
void remove_container(int cid);
int add_ipc(int cid, int from_pid, int to_pid, int value, void * srcva);
void process_ipc();
void calc_credit();
int dequeue_ipc(envid_t * from_pid_ptr, envid_t * to_pid_ptr, uint32_t * value_ptr, void ** srcva_ptr, unsigned * perm_ptr);
int dequeue_cont_ipc(int cid, envid_t * from_pid_ptr, envid_t * to_pid_ptr, uint32_t * value_ptr, void ** srcva_ptr, unsigned * perm_ptr);
int enqueue_cont_ipc(int cid, int from_pid, int to_pid, int value, void * srcva, int perm);
int isqueue(struct Env * e);
void calc_credit();
int cont_credit_use(int cid, int token_used);