# GPU-Rsync

- in gpu_R2rsync_md5, I use naive gpu acceleration method to accelerate searching and comparing part of rsync
- in gpu_R2rsync_md5_o1, I further optimize it by using gpu to parallelzie the sequential recalculation part of rsync
- in gpu_R2rsync_md5_o2, I further optimize it by move the work of building new file to gpu not cpu
- in gpu_R2rsync_md5_o3, I forget what optimization I did
- in gpu_R2rsync_md5_o4, I move the work of building new file back to cpu, since gpu is not suitable to do I/O intensive work
- in gpu_R2rsync_md5_o5, I further optimize it by replacing MD5 hash algorithm with siphash in rsync
- in gpu_R2rsync_md5_o6, I make a better workload balancing between cpu and gpu to faster rsync pipeline
- in gpu_R2rsync_md5_o7, I fix some bugs in file transfer
- in gpu_R2rsync_md5_o8, I fail to use perfect hash for rsync
- in gpu_R2rsync_md5_o9, I observe the register usage of every thread to avoid the problem of using too many regsiers and data spilling to main memory
