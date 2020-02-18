#ifndef __BIN_LIGHTWEIGHTTASK__
#define __BIN_LIGHTWEIGHTTASK__

#define LWT_MAGIC           'LWT'

#define LWP_TYPE_FIX_ADDR   0x01
#define LWP_TYPE_DYN_ADDR   0x02

#define LWP_ARG_MAX         8

#include <dfs.h>

//注意字节对齐 4byte对齐格式
struct rt_lwt
{
    uint8_t lwt_type;
    uint8_t heap_cnt;
    uint8_t reserv[2];

    rt_list_t hlist;                                    /**< headp list */

    uint8_t *text_entry;
    uint32_t text_size;

    uint8_t *data;
    uint32_t data_size;

    uint32_t *kernel_sp;                                /**< kernel stack point */
    struct dfs_fdtable fdt;
    void *args;

    char    cmd[8];
};

struct lwt_header
{
    uint8_t magic;
    uint8_t compress_encrypt_algo;
    uint16_t reserved;

    uint32_t crc32;
};

struct lwt_chunk
{
    uint32_t total_len;

    char name[4];
    uint32_t data_len;
    uint32_t data_len_space;
};


/**
 * 轻量级进程 执行文件
 * 在父进程中fork一个子进程
**/
void lwt_execve(char *filename, int argc, char **argv, char **envp);




#endif