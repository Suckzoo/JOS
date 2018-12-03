#include <inc/container.h>

void init_container();
int add_container(const char * root_str);
void remove_container(int cid);
int add_ipc(int cid, int from_pid, int to_pid, int value, void * srcva);
void process_ipc();
void test_container();
void calc_credit();
