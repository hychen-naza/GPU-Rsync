#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <cmath>
#include "stdint.h"
#include <inttypes.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/time.h>
#include <map>
#include <queue>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <grpcpp/grpcpp.h>
#include "hash.h"
#include "new_file.h"
#include "rsync.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using rsync::FileInfo;
using rsync::FileHead;
using rsync::FileChunkInfo;
using rsync::MatchTokenReply;
using rsync::MatchToken;
using rsync::UnmatchChunks;
using rsync::UnmatchChunkInfo;
using rsync::RsyncReply;
using rsync::Rsync;

extern int n_stream;
extern int chunk_size;
extern int threads_num;
extern int cpu_threads_num;
extern int chunk_num;
extern int round_num;
extern int taskNum;
extern int recalcu_region_size;
extern int total_task_num;

extern std::queue<Task> cputaskq;
extern std::map<std::string, Task> taskMap;
extern std::map<std::string, buildTask> buildTaskMap;
extern cudaStream_t *stream;

extern int time_use;
extern struct timeval start;
extern struct timeval end;
extern std::atomic<int> handled_task_num;

class RsyncServiceImpl final : public Rsync::Service {

  Status PreCalcu(ServerContext* context, const FileHead* request,
                  RsyncReply* reply) override {
    
    std::string rsyncFileName = request->filename();
    chunk_size = request->chunksize();
    struct stat statbuf;
    int fd;
    if((fd = open(rsyncFileName.c_str(), O_RDWR, 0777)) < 0){
        fprintf(stderr, "Unable to open rsync file.\n");
        exit(1);
    }
    if (0 > fstat(fd, &statbuf)) printf("file with path %s not found\n",rsyncFileName.c_str());
    
    size_t fileLen = (size_t) statbuf.st_size;

    char *d_file, *file;
    cudaHostAlloc((void**)&file, sizeof(char)*fileLen,cudaHostAllocDefault);
    cudaMalloc((void**)&d_file, sizeof(char)*fileLen);
    size_t read_size = 0;
    while(read_size < fileLen){
        read_size += read(fd, &(file[read_size]), chunk_size);
    }
    int local_round_num = round_num % n_stream;
    // use multistream or not
    round_num ++;

    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);

    cudaMemcpyAsync(d_file, file, sizeof(char)*(fileLen), cudaMemcpyHostToDevice, stream[local_round_num]);
    cudaEvent_t event;
    cudaEventCreate(&event);
    cudaEventRecord(event, stream[local_round_num]);

    int total_chunk_num = ceil(float(fileLen) / chunk_size); 
    int total_threads = ceil(float(fileLen) / (chunk_num*chunk_size)); 

    int *match_offset, *match_chunkid, *h_stat; 
    cudaHostAlloc((void**)&match_offset, sizeof(int)*total_chunk_num, cudaHostAllocDefault);
    cudaHostAlloc((void**)&match_chunkid, sizeof(int)*total_chunk_num, cudaHostAllocDefault);
    cudaHostAlloc((void**)&h_stat, sizeof(int)*total_threads, cudaHostAllocDefault);
    int *d_match_offset, *d_match_chunkid, *d_stat; 
    cudaMalloc((void**)&d_match_offset,sizeof(int)*total_chunk_num);
    cudaMalloc((void**)&d_match_chunkid,sizeof(int)*total_chunk_num);
    cudaMalloc((void**)&d_stat,sizeof(int)*total_threads);
    cudaMemset(d_match_offset, -1, sizeof(int)*total_chunk_num);
    cudaMemset(d_match_chunkid, -1, sizeof(int)*total_chunk_num);
    cudaMemset(d_stat, 0, sizeof(int)*total_threads);
    Node *d_ht;
    cudaMalloc((void**)&d_ht,sizeof(Node)*HASHSIZE);
    // 在这里才检测是否传输完成file内容
    cudaEventSynchronize(event);
    cudaEventDestroy(event);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the spend " << c_time_use << " us allocate gpu memory in preparation" << std::endl;

