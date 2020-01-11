#include "type.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#ifndef H_HASH_H
#define H_HASH_H
#define HASHSIZE 9999
#define CHAR_OFFSET 0
typedef struct Node{
    int chunk_id;
    uint32 checksum;
    char md5[16];
    struct Node *next;
}Node;


struct Task
{
  std::string fileName;

  char *file; // old file length
  int *stat;
  int *matchChunkid;
  int *matchOffset;

  char *dFileContent;
  int *dMatchChunkid;
  int *dMatchOffset;
  Node *dHt;
  int *dStat;

  int fileLen;
  int chunkSize;
  int totalChunkNum;
  int totalThreads;
  int roundNum;

  Node *ht;

  std::vector<std::vector<int> > matchIdVec;
  int fd;
};

struct buildTask
{
  std::string fileName;
  int chunkSize;
  int totalThreads;
  int fd;
  
  char *file;
  int *stat;
  int *matchChunkid;
  int *matchOffset;

  std::vector<std::vector<int> > matchIdVec;
};

uint hash(uint32 rc);
int insert_hashtable(Node *ht, uint id, uint32 checksum, char md5[16], std::vector<std::vector<int> > &matchIdVec);
cudaStream_t * GPUWarmUp(int n_stream);

#endif
