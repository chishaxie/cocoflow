#ifndef __SIMPLE_RAMD_H__
#define __SIMPLE_RAMD_H__

/* Just to remove the platform differences */
static int simple_rand()
{
	static unsigned long seed = 404264294;
	return (((seed = seed * 214013L + 2531011L) >> 16) & 0x7fff);
}

#endif
