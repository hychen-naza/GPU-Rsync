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
    //printf("in hash func\n");
    return (((rc>>16)& 0xffff ^ ((rc&0xffff) * p)) & 0xffff)%HASHSIZE;
}

bool IsSameChunk(Node *p, uint id, uint32 checksum, char md5[16], int chunkSize){
  if(p->checksum != checksum) return false;
  for(int i=0;i<16;++i){
    if(p->md5[i] != md5[i]){
      return false;
    }
  }
  //std::cout << "we find a same chunk , it is rare\n";
  p->posVec.insert(id);
  return true;
}

int insert_hashtable(HashTable *ht, uint id, uint32 checksum, char md5[16], int chunkSize)
{
  uint index = hash(checksum);
  //printf("index is %d\n",(int)index);
  uint i = 0;
  if(ht->node[index] == NULL){
    ht->node[index] = new Node();
    ht->node[index]->chunk_id = id;
    ht->node[index]->checksum = checksum;
    for(i;i<16;++i){
      ht->node[index]->md5[i] = md5[i];
    }
    ht->node[index]->posVec.insert(id);
    ht->node[index]->next = NULL;
    return 1;
  }
  else{
    Node *p = ht->node[index];    
    for(p;p!=NULL; p=p->next){
      // 先检查是不是同一个chunk
      if(IsSameChunk(p, id, checksum, md5, chunkSize)) return 1;
      // 若不是，看下它是不是最后一个，若是最后一个了，就应该给它分配node
      if(p->next == NULL){
        p->next = new Node();
        p->next->chunk_id = id;
        p->next->checksum = checksum;
        for(i=0;i<16;++i){
          p->next->md5[i] = md5[i];
        }
        p->next->posVec.insert(id);
        p->next->next = NULL;
        return 1;
      }
    }     
  }
}

Node* lookup_hashtable(HashTable *ht, uint32 rc){
    uint index;
    index = hash(rc);
    Node *np = ht->node[index];
    //因为一开始不算md5，所以只能找到index和rc就返回了
    for(np;np != NULL;np=np->next){
        if(rc == np->checksum){
          //std::cout << "yes we find a np that could be a match" << std::endl;
          return np;
        }        
    }
    return NULL;
}

//
std::set<int> md5lookup_hashtable(HashTable *ht, uint32 rc, char md5[16]){
    uint index;
    index = hash(rc);
    Node *np = ht->node[index];   
    for(np;np != NULL;np=np->next){
      bool md5match = true;
      for(int i=0;i<16;++i){
        if(np->md5[i] != md5[i]){
          md5match = false;
          break;
        }
      }
      if(md5match) return np->posVec;      
    }
    std::cout << "you shouldn't come here in ht.cc 92" << std::endl;
}









