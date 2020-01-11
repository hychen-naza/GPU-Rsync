#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
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
struct ComponentInfo
{
  int chunkid;
  int checksum1;
  char checksum2[16];
};




class RsyncServiceImpl final : public Rsync::Service {

  Status GenChecksumList(ServerContext* context, const FileHead* request,
                  FileInfo* reply) override {
    std::string rsyncFileName = request->filename();
    int chunkSize = request->chunksize();
    struct stat statbuf;
    int fd;
    if((fd = open(rsyncFileName.c_str(), O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open rsync file.\n");
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) printf("file with path %s not found\n",rsyncFileName.c_str());
    
    size_t fileSize = (size_t) statbuf.st_size;
    int chunk_num = ceil(fileSize / chunkSize); 
    int chunk_id = 0;
    char chunk_buffer[chunkSize];
    size_t read_size = 0;
    int s1 = 0, s2 = 0;
    reply->set_filename(rsyncFileName);
    while(chunk_id < chunk_num){
      read_size = read(fd, chunk_buffer, chunkSize);
      if(read_size < 0){
          printf("Error in reading");
      }
      FileChunkInfo *item = reply->add_chunkinfo();
      static int rolling_checksum;
      rolling_checksum = get_checksum1(chunk_buffer, (int32)read_size, &s1, &s2);
      static char sum2[16];
      get_checksum2(chunk_buffer, (int32)read_size, sum2);      
      item->set_chunkid(chunk_id);
      item->set_checksum1(rolling_checksum);
      item->set_checksum2(sum2);
      chunk_id ++;
    }
    return Status::OK;
  }

  Status BuildNewFile(ServerContext* context, const UnmatchChunks* request,
                  NewFileReply* reply) override {

    std::string filename = request->filename();
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


    FILE *new_file_fp;
    new_file_fp=fopen("construct.txt","w");
    for(int i=0;i<new_file_len;i++){
      fprintf(new_file_fp,"%c", new_file[i]);
      //std::cout << new_file[i] << std::endl;
    }
    fclose(new_file_fp);

    free(t.fileContent);
    free(t.matchChunkid);
    free(t.matchOffset);
    free(t.ht);
    taskMap.erase(filename);

    reply->set_success(true);
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

  float time_use=0;
  struct timeval start;
  struct timeval end;
  gettimeofday(&start,NULL);  
  
  RunServer();
  gettimeofday(&end,NULL);
  time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
  printf("files with size 512 KB cpu rsync time_use is %d us\n", (int)time_use);

  return 0;
}



