#include "../include/errno.h"

static int klibc_errno_value;

int *klibc_errno_location(void)
{
    return &klibc_errno_value;
}