#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <getopt.h>
#include <stdint.h>
#include <vector>
#include <sys/types.h>
#include <math.h>
#include "hash.h"
#include "checksum.h"


void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads, char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j);
Node lookup_ht(Node *ht, int32 rc, int *chunk_id, uint &first_hashindex, uint &second_hashindex, uint &jump_time);
__device__ uint32 d_get_checksum1(char *buf1, int32 len, uint32 *d_s1, uint32 *d_s2);
__device__ void d_get_checksum2(const uint8_t *in, const size_t inlen, uint8_t *out);

__device__ uint d_hash(uint32 rc);
__device__ uint d_hash2(uint32 rc);
__device__ Node d_lookup_ht(Node *ht, int32 rc, int bucket_num, int c0, int c1, const int4* __restrict__ coef, int &pos, int thread_id);
__device__ bool d_char_compare(char *c1, char *c2);

__constant__ uint8_t k[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};  

inline __device__ uint d_hash(uint32 rc){
    uint p =  1867;
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
}
inline __device__ uint d_hash2(uint32 x){
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return 1+x%(HASHSIZE-1);
}
inline __device__ bool d_char_compare(uint8_t *c1, uint8_t *c2){
  if(c1[0]!=c2[0] || c1[1]!=c2[1] || c1[2]!=c2[2] || c1[3]!=c2[3]) return false;
  else if(c1[4]!=c2[4] || c1[5]!=c2[5] || c1[6]!=c2[6] || c1[7]!=c2[7]) return false;
  else return true;
}
inline bool char_compare(uint8_t *c1, uint8_t *c2){
  if(c1[0]!=c2[0] || c1[1]!=c2[1] || c1[2]!=c2[2] || c1[3]!=c2[3]) return false;
  else if(c1[4]!=c2[4] || c1[5]!=c2[5] || c1[6]!=c2[6] || c1[7]!=c2[7]) return false;
  else return true;
}
__global__ void kernel_test(Node *d_ht, int *d_pos_array, int size){
  int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
  if(thread_id==0){
    for(int i=0;i<size;++i){
      int pos = d_pos_array[i];
      Node np = d_ht[pos];
      //printf("pos %d, np rc %d\n", pos, np.checksum);
    }
  }

}

__global__ void multiwarp_match(Node *ht, char *file, size_t file_len, int total_threads, int chunk_size, int chunk_num, 
          int *match_offset, int *match_chunkid, int *stat, int bucket_num, int c0, int c1, const int4* __restrict__ coef)
{
  int thread_id = blockIdx.x * blockDim.x + threadIdx.x;
  int fileBeginPos = chunk_num*chunk_size*thread_id;
  int chunkBeginPos = chunk_num*thread_id;
  //printf("thread %d start from %d\n", thread_id, fileBeginPos);
  if(fileBeginPos < file_len){
    int recalcu = 1;
    uint32 rc;
    int match_num = 0;
    int i = 0;
    uint32 s1 = 0, s2 = 0;
    //the char in the head of a chunk, it can be used to store as the unmatch value and use to recalcu
    char chunk_head_value;
    for(; i < chunk_size*chunk_num;){
      //剩下的内容已经不够一个chunk_size
      if(fileBeginPos+i>file_len-chunk_size){
        break;
      }
      if(recalcu == 1) rc = d_get_checksum1(&file[fileBeginPos + i], chunk_size, &s1, &s2);
      else if(recalcu == 0){
        s1 -= chunk_head_value + CHAR_OFFSET; 
        s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
        s1 += file[fileBeginPos+i+chunk_size-1] + CHAR_OFFSET;
        s2 += s1;
        rc = (s1 & 0xffff) + (s2 << 16);
      }
      
      chunk_head_value = file[fileBeginPos+i];
      int pos = 0;
      //printf("before hash look up, in thread %d, pos %d, the first chunk rc %d, ht pos is %d\n", thread_id, fileBeginPos + i, (int)rc, pos);
      Node np = d_lookup_ht(ht, rc, bucket_num, c0, c1, coef, pos, thread_id);
      //printf("after hash look up, in thread %d, pos %d, the first chunk rc %d, ht pos is %d\n", thread_id, fileBeginPos + i, (int)rc, pos);
      
      //recalcu = 0;
      //i += 1;
      if(np.checksum == rc){
        uint8_t sum2[8];
        d_get_checksum2((uint8_t*)&file[fileBeginPos+i], (size_t)chunk_size, (uint8_t*)sum2);
        //printf("First, thread %d have match rc\n", thread_id); 
        if(d_char_compare(sum2,np.md5)){  
          //printf("Ha, thread %d have match md5 at file pos %d\n", thread_id, fileBeginPos + i);          
          match_chunkid[chunkBeginPos + match_num] = np.chunk_id;
          match_offset[chunkBeginPos + match_num] = fileBeginPos + i;         
          match_num ++;
          recalcu = 1;
          i += chunk_size;
        }
        else{
          //printf("Ai, thread %d don't have match md5 at file pos %d\n", thread_id, fileBeginPos + i);       
          recalcu = 0;
          i += 1;
        }
      }
      else{
        /*if(thread_id==1){
          printf("Emmm, thread %d don't match at file pos %d, node rc %d, calcu rc %d\n", thread_id, fileBeginPos + i, np.checksum, rc); 
        }*/
        //printf("Emmm, thread %d don't match at file pos %d, node rc %d, calcu rc %d\n", thread_id, fileBeginPos + i, np.checksum, rc); 
        recalcu = 0;
        i += 1;
      }
    }
    stat[thread_id] = match_num;
  }
}