    Task t = {rsyncFileName, file, h_stat, match_chunkid, match_offset, d_file, \
              d_match_chunkid, d_match_offset, d_ht, d_stat, (int)fileLen, \
              chunk_size, total_chunk_num, total_threads, local_round_num, NULL, {}, fd};   
    taskMap.insert(std::pair<std::string, Task> (rsyncFileName, t));
    reply->set_success(true);
    return Status::OK;
  }

  Status CalcuDiff(ServerContext* context, const FileInfo* request,
                  MatchTokenReply* reply) override {
    handled_task_num++;
    int cur_num = handled_task_num;
    if(cur_num == 1){
      gettimeofday(&start,NULL);
    } 
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    Node *ht = (Node*)malloc(sizeof(Node)*HASHSIZE);
    for(int i = 0;i<HASHSIZE;++i){
        ht[i].chunk_id = -1;
    }
    int size = request->chunkinfo_size();
    std::vector<std::vector<int> > matchIdVec(size); // id - id_vec
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const int checksum1 = item.checksum1();
      const uint64_t md5 = item.checksum2();
      uint8_t checksum2[8];
      checksum2[0] = (md5 & 0xFF); 
      checksum2[1] = ((md5 >> 8) & 0xFF); 
      checksum2[2] = ((md5 >> 16) & 0xFF); 
      checksum2[3] = ((md5 >> 24) & 0xFF);
      checksum2[4] = ((md5 >> 32) & 0xFF); 
      checksum2[5] = ((md5 >> 40) & 0xFF); 
      checksum2[6] = ((md5 >> 48) & 0xFF);
      checksum2[7] = ((md5 >> 56) & 0xFF);
      if((insert_hashtable(ht, chunkId, checksum1, checksum2, matchIdVec))==1){}
      else printf("insert failed\n");     
    }
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us construct hash table" << std::endl;

    Task t;
    // make sure we finish the preparation
    while(1){
      auto it = taskMap.find(request->filename());
      if(it != taskMap.end()){
        t = it->second;
        break;
      }
      else{
        std::this_thread::yield(); // CalcuDiff是不是应该多线程，不然这里的yield没有意义啊
      }
    }

    t.ht = ht;
    cudaMemcpy(t.dHt, ht, sizeof(Node)*HASHSIZE, cudaMemcpyHostToDevice);
    int nthreads = threads_num;
    int nblocks = ceil(float(t.totalThreads) / nthreads);

    int i = t.roundNum;
    gettimeofday(&c_start,NULL);
    multiwarp_match<<<nblocks, nthreads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.fileLen, t.totalThreads, t.chunkSize, chunk_num, 
        t.dMatchOffset, t.dMatchChunkid, t.dStat);
    // sync point, wait for memcpy asyn finish
    // 这个memcpy必须放在这里，否则stat会有bug，不知道为何
    cudaMemcpyAsync(t.stat, t.dStat, sizeof(int)*t.totalThreads,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchOffset, t.dMatchOffset, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchChunkid, t.dMatchChunkid, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaEvent_t event, event1;
    cudaEventCreate(&event);
    cudaEventRecord(event, stream[i]);
    while(cudaEventQuery(event)==cudaErrorNotReady){
      std::this_thread::yield();
    }
    cudaEventDestroy(event);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us gpu computation" << std::endl;

    int recalcu_threads = ceil(float(t.totalThreads)/recalcu_region_size)-1;
    gpu_recalcu<<<1, recalcu_threads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.chunkSize, chunk_num, t.dMatchOffset, t.dMatchChunkid, t.dStat, recalcu_region_size);
    cudaEventCreate(&event1);
    cudaEventRecord(event1, stream[i]);
    while(cudaEventQuery(event1)==cudaErrorNotReady){
      std::this_thread::yield();
    }
    cudaEventDestroy(event1);
    //gettimeofday(&c_end,NULL);
    //c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    //std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us gpu computation" << std::endl;

    gettimeofday(&c_start,NULL);
    for(int i=recalcu_region_size-1; i<t.totalThreads; i += recalcu_region_size){
        int t_match_num = t.stat[i];
        int j = i+1;
        // 在没有了testing area后，beginPos肯定是chunk_size*chunk_num*(i+1)
        // 这种状态很好理解，就是上一个thread在最后有match，下一个thread也有match
        // 而且jump后位置超过了下一个thread的第一个match位置
        int jump_pos = t.matchOffset[chunk_num*i+t_match_num-1]+chunk_size;  
        if(t_match_num > 0 && t.stat[j] > 0 && jump_pos > t.matchOffset[chunk_num*j]){
          if(i+recalcu_region_size > t.totalThreads){
            int last_region_size = t.totalThreads - i;
            recalcu(chunk_size, chunk_num, t.stat, jump_pos, t.fileLen, t.totalThreads,
                t.file, t.matchOffset, t.matchChunkid, t.ht, j, last_region_size);
          }
          else{
            recalcu(chunk_size, chunk_num, t.stat, jump_pos, t.fileLen, t.totalThreads,
                t.file, t.matchOffset, t.matchChunkid, t.ht, j, recalcu_region_size);
          }     
        }        
    }

    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us cpu computation" << std::endl;

    gettimeofday(&c_start,NULL);
    cudaFree(t.dFileContent);
    cudaFree(t.dMatchChunkid);
    cudaFree(t.dMatchOffset);   
    cudaFree(t.dHt);
    cudaFree(t.dStat);
    free(t.ht);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us free gpu memory" << std::endl;

    gettimeofday(&c_start,NULL);
    buildTask bt = {t.fileName, t.chunkSize, t.totalThreads, t.fd, t.file, t.stat, t.matchChunkid, t.matchOffset, matchIdVec};
    buildTaskMap[bt.fileName] = bt;
    std::set<int> matchTokens;
    for(int i=0;i<t.totalChunkNum;++i){
        if(t.matchChunkid[i] != -1){
            std::vector<int> idset = matchIdVec[t.matchChunkid[i]];
            for(auto it = idset.begin(); it != idset.end(); ++it){
                matchTokens.insert(*it);
            }
            //std::cout << "match pos " << t.matchOffset[i] << ", match chunk " << t.matchChunkid[i] << std::endl;
        }
    }
    reply->set_filename(t.fileName);
    for (auto it = matchTokens.begin(); it != matchTokens.end(); ++it) {
      MatchToken *item = reply->add_tokeninfo();
      item->set_chunkid(*it);
      //std::cout << "match token " << (*it) << std::endl;
    }
    taskMap.erase(request->filename());
    
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us on send back to client" << std::endl;
    if(cur_num == total_task_num){
      gettimeofday(&end,NULL);
      time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
      std::cout << total_task_num << " rsync file with size " << t.fileLen << ", spend " << time_use << " us on calcu diff" << std::endl;
    }
    return Status::OK;
  }

  Status BuildNewFile(ServerContext* context, const UnmatchChunks* request,
                  RsyncReply* reply) override {

    std::string filename = request->filename();
    buildTask t;
    // make sure we finish the cpu revise
    while(1){
      auto it = buildTaskMap.find(request->filename());
      if(it != buildTaskMap.end()){
        t = it->second;
        buildTaskMap.erase(request->filename());
        break;
      }
      else{
        std::this_thread::yield(); // CalcuDiff是不是应该多线程，不然这里的yield没有意义啊
      }
    }
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
    for(int i=0;i<t.totalThreads;++i){
      //int match_num = t.stat[i];
      int match_pos = i*chunk_num;
      // 找到了，是这里，一个match 但chunk_id 803是在第二个位置，
      //for(int j=0;j<match_num;++j){
      for(int j=0;j<chunk_num;++j){
        if(t.matchChunkid[match_pos+j] == -1) continue;
        int old_file_pos = t.matchOffset[match_pos+j];
        std::vector<int> idset = t.matchIdVec[t.matchChunkid[match_pos+j]];
        for(auto id : idset){
            int new_file_pos =  id * t.chunkSize;
            lseek(t.fd, old_file_pos, SEEK_SET);
            read_size = read(t.fd, chunk_buffer, t.chunkSize);
            memcpy(&new_file[new_file_pos], chunk_buffer, read_size);
        }
      }
    }

    std::ofstream outf;
    outf.open("construct.txt");
    outf.write(new_file, new_file_len);
    outf.close();
    close(t.fd);
    cudaFreeHost(t.matchChunkid);
    cudaFreeHost(t.matchOffset);
    cudaFreeHost(t.stat); 
    cudaFreeHost(t.file);
    reply->set_filename(filename);
    reply->set_success(true);
    
    
    return Status::OK;
  }
};
