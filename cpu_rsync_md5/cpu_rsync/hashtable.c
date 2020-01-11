#include "hashtable.h"

void create_hashtable(HashTable *ht)
{
  ht->node_size = sizeof(Node);
  int i = 0;
  for(i;i<HASHSIZE;++i){
    ht->node[i] = NULL;
  }
}

uint hash(uint32 rc){
    uint p =  1867;
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
}

int insert_hashtable(HashTable *ht, uint id, uint32 checksum, char md5[16])
{
  uint index = hash(checksum);
  //printf("index is %d\n",(int)index);
  uint i = 0;
  if(ht->node[index] == NULL){
    ht->node[index] = (Node *)malloc(sizeof(Node));
    ht->node[index]->chunk_id = id;
    ht->node[index]->checksum = checksum;
    for(i;i<16;++i){
      ht->node[index]->md5[i] = md5[i];
    }
    ht->node[index]->next = NULL;
    return 1;
  }
  else{
    Node *p = ht->node[index];
    for(p;p!=NULL; p=p->next){
      if(p->next == NULL){
        p->next = (Node *)malloc(sizeof(Node));
        p->next->chunk_id = id;
        p->next->checksum = checksum;
        for(i;i<16;++i){
          p->next->md5[i] = md5[i];
        }
        p->next->next = NULL;
      }
      //otherwise it will keep growing
      break;
    } 
    return 1;   
  }
  return 0;
}

Node* lookup_hashtable(HashTable *ht, uint32 rc){
    uint index;
    index = hash(rc);
    Node *np = ht->node[index];
    for(np;np != NULL;np=np->next){
        if(rc == np->checksum)
            return np;
    }
    return NULL;
}