__global__ void gpu_recalcu(Node *ht, char *file, int chunk_size, int chunk_num, int *match_offset, int *match_chunkid,
             int *stat, int region_size, int bucket_num, int c0, int c1, const int4* __restrict__ coef)
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
      //if(i<10) printf("in gpu recalcu thread %d need recalcu, its match num %d, jump pos %d\n", i, t_match_num, jump_pos);         
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
        int pos;
        Node np = d_lookup_ht(ht, rc, bucket_num, c0, c1, coef, pos, thread_id);
        if(np.checksum == rc){
          uint8_t sum2[8];
          d_get_checksum2((uint8_t*)&file[jump_pos], (size_t)chunk_size, (uint8_t*)sum2);
          if(d_char_compare(sum2,np.md5)){  
            for(int k=j_match_begin;k<j_match_begin+chunk_num;++k){         
              if(match_chunkid[k]==-1 || jump_pos+chunk_size > match_offset[k]){
                match_offset[k] = jump_pos;
                match_chunkid[k] = chunk_id;
                stat[j]++;
                break;
              }
            }
            recalcu = 1;
            jump_pos += chunk_size;
          }
          else{
            chunk_head_value = file[jump_pos];
            recalcu = 0;
            jump_pos += 1;
          }
        }
        else{
          chunk_head_value = file[jump_pos];
          recalcu = 0;
          jump_pos += 1;
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
    if(recalcu == 1) rc = get_checksum1(&h_file[jump_pos], length, (int*)&s1, (int*)&s2);   
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
    
    uint first_index, second_index, jump_time;
    Node np = lookup_ht(ht, rc, &chunk_id, first_index, second_index, jump_time);
    if(np.chunk_id == -1){
      recalcu = 0;
      jump_pos += 1;
    }
    else{
      uint8_t sum2[8];
      get_checksum2((uint8_t*)&h_file[jump_pos], (size_t)length, (uint8_t*)sum2); 
      uint index3;
      for(int j = jump_time;;++j){   
        index3 = (first_index+j*second_index)%HASHSIZE;
        np = ht[index3];
        if(np.chunk_id == -1){
          recalcu = 0;
          chunk_head_value = h_file[jump_pos];
          jump_pos += 1;
          break;
        }
        if(char_compare(sum2,np.md5)){
          for(int k=j_match_begin;k<j_match_begin+chunk_num*recalcu_region_size;++k){   
            //已经被置为-1或者目前还没有但马上会被置为-1的       
            if(match_chunkid[k]==-1 || jump_pos+chunk_size > match_offset[k]){
              match_offset[k] = jump_pos;
              match_chunkid[k] = chunk_id;
              //stat[j]++;
              break;
            }
          }
          recalcu = 1;
          jump_pos += chunk_size;
          //printf("we have match in thread %d in gpu\n",thread_id);
          break;
        }      
      }  
    }     
    //还一种可能就是整个chunk_size*chunk_num都没有匹配
    if(match_index >= j_match_num) break;
    //printf("match_index is %d, j_match_num is %d\n",match_index, j_match_num);
  }
}

Node lookup_ht(Node *ht, int32 rc, int *chunk_id, uint &first_index, uint &second_index, uint &jump_time){ 
  uint index = hash(rc);
  uint index2 = hash2(rc);
  first_index = index;
  second_index = index2;
  uint index3;
  //这里是不一定能找到
  for(int i=0;;++i){
    index3 = (index+i*index2)%HASHSIZE;
    if(ht[index3].chunk_id == -1 || rc == ht[index3].checksum){
      jump_time = i;
      return ht[index3];
    }
  }
}


__device__ void d_get_checksum2(const uint8_t *in, const size_t inlen, uint8_t *out){
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    //uint64_t k0 = 50462976;
    //uint64_t k1 = 185207048;
    uint64_t k0 = U8TO64_LE(k);
    uint64_t k1 = U8TO64_LE(k + 8);
    uint64_t m;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8) {
        m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    switch (left) {
    case 7:
        b |= ((uint64_t)in[6]) << 48;
    case 6:
        b |= ((uint64_t)in[5]) << 40;
    case 5:
        b |= ((uint64_t)in[4]) << 32;
    case 4:
        b |= ((uint64_t)in[3]) << 24;
    case 3:
        b |= ((uint64_t)in[2]) << 16;
    case 2:
        b |= ((uint64_t)in[1]) << 8;
    case 1:
        b |= ((uint64_t)in[0]);
        break;
    case 0:
        break;
    }
    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);
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


__device__ Node d_lookup_ht(Node *ht, int32 rc, int bucket_num, int c0, int c1, const int4* __restrict__ coef, int &pos, int thread_id){ 
  unsigned long long int step00 = c0 + c1*abs(rc);
  int bucket_id = ((step00)%1900813+1900813)%bucket_num;
  int4 i4 = coef[bucket_id];
  unsigned long long int step01 = (i4.y+i4.z*(abs(rc)));
  int node_pos = (step01%1900813+1900813)%i4.x + i4.w;
  //printf("GPU bucket id %d, rc %d, step0_pos %lld, step1_pos %d, node pos %d\n", bucket_id, rc, step0_pos, step1_pos, node_pos);
  if(thread_id == 0){
    printf("GPU first roudn rc %d, c0 %d, c1 %d, bucket_num %d, step00 %lld, bucket id %d\n", rc, c0, c1, bucket_num, step00, bucket_id);
  }
  //printf("GPU bucket id %d, rc %d, step01_pos %lld, node pos %d\n", bucket_id, rc, step01, node_pos);
  pos = node_pos;
  //printf("GPU node pos %d checksum %d\n",node_pos, ht[node_pos].checksum);
  return ht[node_pos];
}
