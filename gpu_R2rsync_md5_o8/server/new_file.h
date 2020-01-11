#include "hash.h"

__global__ void kernel_test(Node *d_ht, int *d_pos_array, int total_size);

__global__ void multiwarp_match(Node *ht, char *file, size_t file_len, int total_threads, int chunk_size, int chunk_num, 
          int *match_offset, int *match_chunkid, int *stat, int bucket_num, int c0, int c1, const int4* __restrict__ coef);
__global__ void gpu_recalcu(Node *ht, char *file, int chunk_size, int chunk_num, int *match_offset, int *match_chunkid, 
		int *stat, int region_size, int bucket_num, int c0, int c1, const int4* __restrict__ coef);
void recalcu(int chunk_size, int chunk_num, int *stat, int jump_pos, int file_len, int total_threads,
            char *h_file, int *match_offset, int *match_chunkid, Node *ht, int j, int recalcu_region_size);