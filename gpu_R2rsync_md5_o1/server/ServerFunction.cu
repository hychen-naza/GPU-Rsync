#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <cmath>
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
#include "recalcu.h"
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

extern std::queue<Task> cputaskq;
extern std::map<std::string, Task> taskMap;
extern std::map<std::string, buildTask> buildTaskMap;
extern cudaStream_t *stream;

extern int time_use;
extern struct timeval start;
extern struct timeval end;
extern int handled_task_num;





class RsyncServiceImpl final : public Rsync::Service {

  Status PreCalcu(ServerContext* context, const FileHead* request,
                  RsyncReply* reply) override {
    if(handled_task_num == 0) gettimeofday(&start,NULL);
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
    //char buffer[chunk_size];
    //std::string s = "touted aphelions biorhythms Wright Albigensian shoeing growwastrels. slothfulness blazoned ca9b6)XNa1 UbVgk6%WV+O$EYl5I9l^NuN";
    while(read_size < fileLen){
        read_size += read(fd, &(file[read_size]), chunk_size);
        /*read_size += read(fd, buffer, chunk_size);
        std::string str = buffer;
        if(str.find(s) != std::string::npos){
            std::cout << "this string in pos " << read_size-chunk_size << std::endl;
            exit(0);
        }*/
    }
    int local_round_num = round_num % n_stream;
    // use multistream or not
    //round_num ++;
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
    Task t = {rsyncFileName, file, h_stat, match_chunkid, match_offset, d_file, \
              d_match_chunkid, d_match_offset, d_ht, d_stat, (int)fileLen, \
              chunk_size, total_chunk_num, total_threads, local_round_num, NULL, {}, fd};   
    taskMap.insert(std::pair<std::string, Task> (rsyncFileName, t));
    //printf("server precalcu finished, vector size %d, with content \n",(int)taskMap.size());
    reply->set_success(true);
    return Status::OK;
  }

  Status CalcuDiff(ServerContext* context, const FileInfo* request,
                  MatchTokenReply* reply) override {
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    Node *ht = (Node*)malloc(sizeof(Node)*HASHSIZE);
    for(int i = 0;i<HASHSIZE;++i){
        ht[i].chunk_id = -1;
        ht[i].next = NULL;
    }
    int size = request->chunkinfo_size();
    std::vector<std::vector<int> > matchIdVec(size); // id - id_vec
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const int checksum1 = item.checksum1();
      char checksum2[16];
      strncpy(checksum2, item.checksum2().c_str(), 16);      
      if((insert_hashtable(ht, chunkId, checksum1, checksum2, matchIdVec))==1){}
      else printf("insert failed\n");     
    }
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on get checksum list from client" << std::endl;

    gettimeofday(&c_start,NULL);
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
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on waiting for preparation to finish" << std::endl;
    
