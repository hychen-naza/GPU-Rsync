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
extern int handled_calcutask_num;


class RsyncServiceImpl final : public Rsync::Service {

  Status PreCalcu(ServerContext* context, const FileHead* request,
                  RsyncReply* reply) override {
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);
    std::string rsyncFileName = request->filename();
    int new_file_len = request->filesize();
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
    cudaMemcpyAsync(d_file, file, sizeof(char)*(fileLen), cudaMemcpyHostToDevice, stream[local_round_num]);
    cudaEvent_t event;
    cudaEventCreate(&event);
    cudaEventRecord(event, stream[local_round_num]);

    char *new_file, *d_new_file;
    cudaHostAlloc((void**)&new_file, sizeof(char)*new_file_len, cudaHostAllocDefault);
    cudaMalloc((void**)&d_new_file, sizeof(char)*new_file_len);
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
    Task t = {rsyncFileName, file, new_file, match_chunkid, match_offset, NULL, h_stat, \
              d_file, d_new_file, d_match_chunkid, d_match_offset, d_ht, d_stat, \
              (int)fileLen, new_file_len, chunk_size, total_chunk_num, total_threads, local_round_num, fd};   
    taskMap.insert(std::pair<std::string, Task> (rsyncFileName, t));
    //printf("server precalcu finished, vector size %d, with content \n",(int)taskMap.size());
    reply->set_success(true);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on preparation" << std::endl;
    return Status::OK;
  }

  Status CalcuDiff(ServerContext* context, const FileInfo* request,
                  MatchTokenReply* reply) override {
    if(handled_task_num ==0) gettimeofday(&start,NULL);
    int c_time_use;
    struct timeval c_start;
    struct timeval c_end;
    gettimeofday(&c_start,NULL);

    int size = request->chunkinfo_size();
    int *matchIdArray = (int*)malloc(sizeof(int)*size); // one to one match
    memset(matchIdArray, -1, size);
    std::map<int, std::set<int> > matchIdMap; // one to more match
    //这个ht最好在这里分配，为的是为上面的准备工作再多些时间
    //如果把ht放在准备工作里做，则必须要先while取出task t，但接收checklist时间依然不会少
    Node *ht = (Node*)malloc(sizeof(Node)*HASHSIZE);
    for(int i = 0;i<HASHSIZE;++i){
        ht[i].chunk_id = -1;
        ht[i].next = NULL;
    }
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const int checksum1 = item.checksum1();
      char checksum2[16];
      strncpy(checksum2, item.checksum2().c_str(), 16);      
      if((insert_hashtable(ht, chunkId, checksum1, checksum2, matchIdArray, matchIdMap))==1){}
      else printf("insert failed\n");
    }
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on get checksum list from client" << std::endl;*/

    //gettimeofday(&c_start,NULL);
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
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on waiting for preparation to finish" << std::endl;*/
    
    //gettimeofday(&c_start,NULL);
    
    t.ht = ht;
    cudaMemcpy(t.dHt, ht, sizeof(Node)*HASHSIZE, cudaMemcpyHostToDevice); //80us
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
    while(cudaEventQuery(event)==cudaErrorNotReady){
      std::this_thread::yield();
    }
    //cudaEventSynchronize(event); 
    cudaEventDestroy(event);
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<<"on server, spend " << c_time_use << " us on copying ht into gpu and gpu computation" << std::endl;*/

    //gettimeofday(&c_start,NULL);
    int recalcu_threads = ceil(float(t.totalThreads)/recalcu_region_size)-1;
    //这里线程安排还需要改wait to implement
    gpu_recalcu<<<1, recalcu_threads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.chunkSize, chunk_num, t.dMatchOffset, t.dMatchChunkid, t.dStat, recalcu_region_size);
    cudaMemcpyAsync(t.stat, t.dStat, sizeof(int)*t.totalThreads,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchOffset, t.dMatchOffset, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(t.matchChunkid, t.dMatchChunkid, sizeof(int)*t.totalChunkNum,cudaMemcpyDeviceToHost,stream[i]);
    cudaEventCreate(&event1);
    cudaEventRecord(event1, stream[i]);
    while(cudaEventQuery(event1)==cudaErrorNotReady){
      std::this_thread::yield();
    }
    //cudaEventSynchronize(event1); 
    //cudaEventDestroy(event1);
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , single rsync file with size " << t.fileLen << ", spend " << c_time_use << " us on gpu first recalcu" << std::endl;*/

    //gettimeofday(&c_start,NULL);
    for(int i=recalcu_region_size-1; i<t.totalThreads; i += recalcu_region_size){
        int t_match_num = t.stat[i];
        int j = i+1;
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
    
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , single rsync file with size " << t.fileLen << ", spend " << c_time_use << " us on cpu final recalcu" << std::endl;*/

    cudaFree(t.dMatchChunkid);
    cudaFree(t.dMatchOffset);   
    cudaFree(t.dHt);
    cudaFree(t.dStat);
    free(t.ht);
    buildTask bt = {t.newFileLen, t.chunkSize, t.totalThreads, t.roundNum, t.fd, t.dFileContent, t.dNewFile, t.file, t.newFile, t.stat, t.matchChunkid, t.matchOffset, matchIdArray, matchIdMap, t.totalChunkNum, size};
    buildTaskMap[t.fileName] = bt;

    //gettimeofday(&c_start,NULL);
    std::set<int> matchTokens;
    for(int i=0;i<t.totalChunkNum;++i){
        if(t.matchChunkid[i] != -1){
            int _chunkid = matchIdArray[t.matchChunkid[i]];
            if(_chunkid != -1){
                matchTokens.insert(_chunkid);
            }
            else{
                std::set<int> idset = matchIdMap[t.matchChunkid[i]];
                for(auto it = idset.begin(); it != idset.end(); ++it){
                    matchTokens.insert(*it);
                }
            }
        }
    }
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , rsync file size " << t.fileLen << ", spend " << c_time_use << " us on pick up tokens need to be send" << std::endl;*/
    
    //gettimeofday(&c_start,NULL);
    std::cout << "match token size " << matchTokens.size() << std::endl;
    reply->set_filename(t.fileName);
    for (auto it = matchTokens.begin(); it != matchTokens.end(); ++it) {
      MatchToken *item = reply->add_tokeninfo();
      item->set_chunkid(*it);
    }
    taskMap.erase(request->filename());
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , rsync file size " << t.fileLen << ", spend " << c_time_use << " us on calcu diff" << std::endl;
    handled_calcutask_num++;
    gettimeofday(&end,NULL);
    
    if(handled_calcutask_num == 3){
      time_use =(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
      std::cout << "8 rsync file with size spend " << time_use << " us on gpu rsyn compute" << std::endl;
    }
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
    // 我这边的while都是为异步处理同一个client的多个文件而准备的,同步就没有意义了
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

    //gettimeofday(&c_start,NULL);

    int *dmatchIdArray;
    cudaMalloc((void**)&dmatchIdArray,sizeof(int)*t.totalNewChunkNum);
    cudaMemcpy(dmatchIdArray, t.matchIdArray, sizeof(int)*t.totalNewChunkNum, cudaMemcpyHostToDevice);
    int total_multi_match_num = (t.totalNewChunkNum/20);
    int *d_multi_match_array;
    cudaMalloc((void**)&d_multi_match_array,sizeof(int)*total_multi_match_num); // 这个20参数是我定的
    int *multi_match_array = (int*)malloc(sizeof(int)*total_multi_match_num);
    int nthreads = threads_num;
    int nblocks = ceil(float(t.totalOldChunkNum) / nthreads);
    
    int i = t.roundNum;
    int *multi_match_num = (int*)malloc(sizeof(int)*1);
    int *d_multi_match_num;
    cudaMalloc((void**)&d_multi_match_num,sizeof(int)*1);
    cudaMemset(d_multi_match_num, 0, sizeof(int)*1);
    oldfile_match_build<<<nblocks, nthreads, 0, stream[i]>>>(t.dFileContent, dmatchIdArray, t.chunkSize, chunk_num, t.matchOffset, t.matchChunkid, t.dNewFile, t.totalOldChunkNum, d_multi_match_array, d_multi_match_num);
    cudaMemcpyAsync(t.newFile, t.dNewFile, sizeof(char)*t.newFileLen,cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(multi_match_array, d_multi_match_array, sizeof(int)*total_multi_match_num, cudaMemcpyDeviceToHost,stream[i]);
    cudaMemcpyAsync(multi_match_num, d_multi_match_num, sizeof(int)*1, cudaMemcpyDeviceToHost,stream[i]);
    cudaEvent_t event;
    cudaEventCreate(&event);
    cudaEventRecord(event, stream[i]);
    cudaEventSynchronize(event); 
    cudaEventDestroy(event);
    char chunk_buffer[t.chunkSize];
    size_t read_size = 0;
    for(int i=0;i<*multi_match_num;++i){
      int match_pos = multi_match_array[i];
      int old_file_pos = t.matchOffset[match_pos];
      std::set<int> idset = t.matchIdMap[t.matchChunkid[match_pos]];  
      lseek(t.fd, old_file_pos, SEEK_SET);
      read_size = read(t.fd, chunk_buffer, t.chunkSize);
      for(auto id : idset){
        int new_file_pos = id * t.chunkSize;
        memcpy(&t.newFile[new_file_pos], chunk_buffer, read_size);
      }
    }
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , using gpu old file content and build spend " << c_time_use << " us" << std::endl;*/
    
    //gettimeofday(&c_start,NULL);
    int size = request->chunkinfo_size();
    for (int i = 0; i < size; i++) {
      const UnmatchChunkInfo item = request->chunkinfo(i);
      const int pos = item.pos();   
      memcpy(&t.newFile[pos], item.content().c_str(), item.length());
    }
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server , new file content receive and build spend " << c_time_use << " us" << std::endl;*/

    //gettimeofday(&c_start,NULL);
    std::ofstream outf;
    outf.open("construct.txt");
    outf.write(t.newFile, t.newFileLen);
    outf.close();
    /*gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server, construct new file spend " << c_time_use << " us" << std::endl;*/
    
    //gettimeofday(&c_start,NULL);
    close(t.fd);
    free(t.matchIdArray);
    cudaFree(dmatchIdArray);
    cudaFree(t.dFileContent);
    cudaFree(t.dNewFile);
    cudaFreeHost(t.newFile);
    cudaFreeHost(t.matchChunkid);
    cudaFreeHost(t.matchOffset);
    cudaFreeHost(t.stat); 
    cudaFreeHost(t.file);
    reply->set_filename(filename);
    reply->set_success(true);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout << "on server, build new file spend " << c_time_use << " us" << std::endl;
    handled_task_num++;
    gettimeofday(&end,NULL);
    
    if(handled_task_num == 3){
      time_use =(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);
      std::cout << "8 rsync file with size spend " << time_use << " us on gpu rsyn compute" << std::endl;
    }
    return Status::OK;
  }
};