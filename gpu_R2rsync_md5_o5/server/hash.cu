#include <set>
#include <map>
#include <iostream>
#include "hash.h"

uint hash(uint32 rc){
    uint p =  1867;
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
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

bool IsSameChunk(Node *p, uint id, uint32 checksum, uint8_t md5[8], std::vector<std::vector<int> > &matchIdVec){
  if(p->chunk_id == -1) return false;
  if(p->checksum != checksum) return false;
  if(memcmp(p->md5, md5, 8) != 0) return false;
  matchIdVec[p->chunk_id].push_back(id);
  std::cout << "we find a same chunk , it is rare\n";
  return true;
}

int insert_hashtable(Node *ht, uint id, uint32 checksum, uint8_t md5[8], std::vector<std::vector<int> > &matchIdVec)
{
  uint index = hash(checksum);
  uint i = 0;
  if(ht[index].chunk_id == -1){
    ht[index].chunk_id = id;
    ht[index].checksum = checksum;
    memcpy(ht[index].md5, md5, 8);
    ht[index].next = NULL;
    matchIdVec[id].push_back(id);
    return 1;
  }
  else{
    Node *p = &ht[index];
    for(;p != NULL; p=(p->next)){
      if(IsSameChunk(p, id, checksum, md5, matchIdVec)) return 1;
      if(p->next == NULL){
        p->next = (Node *)malloc(sizeof(Node));
        cudaMallocManaged(&(p->next),sizeof(Node));
        p->next->chunk_id = id;
        p->next->checksum = checksum;
        memcpy(p->next->md5, md5, 8);
        p->next->next = NULL;
        matchIdVec[id].push_back(id);
        return 1;   
      }
    }    
  }
  return 0;  
}











