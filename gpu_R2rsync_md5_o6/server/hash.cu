#include <set>
#include <map>
#include <iostream>
#include "hash.h"

uint hash(uint32 rc){
    uint p =  1867;
    //printf("in hash func\n");
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
}

uint hash2(uint32 x){
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return 1+x%(HASHSIZE-1);
}

cudaStream_t * GPUWarmUp(int n_stream)
{
  Node *p;
  cudaMallocManaged(&p,sizeof(Node));
  int dev = 0;
  cudaSetDevice(dev);
  cudaStream_t *stream =(cudaStream_t*)malloc(n_stream*sizeof(cudaStream_t));
  for(int i=0;i<n_stream;i++)
  {
    cudaStreamCreate(&stream[i]);
  }
  return stream;
}

bool IsSameChunk(Node p, uint id, uint32 checksum, uint8_t md5[8], std::vector<std::vector<int> > &matchIdVec){
  if(p.chunk_id == -1) return false;
  if(p.checksum != checksum) return false;
  if(memcmp(p.md5, md5, 8) != 0) return false;
  matchIdVec[p.chunk_id].push_back(id);
  std::cout << "we find a same chunk , it is rare\n";
  return true;
}

int insert_hashtable(Node *ht, uint id, uint32 checksum, uint8_t md5[8], std::vector<std::vector<int> > &matchIdVec)
{
  uint index = hash(checksum);
  uint index2 = hash2(checksum);
  uint index3;
  for(int j=0;;++j){
    index3 = (index + j*index2)%HASHSIZE;
    if(ht[index3].chunk_id == -1){
      ht[index3].chunk_id = id;
      ht[index3].checksum = checksum;
      
      memcpy(ht[index3].md5, md5, 8);
      if(id == 46 || id == 104 || id == 115){
        printf("id %d, jump %d times, and index is %d\n",id, j,index3);
        for(int k=0;k<8;++k){
          printf("id %d, chunk md5 %d, ht md5 %d\n", id, md5[k], ht[index3].md5[k]);
        }
      }
      matchIdVec[id].push_back(id);
      return 1;
    }
    else{
      if(IsSameChunk(ht[index3], id, checksum, md5, matchIdVec)) return 1;
    }
  }
}











