#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdint.h>
#include <vector>
#include <sys/types.h>
#include <math.h>
#include "hash.h"
#include "checksum.h"


void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads, char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j);
uint32 h_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2);
Node *lookup_ht(Node *ht, int32 rc, int *chunk_id);
__device__ uint32 d_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2);
__device__ void d_get_checksum2(char *buf, int32 len, char *sum);
__device__ void MD5Init(MD5_CTX *context);
__device__ void MD5Update(MD5_CTX *context, char *input,unsigned int inputlen);
__device__ void MD5Transform(unsigned int state[4], char block[64]);
__device__ void MD5Decode(unsigned int *output, char *input,unsigned int len);
__device__ void MD5Encode(char *output,unsigned int *input,unsigned int len);
__device__ void MD5Final(MD5_CTX *context, char digest[16]);
__device__ uint d_hash(uint32 rc);
__device__ Node *d_lookup_ht(Node *ht, int32 rc, int *chunk_id);

__constant__ char PADDING[]={(char)0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  


__global__ void oldfile_match_build(char *dFileContent, int *dmatchIdArray, int chunk_size, int chunk_num, int *dMatchOffset, int *dMatchChunkid, char *d_new_file, int totalChunkNum, int *multi_match_array, int *multi_match_num){
  int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
  if(thread_id < totalChunkNum){
    int match_pos = thread_id;
    int chunk_id = dMatchChunkid[match_pos];
    if(chunk_id == -1) return;
    if(dmatchIdArray[chunk_id] != -1){
      int old_file_pos = dMatchOffset[match_pos];
      int new_file_pos = chunk_id * chunk_size;
      memcpy(&d_new_file[new_file_pos], &dFileContent[old_file_pos], chunk_size);
    }
    else{
      int old = atomicAdd(multi_match_num, 1);
      multi_match_array[old] = match_pos;
    }
  }
}

__global__ void multiwarp_match(Node *ht, char *file, size_t file_len, int total_threads, int chunk_size, int chunk_num, 
          int *match_offset, int *match_chunkid, int *stat)
{
  int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
  int fileBeginPos = chunk_num*chunk_size*thread_id;
  int chunkBeginPos = chunk_num*thread_id;
  //printf("block %d, thread %d, id %d, filebeginpos %d, total file len %d\n",blockIdx.x,threadIdx.x, thread_id, fileBeginPos, file_len);
  if(fileBeginPos < file_len){
    int recalcu = 1;
    uint32 rc;
    int chunk_id;
    int match_num = 0;
    int i = 0;
    uint32 s1 = 0, s2 = 0;
    //the char in the head of a chunk, it can be used to store as the unmatch value and use to recalcu
    char chunk_head_value;
    int length = chunk_size;

    length = chunk_size;
    for(; i < chunk_size*chunk_num;){
      //剩下的内容以及不够一个chunk_size
      if(fileBeginPos+i>file_len-chunk_size){
        length = file_len-fileBeginPos-i;
      }      
      if(recalcu == 1) rc = d_get_checksum1(&file[fileBeginPos + i], length, &s1, &s2);
      else if(recalcu == 0){
        s1 -= chunk_head_value + CHAR_OFFSET; 
        s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
        s1 += file[fileBeginPos+i+length-1] + CHAR_OFFSET;
        s2 += s1;
        rc = (s1 & 0xffff) + (s2 << 16);
      }
      chunk_head_value = file[fileBeginPos+i];
      Node *np = d_lookup_ht(ht, rc, &chunk_id);
      if(np == NULL){
        recalcu = 0;
        i += 1;
      }
      else{
        char sum2[16];
        bool md5match = true;
        d_get_checksum2(&file[fileBeginPos+i], length, sum2); 
        while(1){       
          md5match = true;        
          for(int j=0;j<16;++j){
              if(sum2[j]!=np->md5[j]){                  
                  md5match = false;
                  break;
              }
          }
          if(md5match){
            match_chunkid[chunkBeginPos + match_num] = np->chunk_id;
            match_offset[chunkBeginPos + match_num] = fileBeginPos + i;
            match_num ++;
            recalcu = 1;
            i += chunk_size;
            break;
          }
          else{
            np = np->next;
            if(np == NULL){
              recalcu = 0;
              i += 1;
              break;
            }
          }
        }        
      }     
    }
    //record match_num
    stat[thread_id] = match_num;
  }
}

__global__ void gpu_recalcu(Node *ht, char *file, int chunk_size, int chunk_num, int *match_offset, int *match_chunkid, int *stat, int region_size)
{
  int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
  int start_t = thread_id * region_size;
  //printf("thread %d start recalcu on %d thread, region size %d\n", thread_id, start_t, region_size);
  for(int i=start_t; i<start_t+region_size-1; ++i){
    //printf("thread %d recalcu on its %d thread\n", thread_id, i-start_t);
    int t_match_num = stat[i];
    int j = i+1; 
    int jump_pos = match_offset[chunk_num*i+t_match_num-1]+chunk_size; 
    if(t_match_num > 0 && stat[j] > 0 && jump_pos > match_offset[chunk_num*j]){
      //std::cout << "thread " << start_t << " need recalcu" << std::endl;         
      int match_index = 0;
      int recalcu = 1;
      int chunk_id;
      int j_match_num = stat[j];
      int j_match_begin = chunk_num*j;
      char chunk_head_value;
      uint32 s1, s2, rc;
      while(1){
        if(recalcu == 1) rc = d_get_checksum1(&file[jump_pos], chunk_size, &s1, &s2);   
        else if(recalcu == 0){
          s1 -= chunk_head_value + CHAR_OFFSET; 
          s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
          s1 += file[jump_pos+chunk_size-1] + CHAR_OFFSET;
          s2 += s1;
          rc = (s1 & 0xffff) + (s2 << 16);
        }    
        while(jump_pos > match_offset[j_match_begin+match_index]){            
          if(match_index < j_match_num){
            match_chunkid[j_match_begin+match_index] = -1;
            stat[j]--;
            match_index++;
          } 
          else break;
        }
        if(jump_pos == match_offset[j_match_begin+match_index]) break;

        Node *np = d_lookup_ht(ht, rc, &chunk_id);
        if(np == NULL){
          recalcu = 0;
          jump_pos += 1;
        }
        else{
          char sum2[16];
          bool md5match = true;
          d_get_checksum2(&file[jump_pos], chunk_size, sum2); 
          while(1){       
            md5match = true;        
            for(int k=0;k<16;++k){
                if(sum2[k]!=np->md5[k]){                  
                    md5match = false;
                    break;
                }
            }
            if(md5match){
              for(int k=j_match_begin;k<j_match_begin+chunk_num;++k){         
                if(match_chunkid[k]==-1 || jump_pos+chunk_size > match_offset[k]){
                  match_offset[k] = jump_pos;
                  match_chunkid[k] = chunk_id;
                  stat[j]++;
                  break;
                }
                else{
                  printf("error in 324 in new_file.cu\n");
                }
              }
              jump_pos += chunk_size;        
              recalcu = 1;
              break;
            }
            else{
              np = np->next;
              if(np == NULL){
                chunk_head_value = file[jump_pos];
                jump_pos += 1;
                recalcu = 0;
                break;
              }
            }
          }        
        }
        if(match_index >= j_match_num) break;
      }
    }  
  }        
}




void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads,
            char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j, int recalcu_region_size){
  int match_index = 0;
  int unmatch_index = 0; // 
  int recalcu = 1;
  int chunk_id;
  int length = chunk_size;
  int j_match_num = 0;
  for(int i=0;i<recalcu_region_size;++i){
    j_match_num += stat[j+i];
  }
  int j_match_begin = chunk_num*j;
  char chunk_head_value;
  uint32 s1, s2, rc;
  while(1){
    if(recalcu == 1) rc = h_get_checksum1(&h_file[jump_pos], length, &s1, &s2);   
    else if(recalcu == 0){
      s1 -= chunk_head_value + CHAR_OFFSET; 
      s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
      s1 += h_file[jump_pos+length-1] + CHAR_OFFSET;
      s2 += s1;
      rc = (s1 & 0xffff) + (s2 << 16);
    }
    while(jump_pos > match_offset[j_match_begin+match_index+unmatch_index]){
      if(match_chunkid[j_match_begin+match_index+unmatch_index] == -1){
        unmatch_index += 1;
      }
      else if(match_index < j_match_num){
        match_chunkid[j_match_begin+match_index+unmatch_index] = -1;
        //stat[j]--;
        match_index++;
      } 
      else break;
    }
    if(jump_pos == match_offset[j_match_begin+match_index+unmatch_index] && match_chunkid[j_match_begin+match_index+unmatch_index] != -1) break;

    Node *np = lookup_ht(ht, rc, &chunk_id);
    if(np == NULL){
      recalcu = 0;
      jump_pos += 1;
    }
    else{
      char sum2[16];
      bool md5match = true;
      h_get_checksum2(&h_file[jump_pos], length, sum2); 
      while(1){       
        md5match = true;        
        for(int k=0;k<16;++k){
            if(sum2[k]!=np->md5[k]){                  
                md5match = false;
                break;
            }
        }
        if(md5match){
          for(int k=j_match_begin;k<j_match_begin+chunk_num*recalcu_region_size;++k){   
            //已经被置为-1或者目前还没有但马上会被置为-1的       
            if(match_chunkid[k]==-1 || jump_pos+chunk_size > match_offset[k]){
              match_offset[k] = jump_pos;
              match_chunkid[k] = chunk_id;
              //stat[j]++;
              break;
            }
            else{
              printf("error in 324 in new_file.cu\n");
            }
          }
          jump_pos += chunk_size;        
          recalcu = 1;
          break;
        }
        else{
          np = np->next;
          if(np == NULL){
            chunk_head_value = h_file[jump_pos];
            jump_pos += 1;
            recalcu = 0;
            break;
          }
        }
      }        
    }     
    //还一种可能就是整个chunk_size*chunk_num都没有匹配
    if(match_index >= j_match_num) break;
    //printf("match_index is %d, j_match_num is %d\n",match_index, j_match_num);
  }
}

