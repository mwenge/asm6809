/*

DJB2 string hash function implementation

*/

#ifndef HASH_DJB2_H__InoWnIRELNBjE
#define HASH_DJB2_H__InoWnIRELNBjE

#include <stddef.h>

size_t hash_djb2(void const *x, size_t tablesize);
size_t hash_djb2_case(void const *x, size_t tablesize);

#endif
