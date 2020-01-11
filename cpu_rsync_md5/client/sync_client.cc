#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>    
#include <sys/stat.h> 
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

struct Task
{
  HashTable *ht;
  char *file;
  int *match_chunkid; 
  int *match_offset;
  char *unmatch_value;
  int *unmatch_offset;
  int file_len;
};

void CpuRsync(int fd, size_t fileSize, int chunkSize, std::vector<ComponentInfo> &vec);



class RsyncClient {
 public:
  explicit RsyncClient(std::shared_ptr<Channel> channel)
      : stub_(Rsync::NewStub(channel)) {}


  Task GenChecksumList(int fd, const std::string fileName, const int chunkSize, const int fileLen) {
    FileHead filehead;
    filehead.set_filename(fileName);
    filehead.set_chunksize(chunkSize);
    FileInfo reply;
    ClientContext context;
    Status status = stub_->CalcuDiff(&context, filehead, &reply);
    if (status.ok()) {
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
      char *file = (char *)malloc(sizeof(char)*fileLen);
      size_t read_size = 0;
      while(read_size < fileLen){
        read_size += read(fd, &(file[read_size]), chunk_size);
      }
      int chunk_num = (new_file_len / chunk_size) + 1; 
      int *match_chunkid = (int *)malloc(sizeof(int)*chunk_num);
      int *match_offset = (int *)malloc(sizeof(int)*chunk_num);
      char *unmatch_value = (char *)malloc(sizeof(char)*fileLen);
      int *unmatch_offset = (int *)malloc(sizeof(int)*fileLen);
      memset(match_chunkid, -1, chunk_num);
      Task t = {ht, file, match_chunkid, match_offset, unmatch_value, unmatch_offset, fileLen};
      return t;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      //return "RPC failed";
    }
  }

  void SendFileContent(int *match_chunkid, int *match_offset, char *unmatch_value, int *unmatch_offset, int *file_len, int match_num, int unmatch_num){
    
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

    Task t = client.GenChecksumList(fd, filePath, chunkSize, (size_t) statbuf.st_size);
    // and unmatch info
    int match_num = 0;
    int unmatch_num = 0;
    CpuRsync(t.ht, t.file, t.match_chunkid, t.match_offset, t.unmatch_value, t.unmatch_offset, t.file_len, chunkSize, match_num, unmatch_num);

    client.SendUnmatchChunk(t.match_chunkid, t.match_offset, t.unmatch_value, t.unmatch_offset, t.file_len, match_num, unmatch_num);
    close(fd);
  }
  return 0;
}
