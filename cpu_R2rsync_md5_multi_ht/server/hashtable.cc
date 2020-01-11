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

uint hash2(uint32 x){
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return 1+x%(HASHSIZE-1);
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
  uint index2 = hash2(checksum);
  uint index3;
  for(int j=0;;++j){
    index3 = (index + j*index2)%HASHSIZE;
    if(ht->node[index3] == NULL){
      ht->node[index3] = new Node();
      ht->node[index3]->chunk_id = id;
      ht->node[index3]->checksum = checksum;
      for(int i=0;i<16;++i){
        ht->node[index3]->md5[i] = md5[i];
      }
      ht->node[index3]->posVec.insert(id);
      return 1;
    }
    else{
      if(IsSameChunk(ht->node[index3], id, checksum, md5, chunkSize)) return 1;
    }
  }
}

Node* lookup_hashtable(HashTable *ht, uint32 rc, uint &first_index, uint &second_index, uint &jump_time){
  uint index = hash(rc);
  uint index2 = hash2(rc);
  first_index = index;
  second_index = index2;
  uint index3;
  //这里是不一定能找到
  for(int i=0;;++i){
    index3 = (index+i*index2)%HASHSIZE;
    if(ht->node[index3] == NULL || rc == ht->node[index3]->checksum){
      jump_time = i;
      return ht->node[index3];
    }
  }
}

//
std::set<int> md5lookup_hashtable(HashTable *ht, uint32 rc, char md5[16]){
  uint index, index2, index3;
  index = hash(rc);
  index2 = hash2(rc);
  Node *np;
  //这里是肯定能找到的
  for(int i=0;;++i){
    index3 = (index+i*index2)%HASHSIZE;
    np = ht->node[index3];
    if(np==NULL){
      printf("error in hash.cc 87\n");
      continue;
    }
    if(rc != np->checksum){
      continue;
    }
    bool md5match = true;
    for(int j=0;j<16;++j){
      if(np->md5[j] != md5[j]){
        md5match = false;
        break;
      }
    }
    if(md5match) return np->posVec; 
  }
}









