#include "type.h"
#include <vector>
#include <set>
#include <iostream>
//选一个接近4096的质数，关键是质数
#define HASHSIZE 4093	

struct Node{
    uint chunk_id;
    uint32 checksum;
    char md5[16];
    std::set<int> posVec;
};

typedef struct HashTable{
    Node *node[HASHSIZE];
    size_t node_size;
}HashTable;

void create_hashtable(HashTable *ht);
uint hash(uint32 rc);
/*return node point if match---if not find return null*/
Node *lookup_hashtable(HashTable *ht, uint32 rc, uint &first_index, uint &second_index, uint &jump_time);
/*return 1 insert success --- return 0 fail*/
int insert_hashtable(HashTable *ht, uint id, uint32 checksum, char md5[16], int chunkSize);

bool IsSameChunk(Node *p, uint id, char md5[16], int chunkSize);

std::set<int> md5lookup_hashtable(HashTable *ht, uint32 rc, char md5[16]);