Node *lookup_ht(Node *ht, int32 rc, int *chunk_id){
  uint index = hash(rc);
  Node n = ht[index];
  if(n.chunk_id == -1){    
    return NULL;
  }
  else{
    Node *np = &n;
    for(; np != NULL; np=np->next){
      if(rc == np->checksum){
        *chunk_id = np->chunk_id;
        return np;
      }
    }
    return NULL;
  }
}


__device__ void d_get_checksum2(char *buf, int32 len, char *sum){
  MD5_CTX md5;
  MD5Init(&md5);
  MD5Update(&md5, buf, len);
  MD5Final(&md5, sum);
}

__device__ void MD5Init(MD5_CTX *context)  
{  
    context->count[0] = 0;  
    context->count[1] = 0;   
    //分别赋固定值  
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;  
    context->state[2] = 0x98BADCFE;  
    context->state[3] = 0x10325476;  
}  

__device__ void MD5Update(MD5_CTX *context, char *input,unsigned int inputlen)  
{  
    unsigned int i = 0,index = 0,partlen = 0;      
    index = (context->count[0] >> 3) & 0x3F;  
    partlen = 64 - index;  
    context->count[0] += inputlen << 3;  
    if(context->count[0] < (inputlen << 3))  
        context->count[1]++;
    context->count[1] += inputlen >> 29;  

    if(inputlen >= partlen)  {  
        memcpy(&context->buffer[index],input,partlen);        
        MD5Transform(context->state,context->buffer);  
        for(i = partlen;i+64 <= inputlen;i+=64)  
            MD5Transform(context->state,&input[i]);  
        index = 0;         
    }   
    else  i = 0;
    memcpy(&context->buffer[index],&input[i],inputlen-i);  
}

