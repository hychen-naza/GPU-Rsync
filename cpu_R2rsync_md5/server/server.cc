#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <fstream>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <pthread.h>
#include <vector>
#include <map>
#include <algorithm>
#include <set>
#include <sys/types.h>  
#include <unistd.h>
#include <pthread.h>
#include <grpcpp/grpcpp.h>
#include "rsync.grpc.pb.h"
#include "checksum.h"
#include "hashtable.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using rsync::FileInfo;
using rsync::FileChunkInfo;
using rsync::MatchTokenReply;
using rsync::MatchToken;
using rsync::UnmatchChunks;
using rsync::UnmatchChunkInfo;
using rsync::NewFileReply;
using rsync::Rsync;

struct Task
{
  std::string fileName;
  int fd;
  HashTable *ht;
  char *fileContent;
  int *matchChunkid;
  int *matchOffset;
  int chunkSize;
  int matchNum;
  std::map<int, std::set<int> > matchIdMap;
};

std::map<std::string, Task> taskMap;
std::atomic<int> handled_task_num{0};
int total_task_num = 32;
int time_use;
struct timeval start;
struct timeval end;

int CpuRsync(HashTable *ht, char *file, int *match_chunkid, int *match_offset, int file_len, int chunk_size){
  int match_num = 0, unmatch_num = 0;
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
      static char sum2[16];
      get_checksum2(&file[offset], length, sum2);        
      while(1){     
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


class RsyncServiceImpl final : public Rsync::Service {

  Status CalcuDiff(ServerContext* context, const FileInfo* request,
                  MatchTokenReply* reply) override {
    handled_task_num++;
    int cur_num = handled_task_num;
    if(handled_task_num == 1){
      gettimeofday(&start,NULL);
    } 
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    std::string rsyncFileName = request->filename();
    int chunk_size = request->chunksize();
    int new_file_len = request->filesize();
    struct stat statbuf;
    int fd;
    if((fd = open(rsyncFileName.c_str(), O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open rsync file.\n");
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) printf("file with path %s not found\n",rsyncFileName.c_str());
    size_t old_file_len = (size_t) statbuf.st_size;
    char *file = (char *)malloc(sizeof(char)*old_file_len);
    size_t read_size = 0;
    while(read_size < old_file_len){
        read_size += read(fd, &(file[read_size]), chunk_size);
    }
    int chunk_num = (new_file_len / chunk_size) + 1; 
    int *match_chunkid = (int *)malloc(sizeof(int)*chunk_num);
    int *match_offset = (int *)malloc(sizeof(int)*chunk_num);
    memset(match_chunkid, -1, chunk_num);
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    create_hashtable(ht);
    int size = request->chunkinfo_size();
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const int checksum1 = item.checksum1();
      char checksum2[16];
      strncpy(checksum2, item.checksum2().c_str(), 16);
      if((insert_hashtable(ht, chunkId, checksum1, checksum2, chunk_size))==1){} 
      else printf("insert failed\n");
    }
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us receive package from client" << std::endl;


    gettimeofday(&c_start,NULL);
    int match_num = CpuRsync(ht, file, match_chunkid, match_offset, old_file_len, chunk_size);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us on cpu rsync computation" << std::endl;
    std::set<int> matchTokens;
    std::map<int, std::set<int> > matchIdMap; // one id and its set
    for(int i=0;i<match_num;++i){
      int id = match_chunkid[i];      
      const FileChunkInfo item = request->chunkinfo(id);
      int checksum1 = item.checksum1();
      char checksum2[16];
      strncpy(checksum2, item.checksum2().c_str(), 16);
      std::set<int> idset = md5lookup_hashtable(ht, checksum1, checksum2);
      for(auto it = idset.begin(); it != idset.end(); ++it){
        matchTokens.insert(*it);
      }
      matchIdMap.insert(std::make_pair(id, idset));
    }
    Task t = {rsyncFileName, fd, ht, file, match_chunkid, match_offset, chunk_size, match_num, matchIdMap};
    taskMap.insert(std::make_pair(rsyncFileName,t));
    reply->set_filename(rsyncFileName);
    for (auto it = matchTokens.begin(); it != matchTokens.end(); ++it) {
      MatchToken *item = reply->add_tokeninfo();
      item->set_chunkid(*it);
    }
    std::cout << "total match num is " << match_num << " chunk " << std::endl;
    std::cout << "however, we only need to send " << reply->tokeninfo_size() << " chunk " << std::endl;
    if(cur_num == total_task_num){
      gettimeofday(&end,NULL);
      time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
      std::cout << total_task_num << " rsync file with size " << new_file_len << ", spend " << time_use << " us on CalcuDiff" << std::endl;
    }
    return Status::OK;
  }

  Status BuildNewFile(ServerContext* context, const UnmatchChunks* request,
                  NewFileReply* reply) override {
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    std::string filename = request->filename();
    pthread_t tid = pthread_self();
    std::cout << "build file " << filename << " in thread " << tid << std::endl;
    Task t = taskMap[filename];
    int new_file_len = request->filesize();
    char *new_file = (char *)malloc(sizeof(char)*new_file_len);
    memset(new_file, 0, new_file_len);
    int size = request->chunkinfo_size();
    for (int i = 0; i < size; i++) {
      const UnmatchChunkInfo item = request->chunkinfo(i);
      const int pos = item.pos();    
      memcpy(&new_file[pos], item.content().c_str(), item.length());
    }

    char chunk_buffer[t.chunkSize];
    size_t read_size = 0;    
    for(int i=0;i<t.matchNum;++i){
      int old_file_pos = t.matchOffset[i];  
      std::set<int> idset = t.matchIdMap[t.matchChunkid[i]];
      for(auto id : idset){
        //std::cout << id << std::endl;
        int new_file_pos =  id * t.chunkSize;
        lseek(t.fd, old_file_pos, SEEK_SET);
        read_size = read(t.fd, chunk_buffer, t.chunkSize);
        memcpy(&new_file[new_file_pos], chunk_buffer, read_size);
      }
    }

    std::ofstream outf;
    outf.open("construct.txt");
    outf.write(new_file, new_file_len);
    outf.close();

    free(t.fileContent);
    free(t.matchChunkid);
    free(t.matchOffset);
    free(t.ht);
    taskMap.erase(filename);

    reply->set_success(true);
    reply->set_filename(filename);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);//us nearly 40000us
    //std::cout << "single rsync file with size " << new_file_len << ", spend " << c_time_use << " us on build file" << std::endl;
    return Status::OK;
  }
};


void RunServer() {
  std::string server_address("0.0.0.0:50051");
  RsyncServiceImpl service;
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}


int main(int argc, char **argv){
  RunServer();
  return 0;
}



