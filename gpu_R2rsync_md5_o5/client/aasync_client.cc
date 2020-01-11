#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>    
#include <sys/stat.h> 
#include <sys/types.h>  
#include <sys/time.h>
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
using grpc::ClientAsyncResponseReader;
using grpc::CompletionQueue;
using grpc::ClientContext;
using grpc::Status;
using rsync::FileHead;
using rsync::FileInfo;
using rsync::FileChunkInfo;
using rsync::MatchTokenReply;
using rsync::MatchToken;
using rsync::UnmatchChunks;
using rsync::UnmatchChunkInfo;
using rsync::RsyncReply;
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

int handled_task_num = 0;
int time_use;
struct timeval start;
struct timeval end;

struct ComponentInfo
{
  int chunkid;
  int checksum1;
  uint8_t checksum2[8];
};
struct BuildTask{
  int fd;
  int chunkSize;
  std::string filePath;
  int fileLen;
};
std::map<std::string, BuildTask> taskMap;
void CpuRsync(int fd, size_t fileSize, int chunkSize, std::vector<ComponentInfo> &vec);

class RsyncClient {
 public:
  explicit RsyncClient(std::shared_ptr<Channel> channel)
      : stub_(Rsync::NewStub(channel)) {}

  bool SendFileHead(const std::string fileName, const int fileSize, const int chunkSize) {
    FileHead request;
    request.set_filename(fileName);
    request.set_filesize(fileSize);
    request.set_chunksize(chunkSize);
    AsyncClientCall* call = new AsyncClientCall;
    call->response_reader = stub_->PrepareAsyncPreCalcu(&call->context, request, &cq_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }

  void SendFileInfo(const std::vector<ComponentInfo>& query, const std::string fileName) {
    FileInfo fileInfo;
    fileInfo.set_filename(fileName);
    for (int i = 0; i < query.size(); ++i) {
      FileChunkInfo *item = fileInfo.add_chunkinfo();
      item->set_chunkid(query[i].chunkid);
      item->set_checksum1(query[i].checksum1);
      item->set_checksum2(query[i].checksum2); 
    }
    AsyncClientCallCalcu* call = new AsyncClientCallCalcu;
    call->response_reader = stub_->PrepareAsyncCalcuDiff(&call->context, fileInfo, &cqcalcu_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }

  void SendUnmatchChunk(int fd, int match_chunk_size, int chunk_size, std::string filename, int fileSize, MatchTokenReply &reply){
    int last_chunk_id = -1;
    UnmatchChunks literalBytes;
    char chunk_buffer[chunk_size];
    literalBytes.set_filename(filename);
    literalBytes.set_filesize(fileSize);
    int sendBytes = 0;
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
        sendBytes += read_size;
        item->set_length(read_size);
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
      sendBytes += read_size;
      item->set_content(chunk_buffer);
      item->set_length(read_size);
    }
    std::cout << "in R2sync client need to send " << sendBytes << " bytes" << std::endl;
    AsyncClientCall* call = new AsyncClientCall;
    call->response_reader = stub_->PrepareAsyncBuildNewFile(&call->context, literalBytes, &cqbuild_);
    call->response_reader->StartCall();
    call->response_reader->Finish(&call->reply, &call->status, (void*)call);
  }
  void AsyncCompleteRpcCalcu() {
    void* got_tag;
    bool ok = false;
    while (cqcalcu_.Next(&got_tag, &ok)) {
        AsyncClientCallCalcu* call = static_cast<AsyncClientCallCalcu*>(got_tag);
        GPR_ASSERT(ok);
        if (call->status.ok()){
          int size = call->reply.tokeninfo_size();
          std::string filename = call->reply.filename();
          BuildTask t = taskMap[filename];
          this->SendUnmatchChunk(t.fd, size, t.chunkSize, filename, t.fileLen, call->reply); 
        }
        else std::cout << "RPC failed" << std::endl;
        delete call;
    }
  }
  void AsyncCompleteRpcBuild() {
    void* got_tag;
    bool ok = false;
    while (cqbuild_.Next(&got_tag, &ok)) {
        AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);
        GPR_ASSERT(ok);
        if (call->status.ok()){
          handled_task_num++;
          std::string filename = call->reply.filename();
          BuildTask t = taskMap[filename];
          close(t.fd);
          taskMap.erase(filename);
        }
        else std::cout << "RPC failed" << std::endl;
        delete call;
    }
  }

 private:
  struct AsyncClientCall {
    RsyncReply reply;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<RsyncReply>> response_reader;
  };
  struct AsyncClientCallCalcu {
    MatchTokenReply reply;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<MatchTokenReply>> response_reader;
  };
  std::unique_ptr<Rsync::Stub> stub_;
  CompletionQueue cq_;
  CompletionQueue cqcalcu_;
  CompletionQueue cqbuild_;
};

struct ReqFileInfo{
  std::vector<ComponentInfo> vec;
  std::string filePath;
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
  std::thread thread1_ = std::thread(&RsyncClient::AsyncCompleteRpcCalcu, &client);
  std::thread thread2_ = std::thread(&RsyncClient::AsyncCompleteRpcBuild, &client);
  gettimeofday(&start,NULL);
  int c_time_use;
  struct timeval c_start;
  struct timeval c_end;
  while(fscanf(rd_fp, "%s", filePath) != EOF){
    struct stat statbuf;
    int fd;
    if((fd = open(filePath, O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open file %s\n",filePath);
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) {
        printf("file with path %s not found\n",filePath);
    }
    std::cout << filePath << std::endl;
    client.SendFileHead(filePath, (size_t) statbuf.st_size, chunkSize);
    close(fd);
  }
  fseek(rd_fp, 0, SEEK_SET);
  std::vector<ReqFileInfo> fvec;
  while(fscanf(rd_fp, "%s", filePath) != EOF){
    std::vector<ComponentInfo> vec;
    struct stat statbuf;
    int fd;
    if((fd = open(filePath, O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open file %s\n",filePath);
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) {
        printf("file with path %s not found\n",filePath);
    }
    gettimeofday(&c_start,NULL);
    CpuRsync(fd, (size_t) statbuf.st_size, chunkSize, vec);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on client, spend " << c_time_use << " us on compute file rc" << std::endl;
    BuildTask t = {fd, chunkSize, filePath, (int) statbuf.st_size};
    taskMap.insert(std::make_pair(filePath, t));
    ReqFileInfo fi = {vec, filePath};
    fvec.push_back(fi);
  }
  for(auto it : fvec){
    client.SendFileInfo(it.vec, it.filePath);
  }
  thread1_.join();  
  thread2_.join();
  return 0;
}

void CpuRsync(int fd, size_t fileSize, int chunkSize, std::vector<ComponentInfo> &vec){
  int chunk_num = ceil(fileSize / chunkSize); 
  int chunk_id = 0;
  char chunk_buffer[chunkSize];
  size_t read_size = 0;
  int s1 = 0, s2 = 0;
  
  while(chunk_id <= chunk_num){
      read_size = read(fd, chunk_buffer, chunkSize);
      if(read_size < 0){
          printf("Error in reading");
      }
      static int rolling_checksum;
      rolling_checksum = get_checksum1(chunk_buffer, (int32)read_size, &s1, &s2);
      uint8_t sum2[8];
      get_checksum2((uint8_t*)chunk_buffer, (size_t)read_size, (uint8_t*)sum2);      
      ComponentInfo component = {chunk_id, rolling_checksum}; 
      strncpy(component.checksum2, sum2, 8);
      vec.push_back(component);
      chunk_id ++;
  }
}