__device__ void MD5Final(MD5_CTX *context, char digest[16])  
{  
    unsigned int index = 0,padlen = 0;  
    char bits[8];  
    index = (context->count[0] >> 3) & 0x3F;  
    padlen = (index < 56)?(56-index):(120-index);  
    MD5Encode(bits,context->count,8);
    MD5Update(context,PADDING,padlen);  
    MD5Update(context,bits,8);  
    MD5Encode(digest,context->state,16);  
} 


__device__ void MD5Decode(unsigned int *output, char *input,unsigned int len)  
{  
    unsigned int i = 0,j = 0;  
    while(j < len)  
    {  
        output[i] = (input[j]) |  
            (input[j+1] << 8) |  
            (input[j+2] << 16) |  
            (input[j+3] << 24);  
        i++;  
        j+=4;   
    }  
} 

__device__ void MD5Encode(char *output,unsigned int *input,unsigned int len)  
{  
    unsigned int i = 0,j = 0;  
    while(j < len)  
    {  
        output[j] = input[i] & 0xFF;    
        output[j+1] = (input[i] >> 8) & 0xFF;  
        output[j+2] = (input[i] >> 16) & 0xFF;  
        output[j+3] = (input[i] >> 24) & 0xFF;  
        i++;  
        j+=4;  
    }  
}  

