#include <stdint.h>
#include <sys/types.h>  
#include "ServerFunction.cu"

int n_stream=16;
int chunk_size = 512;
int threads_num = 512;
int cpu_threads_num = 8;
int chunk_num = 3;
int round_num = 0;
int taskNum = 1;
int handled_task_num = 0;
std::queue<Task> cputaskq;
std::map<std::string, Task> taskMap;
std::map<std::string, buildTask> buildTaskMap;
cudaStream_t *stream;

int time_use;
struct timeval start;
struct timeval end;

#define USAGE               \
"usage:\n"                   \
"  server [options]\n"      \
"options:\n"                   \
"  -s [chunk_size] Chunk size of file\n" \
"  -n [chunk_num_each_thread] Chunk num each thread calculate\n" \
"  -t [threads_num] Threads num for rsync file\n" \
"  -h              Show this help message\n"


/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"chunk_size",  required_argument,      NULL,           's'},
  {"chunk_num_each_thread",  required_argument,  NULL,    'n'},
  {"threads_num",  required_argument,     NULL,           't'},
  {"cpu_threads_num",  required_argument, NULL,           'c'},
  {"help",        no_argument,            NULL,           'h'},
  {NULL,            0,                    NULL,            0}
};

void *CPURevise(void *arg){
  while(1){
    if(!cputaskq.empty()){
      Task tt = cputaskq.front();  
      cputaskq.pop(); 
      cudaFree(tt.dFileContent);
      cudaFree(tt.dMatchChunkid);
      cudaFree(tt.dMatchOffset);   
      cudaFree(tt.dHt);
      cudaFree(tt.dStat);      
      for(int i=0;i<tt.totalThreads-1;++i){
        int t_match_num = tt.stat[i];
        int j = i+1; 
        // 在没有了testing area后，beginPos肯定是chunk_size*chunk_num*(i+1)
        // 这种状态很好理解，就是上一个thread在最后有match，下一个thread也有match
        // 而且jump后位置超过了下一个thread的第一个match位置
        if(t_match_num > 0 && tt.stat[j] > 0 && tt.matchOffset[chunk_num*i+t_match_num-1]+chunk_size > tt.matchOffset[chunk_num*j]){      
          int jump_pos = tt.matchOffset[chunk_num*i+t_match_num-1]+chunk_size;  
          recalcu(chunk_size, chunk_num, tt.stat, jump_pos, tt.fileLen, tt.totalThreads,
                tt.file, tt.matchOffset, tt.matchChunkid, tt.ht, j);    
        }
      } 
      free(tt.ht);

      buildTask bt = {tt.fileName, tt.chunkSize, tt.totalThreads, tt.fd, tt.file, tt.stat, tt.matchChunkid, tt.matchOffset, tt.matchIdVec};
      buildTaskMap[bt.fileName] = bt;
      /*taskNum++;
      //std::cout << taskNum << std::endl;
      if(taskNum == 17){
        gettimeofday(&end,NULL);
        time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//us nearly 40000us
        printf("16 rsync files total computation time_use is %d us\n", (int)time_use);
      }*/
    }
    else{
      std::this_thread::yield();
    }
  }
}



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

  int option_char = 0;
  while ((option_char = getopt_long(argc, argv, "o:f:s:n:t:c:h", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        fprintf(stderr, "%s", USAGE);
        exit(__LINE__);
      case 's': // chunk size
        chunk_size = atoi(optarg);
        break; 
      case 'n': // chunk size
        chunk_num = atoi(optarg);
        break;                                              
      case 't': // rsync_file-path
        threads_num = atoi(optarg);
        break;
      case 'c': // rsync_file-path
        cpu_threads_num = atoi(optarg);
        break;
      case 'h': // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
    }
  }
  stream = GPUWarmUp(n_stream);
  // CPU thread 
  /*pthread_t tidp;
  int ret;
  ret = pthread_create(&tidp, NULL, CPURevise, NULL);    //创建线程
  if (ret){
    printf("pthread_create failed:%d\n", ret);
    return -1;
  }*/

  RunServer();

  for(int i=0; i<n_stream; i++){
    cudaStreamDestroy(stream[i]);
  }
}
