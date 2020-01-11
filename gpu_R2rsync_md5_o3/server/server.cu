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
int recalcu_region_size = 2;
int handled_task_num = 0;
int handled_calcutask_num = 0;
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
  {"recalcu_region_size",  required_argument, NULL,           'r'},
  {"help",        no_argument,            NULL,           'h'},
  {NULL,            0,                    NULL,            0}
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

  int option_char = 0;
  while ((option_char = getopt_long(argc, argv, "o:s:n:t:c:r:h", gLongOptions, NULL)) != -1) {
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
      case 'r': // recalcu_region_size
        recalcu_region_size = atoi(optarg);
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
