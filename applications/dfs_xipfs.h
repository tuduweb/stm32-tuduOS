#ifndef __BIN_DFS_XIPFS__
#define __BIN_DFS_XIPFS__


struct dfs_ramfs
{
    rt_uint32_t magic;

    //struct rt_memheap memheap;
    //struct ramfs_dirent root;
};

int dfs_xipfs_init(void);


#endif