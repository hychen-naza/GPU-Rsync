#include "hash.h"

__global__ void multiwarp_match(Node *ht, char *file, size_t file_len, int total_threads, int chunk_size, int chunk_num, 
          int *match_offset, int *match_chunkid, int *stat);
__global__ void gpu_recalcu(Node *ht, char *file, int chunk_size, int chunk_num, int *match_offset, int *match_chunkid, int *stat, int region_size);
void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads,
            char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j, int recalcu_region_size);