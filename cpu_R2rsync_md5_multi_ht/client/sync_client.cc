#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>    
#include <sys/stat.h> 
#include <sys/time.h>
#include <sys/types.h>  
#include <unistd.h>
#include <cmath>
#include <memory>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "rsync.grpc.pb.h"
#include "checksum.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using rsync::FileInfo;
using rsync::FileChunkInfo;
using rsync::MatchTokenReply;
using rsync::MatchToken;
using rsync::UnmatchChunks;
using rsync::UnmatchChunkInfo;
using rsync::NewFileReply;
using rsync::Rsync;


#define USAGE               \
"usage:\n"                   \
"  server [options]\n"      \
"options:\n"                   \
"  -i [ip_addrresses]  ip_addrresses\n"    \
"  -s [chunk_size]  chunk_size\n"    \
"  -h              Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"ip_addrresses",  required_argument,   NULL,           'i'},
  {"chunk_size",  required_argument,      NULL,           's'},
  {"help",        no_argument,            NULL,           'h'},
  {NULL,            0,                    NULL,            0}
};

struct ComponentInfo
{
  int chunkid;
  int checksum1;
  char checksum2[16];
};

void CpuRsync(int fd, size_t fileSize, int chunkSize, std::vector<ComponentInfo> &vec);
int time_use;
struct timeval start;
struct timeval end;


class RsyncClient {
 public:
  explicit RsyncClient(std::shared_ptr<Channel> channel)
      : stub_(Rsync::NewStub(channel)) {}


  MatchTokenReply SendFileInfo(const std::vector<ComponentInfo>& query, const std::string fileName, const int fileSize, const int chunkSize) {
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    fileInfo.set_filesize(fileSize);
    fileInfo.set_chunksize(chunkSize);
    for (int i = 0; i < query.size(); ++i) {
      FileChunkInfo *item = fileInfo.add_chunkinfo();
      item->set_chunkid(query[i].chunkid);
      item->set_checksum1(query[i].checksum1);
      item->set_checksum2(query[i].checksum2); 
    }
    MatchTokenReply reply;
    ClientContext context;
    Status status = stub_->CalcuDiff(&context, fileInfo, &reply);

    if (status.ok()) {
      return reply;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      //return "RPC failed";
    }
  }

  void SendUnmatchChunk(int fd, int match_chunk_size, int chunk_size, std::string filename, int fileSize, MatchTokenReply &reply){
     
    int last_chunk_id = -1;
    UnmatchChunks literalBytes;
    char chunk_buffer[chunk_size];
    literalBytes.set_filename(filename);
    literalBytes.set_filesize(fileSize);
    for (int i = 0; i < match_chunk_size; i++) {
      const MatchToken item = reply.tokeninfo(i);
      const int chunkId = item.chunkid();
      while(chunkId != last_chunk_id+1){
        last_chunk_id++;
        UnmatchChunkInfo *item = literalBytes.add_chunkinfo();
        item->set_pos(last_chunk_id*chunk_size);
        lseek(fd, last_chunk_id*chunk_size, SEEK_SET);
        size_t read_size = read(fd, chunk_buffer, chunk_size);
        item->set_content(chunk_buffer);
        item->set_length(read_size);
        /*if(last_chunk_id == 2005){
          std::cout << "client send this unmatch chunk" << std::endl;
        }*/
      }
      last_chunk_id = chunkId;
    }
    int total_chunk_num = ceil(fileSize / chunk_size); 
    while(last_chunk_id < total_chunk_num){
      last_chunk_id++;
      UnmatchChunkInfo *item = literalBytes.add_chunkinfo();
      item->set_pos(last_chunk_id*chunk_size);
      lseek(fd, last_chunk_id*chunk_size, SEEK_SET);
      size_t read_size = read(fd, chunk_buffer, chunk_size);
      item->set_content(chunk_buffer);
      item->set_length(read_size);
    }

    NewFileReply build_reply;
    ClientContext context;
    Status status = stub_->BuildNewFile(&context, literalBytes, &build_reply);

    if (status.ok()) {
      std::cout << "new file build success" << std::endl;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

 private:
  std::unique_ptr<Rsync::Stub> stub_;
};


int main(int argc, char** argv) {
  FILE * rd_fp = fopen("rsync_files.txt","r");
  if(rd_fp == NULL) printf("open file failed\n");
  std::string ip_addr = "localhost";
  int chunkSize = 256;
  int option_char = 0;
  while ((option_char = getopt_long(argc, argv, "i:s:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(__LINE__);
      case 'i': // server ip
        ip_addr = optarg;
        break;  
      case 's': // chunk size
        chunkSize = atoi(optarg);
        break;                                                 
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }
  ip_addr += ":50051";
  char filePath[64];
  RsyncClient client(grpc::CreateChannel(
      ip_addr, grpc::InsecureChannelCredentials()));
  std::vector<ComponentInfo> vec;
  int handled_task_num = 0;
  while(fscanf(rd_fp, "%s", filePath) != EOF){
    if(handled_task_num == 0){
      gettimeofday(&start,NULL);
    }
    struct stat statbuf;
    int fd;
    if((fd = open(filePath, O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open file %s\n",filePath);
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) {
        printf("file with path %s not found\n",filePath);
    }
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    CpuRsync(fd, (size_t) statbuf.st_size, chunkSize, vec); 
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);//us nearly 40000us
    std::cout << "on client , single rsync file with size " << (size_t) statbuf.st_size << ", spend " << c_time_use << " us on generating checksum list" << std::endl;
    MatchTokenReply reply = client.SendFileInfo(vec, filePath, (size_t) statbuf.st_size, chunkSize);    
    std::cout << "matchTokens size is " << reply.tokeninfo_size() << std::endl;
    vec.clear();
    int size = reply.tokeninfo_size();
    client.SendUnmatchChunk(fd, size, chunkSize, filePath, (size_t) statbuf.st_size, reply);
    close(fd);
    handled_task_num++;
    if(handled_task_num == 1){
      gettimeofday(&end,NULL);
      time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
      std::cout << "single rsync file with size " << (size_t) statbuf.st_size << ", spend " << time_use << " us" << std::endl;
    }
  }
  return 0;
}



void CpuRsync(int fd, size_t fileSize, int chunkSize, std::vector<ComponentInfo> &vec){

  int chunk_num = ceil(fileSize / chunkSize); 
  int chunk_id = 0;
  char chunk_buffer[chunkSize];
  size_t read_size = 0;
  int s1 = 0, s2 = 0;
  
  while(chunk_id < chunk_num){
      read_size = read(fd, chunk_buffer, chunkSize);
      if(read_size < 0){
          printf("Error in reading");
      }
      static int rolling_checksum;
      rolling_checksum = get_checksum1(chunk_buffer, (int32)read_size, &s1, &s2);
      static char sum2[16];
      get_checksum2(chunk_buffer, (int32)read_size, sum2);      
      ComponentInfo component = {chunk_id, rolling_checksum}; 
      strncpy(component.checksum2, sum2, 16);
      vec.push_back(component);
      chunk_id ++;
  }
}