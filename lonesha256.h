/*
Lonesha256.h - Portable, endian-proof, single-file, single-function sha256 implementation, originally based on LibTomCrypt

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

/*
Lonesha256 function:
(static|extern) int lonesha256 (unsigned char* out, const unsigned char* in, unsigned long inlen)
    Writes the sha256 hash of the first "inlen" bytes in buffer "in" to buffer "out".
    Returns 0 for success, may return non-zero in future versions to indicate error.
*/
#ifndef __LONESHA256_H_
#define __LONESHA256_H_

#define SHA256SUM_LEN 32

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//lonesha256 declaration
int lonesha256(unsigned char*, const unsigned char*, unsigned long);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif

