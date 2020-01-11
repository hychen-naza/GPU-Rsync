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

#define CHAR_OFFSET 0

//储存一个MD5 text信息 
typedef struct
{
    unsigned int count[2];
    //记录当前状态，其数据位数   

    unsigned int state[4];
    //4个数，一共32位 记录用于保存对512bits信息加密的中间结果或者最终结果  

    char buffer[64];
    //一共64字节，512位      
}MD5_CTX;
#endif

uint32 get_checksum1(char *buf1, int32 len, int *d_s1, int *d_s2);
void get_checksum2(char *buf, int32 len, char *sum);
void MD5Init(MD5_CTX *context);

void MD5Update(MD5_CTX *context, char *input,unsigned int inputlen);

void MD5Final(MD5_CTX *context, char digest[16]);

void MD5Transform(unsigned int state[4], char block[64]);

void MD5Encode( char *output,unsigned int *input,unsigned int len);

void MD5Decode(unsigned int *output, char *input,unsigned int len);

