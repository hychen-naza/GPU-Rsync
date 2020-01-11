#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <sys/types.h>  
#include <unistd.h>
#include <pthread.h>
#include <grpcpp/grpcpp.h>
#include "rsync.grpc.pb.h"
#include "checksum.h"
#include "hashtable.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using rsync::FileInfo;
using rsync::FileHead;
using rsync::FileChunkInfo;
using rsync::RsyncReply;
using rsync::Rsync;



struct Task
{
  std::string fileName;
  char *fileContent;
  int *matchChunkid;
  int *matchOffset;
  char *unmatchValue;
  int *unmatchOffset;
  int fileLen;
  int chunkSize;
};

std::vector<Task> taskVec;

void CpuRsync(HashTable *ht, char *file, int *match_chunkid, int *match_offset, char *unmatch_value, \
  int *unmatch_offset, int file_len, int chunk_size){
  
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
      //not pass the first check, almost failed in this match test, showing hash func works well
      if(np == NULL){
          move_chunk = 0;
          recalcu = 0;
          goto RecordInfo;
      }
      else{
          //not pass the second check
          if(np->checksum != rolling_checksum){
              //printf("checksum not match\n");
              move_chunk = 0;
              recalcu = 0;
              goto RecordInfo;
          }
          else{
              static char sum2[16];
              get_checksum2(&file[offset], length, sum2);
              for(int i=0;i<16;++i){
                  //not pass the third check
                  if(sum2[i]!=np->md5[i]){ 
                      //printf("md5 not match\n");
                      move_chunk = 0;
                      recalcu = 0;
                      goto RecordInfo;
                  }
              }
              chunk_id = (int)np->chunk_id;
              move_chunk = 1;
              recalcu = 1;
              goto RecordInfo;
          }
      }
RecordInfo:      

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
}


class RsyncServiceImpl final {
  public: 
    ~RsyncServiceImpl() {
      server_->Shutdown();      
      cq_->Shutdown();
    }

    void Run() {
      std::string server_address("0.0.0.0:50051");
      ServerBuilder builder;
      builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&service);
      cq_ = builder.AddCompletionQueue();
      server_ = builder.BuildAndStart();
      std::cout << "Server listening on " << server_address << std::endl;
      HandleRpcs();
    }


  private:
  // Class encompasing the state and logic needed to serve a request.
  class CallData {
    public:
    CallData(Rsync::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        status_ = PROCESS;
        service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_,
                                  this);
      } else if (status_ == PROCESS) {

        new CallData(service_, cq_);
        std::string prefix("Hello ");
        reply_.set_message(prefix + request_.name());
        status_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(status_ == FINISH);
        delete this;
      }
    }

    private:

    Rsync::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    ServerCompletionQueue* cq_;
    ServerContext ctx_;
    FileHead request_;
    RsyncReply reply_;
    ServerAsyncResponseWriter<RsyncReply> responder_;
    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;  
  };
  void HandleRpcs() {
    new CallData(&service_, cq_.get());
    void* tag;  
    bool ok;
    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  Rsync::AsyncService service_;
  std::unique_ptr<Server> server_;
};





  Status PreCalcu(ServerContext* context, const FileHead* request,
                  RsyncReply* reply) override {
    std::string rsyncFileName = request->filename();
    int chunk_size = request->chunksize();
    struct stat statbuf;
    int fd;
    if((fd = open(rsyncFileName.c_str(), O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open rsync file.\n");
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) printf("file with path %s not found\n",rsyncFileName.c_str());
    
    size_t file_len = (size_t) statbuf.st_size;
    char *file = (char *)malloc(sizeof(char)*file_len);
    size_t read_size = 0;
    while(read_size < file_len){
        read_size += read(fd, &(file[read_size]), chunk_size);
    }
    char *unmatch_value = (char *)malloc(sizeof(char)*file_len);
    int *unmatch_offset = (int *)malloc(sizeof(int)*file_len);
    int chunk_num = (file_len / chunk_size) + 1; 
    int *match_chunkid = (int *)malloc(sizeof(int)*chunk_num);
    int *match_offset = (int *)malloc(sizeof(int)*chunk_num);
    Task t = {rsyncFileName, file, match_chunkid, match_offset, unmatch_value, unmatch_offset, (int)file_len, chunk_size};
    taskVec.push_back(t);
    reply->set_success(true);
    printf("server precalcu finished\n");
    return Status::OK;
  }

  Status CalcuDiff(ServerContext* context, const FileInfo* request,
                  RsyncReply* reply) override {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    create_hashtable(ht);
    int size = request->chunkinfo_size();
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const int checksum1 = item.checksum1();
      char checksum2[16];
      strncpy(checksum2, item.checksum2().c_str(), 16);
      if((insert_hashtable(ht, chunkId, checksum1, checksum2))==1){}
      else printf("insert failed\n");
    }
    printf("server insert finished\n");

    for(auto iter = taskVec.begin(); iter != taskVec.end(); ++iter){
      if(iter->fileName.compare(request->filename())==0){
        printf("server begin compute\n");
        CpuRsync(ht, iter->fileContent, iter->matchChunkid, iter->matchOffset, iter->unmatchValue, iter->unmatchOffset, iter->fileLen, iter->chunkSize);
        
        free(iter->fileContent);
        free(iter->matchChunkid);
        free(iter->matchOffset);
        free(iter->unmatchValue);
        free(iter->unmatchOffset);
        printf("server end compute\n");
        taskVec.erase(iter);
        break;
      }
    }
    free(ht);
    reply->set_success(true);
    printf("server end rsync\n");
    return Status::OK;
  }




int main(int argc, char** argv) {

  float time_use=0;
  struct timeval start;
  struct timeval end;
  gettimeofday(&start,NULL); 

  ServerImpl server;
  server.Run();

  gettimeofday(&end,NULL);
  time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
  printf("files with size 512 KB cpu rsync time_use is %d us\n", (int)time_use);

  return 0;
}




