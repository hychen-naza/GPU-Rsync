#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/types.h>  
#include <unistd.h>

#include "checksum.h"
#include "hashtable.h"

#define USAGE               \
"usage:\n"                   \
"  server [options]\n"      \
"options:\n"                   \
"  -f [rsyn_file]  File to be rsyn\n"    \
"  -s [chunk_size] Chunk size of file\n" \
"  -h              Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"old_file",   required_argument,      NULL,            'o'},
  {"rsyn_file",   required_argument,      NULL,           'f'},
  {"chunk_size",  required_argument,      NULL,           's'},
  {"help",        no_argument,            NULL,           'h'},
  {NULL,            0,                    NULL,            0}
};

// 代码极度混乱。。。
void cpu_rsync(char *rsync_file_path, char* chunk_buffer, int chunk_size, HashTable *ht, char *old_file_path);


int main(int argc, char **argv){
  int chunk_size = 128;
  char *file_path = "test.txt";
  char *rsync_file_path = "new_test.txt"; 
  //char *file_path = "test.txt";
  //char *rsync_file_path = "new_test.txt"; 
  int option_char = 0;
  while ((option_char = getopt_long(argc, argv, "o:f:s:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(__LINE__);
      case 's': // chunk size
        chunk_size = atoi(optarg);
        break;   
      case 'o': // file-path
        file_path = optarg;
        break;                                          
      case 'f': // file-path
        rsync_file_path = optarg;
        break;                                          
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }
  //printf("server start\n");
  struct stat statbuf;
  int fd;
  if((fd = open(file_path, O_RDWR, 0777)) < 0){
      fprintf(stderr, "Unable to open file %s\n",file_path);
      exit(1);
  }
  if (0 > fstat(fd, &statbuf)) {
      printf("file with path %s not found\n",file_path);
  }
  size_t file_len = (size_t) statbuf.st_size;  
  uint chunk_num = (file_len / chunk_size) + 1; 
  uint chunk_id = 0;
  char chunk_buffer[chunk_size];
  size_t read_size = 0;
  int s1 = 0, s2 = 0;
  HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
  create_hashtable(ht);
  //printf("create hash table success\n");
  while(chunk_id < chunk_num){
      read_size = read(fd, chunk_buffer, chunk_size);
      if(read_size < 0){
          printf("Error in reading");
      }
      static uint32 rolling_checksum;
      rolling_checksum = get_checksum1(chunk_buffer, (int32)read_size, &s1, &s2);
      //printf("rolling_checksum is %d\n",(int)rolling_checksum);
      static char sum2[16];
      get_checksum2(chunk_buffer, (int32)read_size, sum2);
      //not know why they output using a loop
      /*for(int i=0;i<16;++i){
        printf("md5 is %02x\n", sum2[i]); 
      }*/
      if((insert_hashtable(ht, chunk_id, rolling_checksum, sum2))==1){
        //printf("insert success\n");
      }
      else{
        printf("insert failed\n");
      }
      chunk_id ++;
  }
  close(fd);

  //open the new file to rsync
 
  cpu_rsync(rsync_file_path, chunk_buffer, chunk_size, ht, file_path);

  return 0;
}



void cpu_rsync(char *rsync_file_path, char* chunk_buffer, int chunk_size, HashTable *ht, char *old_file_path){

  // i did a little tests, and find that memory allocation unmatch_value .. isn't time consuming, 
  // have no big effect on total time usage
  float time_use=0;
  struct timeval start;
  struct timeval end;
  gettimeofday(&start,NULL);  

  struct stat statbuf;
  int fd;
  if((fd = open(rsync_file_path, O_RDWR, 0777)) < 0){
      fprintf(stderr, "Unable to open rsync file.\n");
      exit(1);
  }
  if (0 > fstat(fd, &statbuf)) {
      printf("file with path %s not found\n",rsync_file_path);
  }

  size_t file_len = (size_t) statbuf.st_size;

  int chunk_num = (file_len / chunk_size) + 1; 
  // cpu_rsync is only used for small size file rsync
  int match_chunkid[chunk_num];
  int match_offset[chunk_num];
  int match_num = 0, unmatch_num = 0;
  char unmatch_value[file_len];
  int unmatch_offset[file_len];
  int chunk_id;
  long int offset = 0;
  uint32 rolling_checksum, s1, s2;
  int move_chunk = 0; // 0 means we move forward 1 byte, while 1 means we move chunk size
  char chunk_head_value;
  int recalcu = 1;
  FILE *rc_fp=fopen("testfilerc.txt","w");
  while(offset < file_len){
      // 文件读取这里不应该每次都重新读一遍，效率好低
      if((lseek(fd, offset, SEEK_SET))==-1){
          printf("error in file seek\n");}
      int read_size = read(fd, chunk_buffer, chunk_size);
      if(read_size < 0){
          printf("Error in reading\n");
      }

      if(recalcu==1){
          rolling_checksum = get_checksum1(chunk_buffer, (int32)read_size, &s1, &s2);
      }
      else if(recalcu==0){
          s1 -= chunk_head_value + CHAR_OFFSET; 
          s2 -= chunk_size * (chunk_head_value + CHAR_OFFSET);
          s1 += chunk_buffer[read_size-1] + CHAR_OFFSET;
          s2 += s1;
          rolling_checksum = (s1 & 0xffff) + (s2 << 16);
      }
      //fprintf(rc_fp, "offset %d rc value %d\n", offset, rolling_checksum);
      // chunk_head_value keep the first char of last chunk
      chunk_head_value = chunk_buffer[0];
      Node *np = lookup_hashtable(ht, rolling_checksum);
      //not pass the first check, almost failed in this match test, showing hash func works well
      if(np == NULL){
          //printf("not find in hash table\n");
          offset += 1; 
          move_chunk = 0;
          recalcu = 0;
          goto RecordInfo;;
      }
      else{
          //not pass the second check
          if(np->checksum != rolling_checksum){
              //printf("checksum not match\n");
              offset += 1; 
              move_chunk = 0;
              recalcu = 0;
              goto RecordInfo;;
          }
          else{
              static char sum2[16];
              get_checksum2(chunk_buffer, (int32)read_size, sum2);
              for(int i=0;i<16;++i){
                  //not pass the third check
                  if(sum2[i]!=np->md5[i]){ 
                      //printf("md5 not match\n");
                      offset += 1;
                      move_chunk = 0;
                      recalcu = 0;
                      goto RecordInfo;
                  }
              }
              chunk_id = (int)np->chunk_id;
              offset += read_size;
              move_chunk = 1;
              recalcu = 1;
              goto RecordInfo;
          }
      }
RecordInfo:      
      if(move_chunk==0){
        unmatch_value[unmatch_num] = chunk_head_value;
        unmatch_offset[unmatch_num] = offset-1;
        unmatch_num ++;
        /*printf("\nfind an unmatch\n");
        printf("offset is %d\n",(int)offset);
        printf("unmatch value %c\n",chunk_head_value);*/
      }
      else if(move_chunk==1){
        match_chunkid[match_num] = chunk_id;
        match_offset[match_num] = offset-chunk_size;
        match_num ++;
        /*printf("\nfind a match\n");
        printf("offset is %d\n",(int)offset);
        printf("match chunk id %d\n",(int)np->chunk_id);*/
      }   
  }

  gettimeofday(&end,NULL);
  time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
  printf("cpu rsync computation time_use is %d us\n",(int)time_use);
  int total_send_bytes = unmatch_num + 4*unmatch_num + 2*4*match_num;
  printf("unmatch num is %d, match num is %d, total send bytes is %d\n", unmatch_num, match_num, total_send_bytes);

  //construct new file
  // only 400us
  gettimeofday(&start,NULL);  
  char new_file[file_len];
  int old_fd = open(old_file_path, O_RDWR, 0777);

  for(int i = 0; i < unmatch_num; ++i){
    int offset = unmatch_offset[i];
    new_file[offset] = unmatch_value[i];
  }
  for(int i = 0;i<match_num;++i){
    lseek(old_fd, match_chunkid[i]*chunk_size, SEEK_SET);
    int read_size = read(old_fd, chunk_buffer, chunk_size);
    memcpy(&new_file[match_offset[i]],chunk_buffer,read_size);
  }
  FILE *fp;
  fp=fopen("construct.txt","w");
  for(int i=0;i<file_len;i++){
    fprintf(fp,"%c", new_file[i]);
  }
  gettimeofday(&end,NULL);
  time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//微秒
  printf("cpu rsync construct new file time_use is %d us\n",(int)time_use);
  // make sure new construct.txt is the same as the rsync file
  // system("diff construct.txt ..");
}







