#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <cmath>
#include "stdint.h"
#include <algorithm>
#include <inttypes.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <ctime>
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
    //Node *d_ht;
    //cudaMalloc((void**)&d_ht,sizeof(Node)*HASHSIZE);
    // 在这里才检测是否传输完成file内容
    cudaEventSynchronize(event);
    cudaEventDestroy(event);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the spend " << c_time_use << " us allocate gpu memory in preparation" << std::endl;

    Task t = {rsyncFileName, file, h_stat, match_chunkid, match_offset, d_file, \
              d_match_chunkid, d_match_offset, NULL, d_stat, (int)fileLen, \
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
    
    int size = request->chunkinfo_size();
    int bucket_up_limit = 128;
    int c0,c1;
    int bucket_num = ceil(float(size)/bucket_up_limit);
    std::vector<std::vector<Node> > buckets(bucket_num);
  BucketInit:
    for(auto vec : buckets){
      vec.clear();
    }
    //std::srand(time(0));
    c0 = std::rand() % 10001;
    c1 = std::rand() % 101;
    uint32 first_chunk_rc;
    std::vector<std::vector<int> > matchIdVec(size); // id - id_vec
    for (int i = 0; i < size; i++) {
      const FileChunkInfo item = request->chunkinfo(i);
      const int chunkId = item.chunkid();
      const uint32 checksum1 = item.checksum1();
      const uint64_t md5 = item.checksum2();
      long long step0 = (c0+c1*abs(checksum1));
      int bucket_id = (step0%1900813+1900813)%bucket_num;
      if(buckets[bucket_id].size()<bucket_up_limit) buckets[bucket_id].push_back(Node{chunkId, checksum1, {(uint8_t)(md5 & 0xFF), (uint8_t)((md5 >> 8) & 0xFF), (uint8_t)((md5 >> 16) & 0xFF), (uint8_t)((md5 >> 24) & 0xFF), (uint8_t)((md5 >> 32) & 0xFF), (uint8_t)((md5 >> 40) & 0xFF), (uint8_t)((md5 >> 48) & 0xFF), (uint8_t)((md5 >> 56) & 0xFF)}});
      else {
        printf("in first round, you should choose new hash function\n");
        goto BucketInit;
      }
      if(i==0) printf("real cpu first roudn rc %d, c0 %d, c1 %d, bucket_id %d, step0 %lld, bucket_num %d\n", checksum1, c0, c1, bucket_id, step0, bucket_num);
    }
    int constant_total_size = buckets.size();
    int4 *constantCoef = (int4*)malloc(sizeof(int4)*constant_total_size);

    int total_size = 0, cur_size = 0;
    for(auto vec : buckets) total_size += ((int)pow(vec.size(),2));
    Node *ht = (Node*)malloc(sizeof(Node)*total_size);
    int first_hash_c0 = c0;
    int first_hash_c1 = c1;

    std::vector<std::vector<int> > HTpos(bucket_num);
    for(int i=0;i<buckets.size();++i){
      int fail_count = 0;
      std::vector<Node> bucket = buckets[i];
      if(bucket.size()>0){
        int bucket_size = bucket.size();
      BucketHashFuncFind: 
        HTpos[i].clear();
        // clean
        if(fail_count>0) memset((void*)&ht[cur_size], 0, sizeof(Node)*pow(bucket_size,2));
        fail_count++;
        c0 = std::rand() % 10001;
        c1 = std::rand() % 101;
        for(auto node : bucket){
          //int step0 = node.checksum;
          long long step0 = c0+c1*(abs(node.checksum));
          int step1 = (step0)%1900813+1900813;
          int bucket_pos = (step1)%(int)pow(bucket_size,2) + cur_size;
          
          if(ht[bucket_pos].checksum == 0){
            ht[bucket_pos] = node;
            HTpos[i].push_back(bucket_pos);
            //printf("in cpu node checksum %d, bucket pos %d, assigned ht rc %d\n", node.checksum, bucket_pos, ht[bucket_pos].checksum);
          }
          else{
            printf("in %d bucket with size %d, we fail to find a perfect hash function, fail time %d\n", i, bucket_size, fail_count);
            goto BucketHashFuncFind;
          }
          if(node.checksum == 1299407492){
            //printf("CPU bucket id %d, rc %d, step01_pos %lld, node pos %d\n", c0, c1, i, node.checksum, step0, bucket_pos);
          }
        }
        constantCoef[i].x = (int)pow(bucket_size,2);
        constantCoef[i].y = c0;
        constantCoef[i].z = c1;
        constantCoef[i].w = cur_size;
        cur_size += (int)pow(bucket_size,2);
      }   
    }
    int4* dCoef;
    cudaMalloc((void**)&dCoef, constant_total_size*sizeof(int4));
    cudaMemcpy(dCoef, constantCoef, constant_total_size*sizeof(int4), cudaMemcpyHostToDevice);
    gettimeofday(&c_end,NULL);
    c_time_use=(c_end.tv_sec-c_start.tv_sec)*1000000+(c_end.tv_usec-c_start.tv_usec);
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << " us construct hash table" << std::endl;
    

    int pos_size=0;
    for(int i=0;i<HTpos.size();++i){
      pos_size += HTpos[i].size();
    }
    int *h_pos_array = (int*)malloc(sizeof(int)*pos_size);


    int array_pos = 0;
    printf("let check host ht data\n");
    for(int i=0;i<HTpos.size();++i){
      for(int j=0;j<HTpos[i].size();++j){
        int pos = HTpos[i][j];
        h_pos_array[array_pos] = pos;
        Node np = ht[pos];
        //printf("node pos %d, checksum %d, h_pos_array %d\n", pos, np.checksum, h_pos_array[array_pos]);
        array_pos ++;
      }
    }
    std::sort(h_pos_array,h_pos_array+pos_size);
    for(int i=0;i<pos_size;++i){
      //printf("have node pos %d\n", h_pos_array[i]);
    }
    
    int *d_pos_array;
    cudaMalloc((void**)&d_pos_array,sizeof(int)*pos_size);
    cudaMemcpy(d_pos_array, h_pos_array, sizeof(int)*pos_size, cudaMemcpyHostToDevice);

    Node *d_ht;
    cudaMalloc((void**)&d_ht,sizeof(Node)*total_size);
    cudaMemcpy(d_ht, ht, sizeof(Node)*total_size, cudaMemcpyHostToDevice);
    kernel_test<<<1, 1>>>(d_ht, d_pos_array, pos_size);
    cudaDeviceSynchronize();
    //exit(1);
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
    t.dHt = d_ht;

    int nthreads = threads_num;
    int nblocks = ceil(float(t.totalThreads) / nthreads);

    int i = t.roundNum;
    gettimeofday(&c_start,NULL);
    printf("start multi warp match\n");
    multiwarp_match<<<nblocks, nthreads, 0, stream[i]>>>(d_ht, t.dFileContent, t.fileLen, t.totalThreads, t.chunkSize, chunk_num, 
        t.dMatchOffset, t.dMatchChunkid, t.dStat, bucket_num, first_hash_c0, first_hash_c1, dCoef);
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
    std::cout<< "the " << cur_num << "th task spend " << c_time_use << "us gpu computation" << std::endl;
    printf("the total ht size %d\n",sizeof(Node)*total_size);
    exit(1);

    int recalcu_threads = ceil(float(t.totalThreads)/recalcu_region_size)-1;
    gpu_recalcu<<<1, recalcu_threads, 0, stream[i]>>>(t.dHt, t.dFileContent, t.chunkSize, chunk_num, t.dMatchOffset, t.dMatchChunkid, 
                  t.dStat, recalcu_region_size, bucket_num, first_hash_c0, first_hash_c1, dCoef);
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
          /*if(i<10){
            printf("thread %d need recalcu, its begin pos %d, jump pos %d, original match pos %d, match num %d\n",j, t.matchOffset[chunk_num*j], jump_pos, t.matchOffset[chunk_num*i], t_match_num);
          }*/
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
