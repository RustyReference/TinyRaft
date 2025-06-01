#include "../include.h"

#ifndef ENCRYPT_H
#define ENCRYPT_H

// encrypt max len bytes from src to dst
// @src : encrypt from
// @dst : encrypt to
// @len : max bytes
void encrypt(char src[255], char dst[255], int len);

// decrypt max len bytes from src to dst
// @src : encrypt from
// @dst : encrypt to
// @len : max bytes
void decrypt(char src[255], char dst[255], int len);

#endif
