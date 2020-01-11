#include "type.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#ifndef H_HASH_H
#define H_HASH_H
#define HASHSIZE 4096
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
  char *newFile;
  int *matchChunkid;
  int *matchOffset;
  Node *ht;
  int *stat;

  char *dFileContent;
  char *dNewFile;
  int *dMatchChunkid;
  int *dMatchOffset;
  Node *dHt;
  int *dStat;

  int fileLen;
  int newFileLen;
  int chunkSize;
  int totalChunkNum;
  int totalThreads;
  int roundNum;
  int fd;
};

struct buildTask
{

  int newFileLen;
  int chunkSize;
  int totalThreads;
  int roundNum;
  int fd;
  
  char *dFileContent;
  char *dNewFile;
  char *file;
  char *newFile;
  int *stat;
  int *matchChunkid;
  int *matchOffset;

  int *matchIdArray;
  std::map<int, std::set<int> > matchIdMap;

  int totalOldChunkNum;
  int totalNewChunkNum; 
};

uint hash(uint32 rc);
int insert_hashtable(Node *ht, uint id, uint32 checksum, char md5[16], int *matchIdArray, std::map<int, std::set<int> > &matchIdMap);
cudaStream_t * GPUWarmUp(int n_stream);

#endif
