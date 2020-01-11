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


#define UNMATCH_SIZE 99999

void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads, char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j);
uint32 h_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2);
int lookup_ht(Node *ht, int32 rc, int *chunk_id);
__device__ uint32 d_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2);
__device__ uint d_hash(uint32 rc);
__device__ int d_lookup_ht(Node *ht, int32 rc, int *chunk_id);

__device__ int *test1(int *chunkid, int pos){
  chunkid[pos] += 1;
  return &chunkid[pos];
}
__global__ void test(int *chunkid){
  for(int i=0;i<10;++i){
    test1(chunkid, i);
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
    int match_num = 0, move;
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
      move = d_lookup_ht(ht, rc, &chunk_id);

      if(move == 1){
        match_chunkid[chunkBeginPos + match_num] = chunk_id;
        match_offset[chunkBeginPos + match_num] = fileBeginPos + i;
        match_num ++;
        recalcu = 1;
        i += chunk_size;
      }
      else if(move == 0){
        recalcu = 0;
        i += 1;
      }
    }
    //record match_num
    stat[thread_id] = match_num;
  }
}

void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads,
            char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j){
  int match_index = 0;
  int recalcu = 1;
  int chunk_id;
  int length = chunk_size;
  int j_match_num = stat[j];
  int j_match_begin = chunk_num*j;
  char chunk_head_value;
  uint32 s1, s2, rc;
  while(1){
    if(jump_pos>file_len-chunk_size) length = file_len-chunk_size;
    if(recalcu == 1) rc = h_get_checksum1(&h_file[jump_pos], length, &s1, &s2);   
    else if(recalcu == 0){
      s1 -= chunk_head_value + CHAR_OFFSET; 
      s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
      s1 += h_file[jump_pos+length-1] + CHAR_OFFSET;
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

    // match到直接跳出来
    // 没有match的话，只有当jump_pos小于unmatch_offset和match_offset时，我们才确认是没有match到，才写入文件算下一个rc
    // fprintf(cross_fp, "offset %d rc value %d\n", jump_pos, rc);    
    int move = lookup_ht(ht, rc, &chunk_id);
    if(move == 1) {
      for(int k=j_match_begin;k<j_match_begin+chunk_num;++k){   
        //已经被置为-1或者目前还没有但马上会被置为-1的       
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
    }  
    else if (move == 0){         
      chunk_head_value = h_file[jump_pos];
      jump_pos += 1;
      recalcu = 0;
    } 
    //还一种可能就是整个chunk_size*chunk_num都没有匹配
    if(match_index >= j_match_num) break;
  }
}




int lookup_ht(Node *ht, int32 rc, int *chunk_id){
//if match return 1, else return 0  
  uint index = hash(rc);
  Node n = ht[index];
  if(n.chunk_id == -1){    
    return 0;
  }
  else{
    if(n.checksum == rc){
      *chunk_id = n.chunk_id;
      return 1;
    }
    else{
      Node *np = n.next;
      for(;np!=NULL;np=np->next){        
        if(rc == np->checksum){
          *chunk_id = n.chunk_id;
          return 1;
        }
      }
    }
    return 0;
  }
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


__device__ int d_lookup_ht(Node *ht, int32 rc, int *chunk_id){
//if match return 1, else return 0  
  uint index = d_hash(rc);
  Node n = ht[index];
  if(n.chunk_id == -1){    
    return 0;
  }
  else{
    if(n.checksum == rc){
      *chunk_id = n.chunk_id;
      return 1;
    }
    else{
      Node *np = n.next;
      for(;np!=NULL;np=np->next){        
        if(rc == np->checksum){
          *chunk_id = n.chunk_id;
          return 1;
        }
      }
    }
    return 0;
  }
}