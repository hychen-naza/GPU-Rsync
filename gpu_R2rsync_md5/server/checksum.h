#include "type.h"

#ifndef CHUNK_SUM_H
#define CHUNK_SUM_H
#define F(x,y,z) ((x & y) | (~x & z))  
#define G(x,y,z) ((x & z) | (y & ~z))  
#define H(x,y,z) (x^y^z)  
#define I(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) { a += F(b,c,d) + x + ac;  a = ROTATE_LEFT(a,s); a += b; }
#define GG(a,b,c,d,x,s,ac) { a += G(b,c,d) + x + ac;  a = ROTATE_LEFT(a,s); a += b; }
#define HH(a,b,c,d,x,s,ac) { a += H(b,c,d) + x + ac;  a = ROTATE_LEFT(a,s); a += b; }
#define II(a,b,c,d,x,s,ac) { a += I(b,c,d) + x + ac;  a = ROTATE_LEFT(a,s); a += b; }
struct MD5_CTX
{
    unsigned int count[2];
    unsigned int state[4];
    char buffer[64];   
};
#endif

void h_get_checksum2(char *buf, int32 len, char *sum);
void h_MD5Init(MD5_CTX *context);

void h_MD5Update(MD5_CTX *context, char *input,unsigned int inputlen);

void h_MD5Final(MD5_CTX *context, char digest[16]);

void h_MD5Transform(unsigned int state[4], char block[64]);

void h_MD5Encode( char *output,unsigned int *input,unsigned int len);

void h_MD5Decode(unsigned int *output, char *input,unsigned int len);

