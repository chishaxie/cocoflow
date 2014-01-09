#define _GNU_SOURCE

#include <string.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <linux/sysctl.h>

int get_max_map_count()
{
	int ret = 0;
	
	struct __sysctl_args args;
	int name[] = {CTL_VM, VM_MAX_MAP_COUNT};
	size_t len = sizeof(ret);
	
	memset(&args, 0, sizeof(args));
	args.name = name;
	args.nlen = sizeof(name)/sizeof(name[0]);
	args.oldval = &ret;
	args.oldlenp = &len;
	
	/* int _sysctl(struct __sysctl_args *args); */
	if (syscall(SYS__sysctl, &args) == -1)
		return -1;
	else
		return ret;
}
