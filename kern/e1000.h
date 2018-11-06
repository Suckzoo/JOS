
#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H


#include <kern/pci.h>

int e1000_attach(struct pci_func *pcif);
int e1000_transmit(const char *buf, unsigned int len);
int e1000_receive(char *buf, unsigned int len);



#endif	// JOS_KERN_E1000_H
