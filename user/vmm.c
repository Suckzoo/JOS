#include <inc/lib.h>
#include <inc/vmx.h>
#include <inc/elf.h>
#include <inc/ept.h>
#include <inc/stdio.h>

#define GUEST_KERN "/vmm/kernel"
#define GUEST_BOOT "/vmm/boot"

#define JOS_ENTRY 0x7000

#define HANDLE_ERROR(ret) if ((ret) < 0) return (ret)

// Map a region of file fd into the guest at guest physical address gpa.
// The file region to map should start at fileoffset and be length filesz.
// The region to map in the guest should be memsz.  The region can span multiple pages.
//
// Return 0 on success, <0 on failure.
//
static int
map_in_guest( envid_t guest, uintptr_t gpa, size_t memsz, 
	      int fd, size_t filesz, off_t fileoffset ) {
	/* Your code here */

	envid_t srcid = sys_getenvid();
	size_t next_read = PGSIZE < filesz ? PGSIZE : filesz;
	off_t next_offset = fileoffset;
	int r;
	int perm = PTE_P | PTE_U | PTE_W;
	size_t total_map;
	for(total_map = 0; total_map < memsz; total_map += PGSIZE) {
		if (filesz > 0) {
			r = sys_page_alloc(srcid, UTEMP, perm);
			HANDLE_ERROR(r);
			r = seek(fd, next_offset);
			HANDLE_ERROR(r);
			off_t pgoff = PGOFF(gpa);
			size_t next_read = PGSIZE - pgoff < filesz ? PGSIZE - pgoff : filesz;
			int n = readn(fd, UTEMP + pgoff, next_read);
			HANDLE_ERROR(n);
			r = sys_ept_map(srcid, UTEMP, guest, (void*)gpa - pgoff, perm);
      HANDLE_ERROR(r);
			filesz -= n;
			gpa += PGSIZE - pgoff;
			next_offset += next_read;
			r = sys_page_unmap(srcid, UTEMP);
			HANDLE_ERROR(r);
		} else {
			r = sys_page_alloc(srcid, UTEMP, __EPTE_FULL);
			HANDLE_ERROR(r);
			off_t pgoff = PGOFF(gpa);
			r = sys_ept_map(srcid, UTEMP, guest, (void*)gpa - pgoff, __EPTE_FULL);
			HANDLE_ERROR(r);
			r = sys_page_unmap(srcid, UTEMP);
			HANDLE_ERROR(r);
			gpa += PGSIZE - pgoff;
		}
	}
	return 0;
} 

// Read the ELF headers of kernel file specified by fname,
// mapping all valid segments into guest physical memory as appropriate.
//
// Return 0 on success, <0 on error
//
// Hint: compare with ELF parsing in env.c, and use map_in_guest for each segment.
static int
copy_guest_kern_gpa( envid_t guest, char* fname ) {

	/* Your code here */
	int r;
	envid_t srcid = sys_getenvid();
	int fd = open(fname, O_RDONLY);
	HANDLE_ERROR(fd);

	uint8_t binary[512];
	r = readn(fd, binary, 512);
	if (r < 0) {
		close(fd);
		return r;
	}

	struct Elf* elf = (struct Elf*)binary;
	if (elf->e_magic != ELF_MAGIC) {
		return -E_NOT_EXEC;
	}

	// Let's load kern elf!
	struct Proghdr *ph, *eph;
	ph  = (struct Proghdr *)((uint8_t *)elf + elf->e_phoff);
	eph = ph + elf->e_phnum;
	for(;ph < eph; ph++) {
		if (ph->p_type == ELF_PROG_LOAD) {
			r = map_in_guest(guest, ph->p_pa, ph->p_memsz, fd, ph->p_filesz, ph->p_offset);
			if (r < 0) {
				close(fd);
				return r;
			}
		}
	}
	
	return 0;
}

void
umain(int argc, char **argv) {

	int ret;
	envid_t guest;
	char filename_buffer[50];	//buffer to save the path 
	int vmdisk_number;
	int r;
	if ((ret = sys_env_mkguest( GUEST_MEM_SZ, JOS_ENTRY )) < 0) {
		cprintf("Error creating a guest OS env: %e\n", ret );
		exit();
	}
	guest = ret;

	// Copy the guest kernel code into guest phys mem.
	if((ret = copy_guest_kern_gpa(guest, GUEST_KERN)) < 0) {
		cprintf("Error copying page into the guest - %d\n.", ret);
		exit();
	}

	// Now copy the bootloader.
	int fd;
	if ((fd = open( GUEST_BOOT, O_RDONLY)) < 0 ) {
		cprintf("open %s for read: %e\n", GUEST_BOOT, fd );
		exit();
	}

	// sizeof(bootloader) < 512.
	if ((ret = map_in_guest(guest, JOS_ENTRY, 512, fd, 512, 0)) < 0) {
		cprintf("Error mapping bootloader into the guest - %d\n.", ret);
		exit();
	}

#ifndef VMM_GUEST	
	sys_vmx_incr_vmdisk_number();	//increase the vmdisk number
	//create a new guest disk image

	vmdisk_number = sys_vmx_get_vmdisk_number();
	snprintf(filename_buffer, 50, "/vmm/fs%d.img", vmdisk_number);

	cprintf("Creating a new virtual HDD at /vmm/fs%d.img\n", vmdisk_number);
	r = copy("vmm/clean-fs.img", filename_buffer);

	if (r < 0) {
		cprintf("Create new virtual HDD failed: %e\n", r);
		exit();
	}

	cprintf("Create VHD finished\n");
#endif

	// Mark the guest as runnable.
	sys_env_set_status(guest, ENV_RUNNABLE);
	wait(guest);
}


