# LWP 轻量级线程

### 主体结构

```c++
struct rt_lwt
{
    mpu_table_t mpu_table;//mpu保护信息

    uint8_t lwt_type;
    uint8_t heap_cnt;
    uint8_t reserv[2];

    struct rt_lwt *parent;  /* 父辈 */
    struct rt_lwt *first_child; /* 首儿子 */
    struct rt_lwt *sibling; /* 同辈 */
    rt_list_t wait_list;

    int32_t finish;
    int lwp_ret;

    rt_list_t hlist;    /**< headp list */

    //程序代码段位置
    uint8_t *text_entry;
    uint32_t text_size;
    //静态数据段
    uint8_t *data;
    uint32_t data_size;

    uint32_t *kernel_sp;    /**< kernel stack point */
    struct dfs_fdtable fdt;
    void *args;
    int ref;

    pid_t pid;
    rt_list_t t_grp;

    char    cmd[8];

    //signal相关
    rt_uint32_t signal;


    //其他
    rt_list_t object_list;  /** 存储里面的object结构 在析构时销毁 **/
};
```