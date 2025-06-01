#include "../include.h"

// TODO: This is a placeholder encryption/decryption algorithm. ANYTHING would be better than this. Please replace. Disgusting.

// encrypt max len bytes from src to dst
// @src : encrypt from
// @dst : encrypt to
// @len : max bytes
void encrpyt(char* src, char* dst, int len) {
	for(int i = 0; i < len; i++) {
		// invalid
		if(!src[i]) { break; }

		// alphabet
		if(src[i] == 'z' || src[i] == 'Z') {
			dst[i] = src[i]-('z'-'a');
		} else if(isdigit(src[i])) {
			dst[i] = src[i]+1;
		}

		// number
		if(src[i] == '9') {
			dst[i] = src[i]-('9'-'0');
		} else if(isdigit(src[i])) {
			dst[i] = src[i]+1;
		}
	}
}

// decrypt max len bytes from src to dst
// @src : encrypt from
// @dst : encrypt to
// @len : max bytes
void decrpyt(char* src, char* dst, int len) {
	for(int i = 0; i < len; i++) {
		// invalid
		if(!src[i]) { break; }

		// alphabet
		if(src[i] == 'a' || src[i] == 'A') {
			dst[i] = src[i]+('z'-'a');
		} else if(isdigit(src[i])) {
			dst[i] = src[i]-1;
		}

		// number
		if(src[i] == '0') {
			dst[i] = src[i]+('9'-'0');
		} else if(isdigit(src[i])) {
			dst[i] = src[i]-1;
		}
	}
}