__device__ void MD5Transform(unsigned int state[4], char block[64])  
{  
    unsigned int a = state[0];  
    unsigned int b = state[1];  
    unsigned int c = state[2];  
    unsigned int d = state[3];      
    unsigned int x[16];      
    MD5Decode(x,block,64);  
    FF(a, b, c, d, x[ 0], 7, 0xd76aa478);   
    FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);   
    FF(c, d, a, b, x[ 2], 17, 0x242070db);   
    FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);       
    FF(a, b, c, d, x[ 4], 7, 0xf57c0faf);   
    FF(d, a, b, c, x[ 5], 12, 0x4787c62a);   
    FF(c, d, a, b, x[ 6], 17, 0xa8304613);   
    FF(b, c, d, a, x[ 7], 22, 0xfd469501);       
    FF(a, b, c, d, x[ 8], 7, 0x698098d8);   
    FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);   
    FF(c, d, a, b, x[10], 17, 0xffff5bb1);   
    FF(b, c, d, a, x[11], 22, 0x895cd7be);      
    FF(a, b, c, d, x[12], 7, 0x6b901122);   
    FF(d, a, b, c, x[13], 12, 0xfd987193);   
    FF(c, d, a, b, x[14], 17, 0xa679438e);   
    FF(b, c, d, a, x[15], 22, 0x49b40821);     
    GG(a, b, c, d, x[ 1], 5, 0xf61e2562);   
    GG(d, a, b, c, x[ 6], 9, 0xc040b340);   
    GG(c, d, a, b, x[11], 14, 0x265e5a51);   
    GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);       
    GG(a, b, c, d, x[ 5], 5, 0xd62f105d);   
    GG(d, a, b, c, x[10], 9,  0x2441453);   
    GG(c, d, a, b, x[15], 14, 0xd8a1e681);   
    GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);       
    GG(a, b, c, d, x[ 9], 5, 0x21e1cde6);   
    GG(d, a, b, c, x[14], 9, 0xc33707d6);   
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87);   
    GG(b, c, d, a, x[ 8], 20, 0x455a14ed);       
    GG(a, b, c, d, x[13], 5, 0xa9e3e905);   
    GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8);   
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9);   
    GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);   
    HH(a, b, c, d, x[ 5], 4, 0xfffa3942);   
    HH(d, a, b, c, x[ 8], 11, 0x8771f681);   
    HH(c, d, a, b, x[11], 16, 0x6d9d6122);   
    HH(b, c, d, a, x[14], 23, 0xfde5380c);       
    HH(a, b, c, d, x[ 1], 4, 0xa4beea44);   
    HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);   
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60);   
    HH(b, c, d, a, x[10], 23, 0xbebfbc70);       
    HH(a, b, c, d, x[13], 4, 0x289b7ec6);   
    HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);   
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085);   
    HH(b, c, d, a, x[ 6], 23,  0x4881d05);       
    HH(a, b, c, d, x[ 9], 4, 0xd9d4d039);   
    HH(d, a, b, c, x[12], 11, 0xe6db99e5);   
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8);   
    HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);   
    II(a, b, c, d, x[ 0], 6, 0xf4292244);   
    II(d, a, b, c, x[ 7], 10, 0x432aff97);   
    II(c, d, a, b, x[14], 15, 0xab9423a7);   
    II(b, c, d, a, x[ 5], 21, 0xfc93a039);       
    II(a, b, c, d, x[12], 6, 0x655b59c3);   
    II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);   
    II(c, d, a, b, x[10], 15, 0xffeff47d);   
    II(b, c, d, a, x[ 1], 21, 0x85845dd1);       
    II(a, b, c, d, x[ 8], 6, 0x6fa87e4f);   
    II(d, a, b, c, x[15], 10, 0xfe2ce6e0);   
    II(c, d, a, b, x[ 6], 15, 0xa3014314);   
    II(b, c, d, a, x[13], 21, 0x4e0811a1);      
    II(a, b, c, d, x[ 4], 6, 0xf7537e82);   
    II(d, a, b, c, x[11], 10, 0xbd3af235);   
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb);   
    II(b, c, d, a, x[ 9], 21, 0xeb86d391);      
    state[0] += a;  
    state[1] += b;  
    state[2] += c;  
    state[3] += d;  
}  

__device__ uint32 d_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2)
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
    *d_s1 = s1;
    *d_s2 = s2;
    return (s1 & 0xffff) + (s2 << 16);
}

uint32 h_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2)
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
    *d_s1 = s1;
    *d_s2 = s2;
    return (s1 & 0xffff) + (s2 << 16);
}


__device__ uint d_hash(uint32 rc){
    uint p =  1867;
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
}


__device__ Node* d_lookup_ht(Node *ht, int32 rc, int *chunk_id){ 
  uint index = d_hash(rc);
  Node n = ht[index];
  if(n.chunk_id == -1){    
    return NULL;
  }
  else{
    Node *np = &n;
    for(; np != NULL; np=np->next){
      if(rc == np->checksum){
        *chunk_id = np->chunk_id;
        return np;
      }
    }
    return NULL;
  }
}