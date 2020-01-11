# GPU-Rsync

- in gpu_R2rsync_md5, I use naive gpu acceleration method to accelerate searching and comparing
- in gpu_R2rsync_md5_o1, I further optimize it by using gpu to parallelzie the sequential recalculation
- in gpu_R2rsync_md5_o2, I further optimize it by put the work of building new file in gpu not cpu
- in gpu_R2rsync_md5_o3, I forget what optimization I did
- in gpu_R2rsync_md5_o4, I put the work of building new file back to cpu, since gpu is not suitable to do I/O intensive work
- in gpu_R2rsync_md5_o5, I further optimize it by replacing MD5 hash algorithm with siphash
- in gpu_R2rsync_md5_o6, I make a better workload balancing between cpu and gpu
- in gpu_R2rsync_md5_o7, I fix some bugs in file transfer
- in gpu_R2rsync_md5_o8, I fail to use perfect hash
- in gpu_R2rsync_md5_o9, I observer the register usage of every thread to avoid the problem of using too many regsiers and data spilling to main memory
