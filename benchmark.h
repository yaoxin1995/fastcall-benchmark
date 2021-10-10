

#pragma once

#include <sys/ioctl.h>
#include <unistd.h>

#define SIZE_OF_SAMPLE 1000000


#define IOCTL_REGISTRATION 0
#define IOCTL_DEREGISTRATION 1
#define IOCTL_NOOP 2

#define FCE_DEVICE_FILE "/dev/fastcall-examples"

#define NR_SYS_NOOP 443