    t.ht = ht;
    cudaMemcpy(t.dHt, ht, sizeof(Node)*HASHSIZE, cudaMemcpyHostToDevice);
    int nthreads = threads_num;
    int nblocks = ceil(float(t.totalThreads) / nthreads);
    //std::cout << "total threads " << t.totalThreads << ", threads in a block " << nthreads << ", nblocks " << nblocks << std::endl;
    int i = t.roundNum;
    multiwarp_match<<<nblocks, nthreads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.fileLen, t.totalThreads, t.chunkSize, chunk_num, 
        t.dMatchOffset, t.dMatchChunkid, t.dStat);
    // sync point, wait for memcpy asyn finish
    cudaEvent_t event, event1;
    cudaEventCreate(&event);
    cudaEventRecord(event, stream[i]);
    cudaEventSynchronize(event); 
    cudaEventDestroy(event);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on copying ht into gpu and gpu computation" << std::endl;

    gettimeofday(&c_start,NULL);
    int recalcu_threads = ceil(float(t.totalThreads)/recalcu_region_size)-1;
    std::cout << "recalcu_region_size is " << recalcu_region_size << ", recalcu_threads is " << recalcu_threads << std::endl;
    gpu_recalcu<<<1, recalcu_threads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.chunkSize, chunk_num, t.dMatchOffset, t.dMatchChunkid, t.dStat, recalcu_region_size);
    cudaMemcpyAsync(t.stat, t.dStat, sizeof(int)*t.totalThreads,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchOffset, t.dMatchOffset, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchChunkid, t.dMatchChunkid, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaEventCreate(&event1);
    cudaEventRecord(event1, stream[i]);
    cudaEventSynchronize(event1); 
    cudaEventDestroy(event1);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , single rsync file with size " << t.fileLen << ", spend " << c_time_use << " us on gpu first recalcu" << std::endl;

    
    /*while(cudaEventQuery(event)==cudaErrorNotReady){
      std::this_thread::yield();
    }*/
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
    std::cout << "on server , single rsync file with size " << t.fileLen << ", spend " << c_time_use << " us on cpu final recalcu" << std::endl;

    cudaFree(t.dFileContent);
    cudaFree(t.dMatchChunkid);
    cudaFree(t.dMatchOffset);   
    cudaFree(t.dHt);
    cudaFree(t.dStat);

    free(t.ht);
    buildTask bt = {t.fileName, t.chunkSize, t.totalThreads, t.fd, t.file, t.stat, t.matchChunkid, t.matchOffset, matchIdVec};
    buildTaskMap[bt.fileName] = bt;

    gettimeofday(&c_start,NULL);
    std::set<int> matchTokens;
    for(int i=0;i<t.totalChunkNum;++i){
        if(t.matchChunkid[i] != -1){
            std::vector<int> idset = matchIdVec[t.matchChunkid[i]];
            //std::cout << "match chunk id " << t.matchChunkid[i] << " has match id set size " << idset.size() << std::endl;
            for(auto it = idset.begin(); it != idset.end(); ++it){
                matchTokens.insert(*it);
            }
        }
    }
    std::cout << "match token size " << matchTokens.size() << std::endl;
    for (auto it = matchTokens.begin(); it != matchTokens.end(); ++it) {
      MatchToken *item = reply->add_tokeninfo();
      item->set_chunkid(*it);
    }
    // push it to another queue
    // 这样做的话，最后一个cputask会一直没人处理
    //t.matchIdVec = matchIdVec;
    taskMap.erase(request->filename());
    //cputaskq.push(t); // 等下，直接在这里做cpu revise把
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , rsync file size " << t.fileLen << ", spend " << c_time_use << " us on sending back to client" << std::endl;
    return Status::OK;
  }

  Status BuildNewFile(ServerContext* context, const UnmatchChunks* request,
                  RsyncReply* reply) override {
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
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
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , new file content receive and build spend " << c_time_use << " us" << std::endl;
    char chunk_buffer[t.chunkSize];
    size_t read_size = 0;

    gettimeofday(&c_start,NULL);
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
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , using old file content and build spend " << c_time_use << " us" << std::endl;

    gettimeofday(&c_start,NULL);
    std::ofstream outf;
    outf.open("construct.txt");
    outf.write(new_file, new_file_len);
    outf.close();
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server, construct new file spend " << c_time_use << " us" << std::endl;
    
    gettimeofday(&c_start,NULL);
    close(t.fd);
    cudaFreeHost(t.matchChunkid);
    cudaFreeHost(t.matchOffset);
    cudaFreeHost(t.stat); 
    cudaFreeHost(t.file);
    //taskMap.erase(filename);
    reply->set_success(true);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server, free cuda host file spend " << c_time_use << " us" << std::endl;
    //handled_task_num++;
    if(handled_task_num == 0){
      gettimeofday(&end,NULL);
      time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
      std::cout << "single rsync file with size " << new_file_len << ", spend " << time_use << " us on build new file" << std::endl;
    }
    return Status::OK;
  }
};