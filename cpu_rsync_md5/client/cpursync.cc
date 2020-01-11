

int CpuRsync(HashTable *ht, char *file, int *match_chunkid, int *match_offset, int *unmatch_value, int *unmatch_offset, int file_len, int chunk_size
            int &match_num, int &unmatch_num){
  match_num = unmatch_num = 0;
  int chunk_id;
  long int offset = 0;
  uint32 rolling_checksum;
  int s1, s2;
  int move_chunk = 0; // 0 means we move forward 1 byte, while 1 means we move chunk size
  char chunk_head_value;
  int recalcu = 1;
  int length = chunk_size;
  while(offset < file_len){
    if(offset > file_len-chunk_size){
        length = file_len-offset;
    }
    if(recalcu==1){
        rolling_checksum = get_checksum1(&file[offset], length, &s1, &s2);
        /*std::cout << "content is ";
        for(int i=0;i<length;++i){
          std::cout << file[offset+i];
        }
        std::cout << std::endl;*/
    }
    else if(recalcu==0){
        s1 -= chunk_head_value + CHAR_OFFSET; 
        s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
        s1 += file[offset+length-1] + CHAR_OFFSET;
        s2 += s1;
        rolling_checksum = (s1 & 0xffff) + (s2 << 16);
    }
    // chunk_head_value keep the first char of last chunk
    chunk_head_value = file[offset];
    Node *np = lookup_hashtable(ht, rolling_checksum);
    // not pass the first check, almost failed in this match test, showing hash func works well
    // 可能是index不对，也可能是Indexy一样但rc不一样
    if(np == NULL){
      //std::cout << "node point is null" << std::endl;
      move_chunk = 0;
      recalcu = 0;
    }
    // rc也相等，但md5还不知道等不等
    else{        
      while(1){       
        static char sum2[16];
        get_checksum2(&file[offset], length, sum2);  
        bool md5match = true;        
        for(int i=0;i<16;++i){
            if(sum2[i]!=np->md5[i]){                  
                md5match = false;
                break;
            }
        }
        if(md5match){
          chunk_id = (int)np->chunk_id;
          move_chunk = 1;
          recalcu = 1;
          break;
        }
        else{
          np = np->next;
          if(np == NULL){
            move_chunk = 0;
            recalcu = 0;
            break;
          }
        }
      }          
    }     
    if(move_chunk==0){
      unmatch_value[unmatch_num] = chunk_head_value;
      unmatch_offset[unmatch_num] = offset;
      unmatch_num ++;
      offset += 1; 
    }
    else if(move_chunk==1){
      match_chunkid[match_num] = chunk_id;
      match_offset[match_num] = offset;
      match_num ++;
      offset += chunk_size; 
    }   
    else{
      printf("you shouldn't come here\n");
    }
  }
  return match_num;
}