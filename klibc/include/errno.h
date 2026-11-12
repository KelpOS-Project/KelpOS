#ifndef KLIBC_ERRNO_H
#define KLIBC_ERRNO_H

#define EDOM 33
#define EINVAL 22
#define ERANGE 34

int *klibc_errno_location(void);
#define errno (*klibc_errno_location())

#endif