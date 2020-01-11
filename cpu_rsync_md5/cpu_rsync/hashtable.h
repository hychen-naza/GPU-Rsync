#include "type.h"
#define HASHSIZE 1024

typedef struct Node{
    uint chunk_id;
    uint32 checksum;
    char md5[16];
    struct Node *next;
}Node;

typedef struct HashTable{
    Node *node[HASHSIZE];
    size_t node_size;
}HashTable;

void create_hashtable(HashTable *ht);
uint hash(uint32 rc);
/*return node point if match---if not find return null*/
Node* lookup_hashtable(HashTable *ht, uint32 rc);
/*return 1 insert success --- return 0 fail*/
int insert_hashtable(HashTable *ht, uint id, uint32 checksum, char md5[16]);
