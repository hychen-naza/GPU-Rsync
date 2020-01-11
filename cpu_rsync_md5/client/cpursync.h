#include "checksum.h"
#include "hashtable.h"

int CpuRsync(HashTable *ht, char *file, int *match_chunkid, int *match_offset, int file_len, int chunk_size);