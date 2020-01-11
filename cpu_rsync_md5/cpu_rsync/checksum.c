#include "checksum.h"

uint32 get_checksum1(char *buf1, int32 len, int *d_s1, int *d_s2)
{
    int32 i;
    uint32 s1, s2;
    char *buf = (char *)buf1;

    s1 = s2 = 0;
    for (i = 0; i < (len-4); i+=4) {
	s2 += 4*(s1 + buf[i]) + 3*buf[i+1] + 2*buf[i+2] + buf[i+3] +
	  10*CHAR_OFFSET; 
	s1 += (buf[i+0] + buf[i+1] + buf[i+2] + buf[i+3] + 4*CHAR_OFFSET);
    }
    for (; i < len; i++) {
	s1 += (buf[i]+CHAR_OFFSET); s2 += s1;
    }
    // return s1,s2 for the next time use
    *d_s1 = s1;
    *d_s2 = s2;
    return (s1 & 0xffff) + (s2 << 16);
}

void get_checksum2(char *buf, int32 len, char *sum)
{
    MD5_CTX md5;  //定义一个MD5 text
    MD5Init(&md5);//初始化           
    MD5Update(&md5, buf, len);//进行初步分组加密  
    MD5Final(&md5, sum);   //进行后序的补足，并加密 
}
