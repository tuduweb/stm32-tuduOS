/**
 * 轻量级线程 Light Weight Process
 * 由于是RTT4.02版本的再构建,所以暂时取名为LWT,Process->Task
 * 主要用来实现.bin文件的加载执行等
**/


#include <lwt.h>
#include <rtthread.h>
#include <rthw.h>
#include <dfs_posix.h>

#define DBG_TAG    "LWP"
#define DBG_LVL    DBG_WARNING
#include <rtdbg.h>

//---->以下为日志单元的配置项
#define LOG_TAG     "lwt"     // 该模块对应的标签。不定义时，默认：NO_TAG
//#define LOG_LVL     LOG_LVL_DBG   // 该模块对应的日志输出级别。不定义时，默认：调试级别
#include <ulog.h>                 // 必须在 LOG_TAG 与 LOG_LVL 下面
//日志单元配置项结束<----


struct lwt_pidmap lwt_pid;

extern void lwp_user_entry(void *args, const void *text, void *data);
void lwt_set_kernel_sp(uint32_t *sp)
{
    rt_thread_t thread = rt_thread_self();
    struct rt_lwt *user_data;
    user_data = (struct rt_lwt *)rt_thread_self()->lwp;
    user_data->kernel_sp = sp;
    //kernel_sp = sp;
}

uint32_t *lwt_get_kernel_sp(void)
{
    struct rt_lwt *user_data;
    user_data = (struct rt_lwt *)rt_thread_self()->lwp;
    //return kernel_sp;
    return user_data->kernel_sp;
}

/**
 * 参数复制 把参数拷贝到lwt的结构体中
**/
static int lwt_argscopy(struct rt_lwt *lwt, int argc, char **argv)
{
    int size = sizeof(int)*3; /* store argc, argv, NULL */
    int *args;
    char *str;
    char **new_argv;
    int i;
    int len;

    for (i = 0; i < argc; i ++)
    {
        size += (rt_strlen(argv[i]) + 1);
    }
    size  += (sizeof(int) * argc);

    args = (int*)rt_malloc(size);
    if (args == RT_NULL)
        return -1;

    str = (char*)((int)args + (argc + 3) * sizeof(int));
    new_argv = (char**)&args[2];
    args[0] = argc;
    args[1] = (int)new_argv;

    for (i = 0; i < argc; i ++)
    {
        len = rt_strlen(argv[i]) + 1;
        new_argv[i] = str;
        rt_memcpy(str, argv[i], len);
        str += len;
    }
    new_argv[i] = 0;
    lwt->args = args;

    return 0;
}
#include "easyflash.h"
static int lwt_load(const char *filename, struct rt_lwt *lwt, uint8_t *load_addr, size_t addr_size)
{
    int fd;
    uint8_t *ptr;
    int result = RT_EOK;
    int nbytes;
    struct lwt_header header;
    struct lwt_chunk  chunk;

    /* check file name */
    RT_ASSERT(filename != RT_NULL);
    /* check lwp control block */
    RT_ASSERT(lwt != RT_NULL);

    /* 根据加载地址判断地址是否为Fix mode */
    if (load_addr != RT_NULL)
    {
        lwt->lwt_type = LWP_TYPE_FIX_ADDR;
        ptr = load_addr;
    }
    else
    {
        lwt->lwt_type = LWP_TYPE_DYN_ADDR;
        ptr = RT_NULL;
    }


    const char* itemname;
    /* 查找文件名(也就是去除掉目录) -> 若不存在待查字符,则返回空指针*/
    itemname = strrchr( filename, '/');
    if(itemname == RT_NULL)
        itemname = filename;//不存在'/',那么直接拿来用
    else
        itemname++;//去除掉'/'只要后面的 比如 bin/app.bin -> app.bin
    //这个参数干嘛用的呢..
    rt_strncpy(lwt->cmd, itemname, 8);
    
    /* 这里需要更换成xipfs 现在暂时是fatfs */
    fd = open(filename, 0, O_RDONLY);

    if (fd < 0)
    {
        dbg_log(DBG_ERROR, "open file:%s failed!\n", filename);
        result = -RT_ENOSYS;
        goto _exit;
    }else{
        //fd >= 0

        struct env_meta_data env;

        //ioctl
        if(ioctl(fd, 0x0002, &env) != RT_EOK)
        {
            LOG_E("Can't find that [%s] ENV!", filename);
            result = -RT_EEMPTY;
            goto _exit;
        }



        //判断是否能XIP启动,无法XIP启动那么需要复制到RAM中

        //lwp->text_size = RT_ALIGN(chunk.data_len_space, 4);
        lwt->text_size = RT_ALIGN(env.value_len, 4);
        rt_uint8_t * new_entry = (rt_uint8_t *)rt_malloc( lwt->text_size );//RT_MALLOC_ALGIN
        rt_uint8_t * align_entry = (rt_uint8_t *)RT_ALIGN((int)new_entry, 4);

        //new_entry = RT_ALIGN(new_entry);

        if (new_entry == RT_NULL)
        {
            dbg_log(DBG_ERROR, "alloc text memory faild!\n");
            result = -RT_ENOMEM;
            goto _exit;
        }
        else
        {
            dbg_log(DBG_LOG, "lwp text malloc : %p, size: %d!\n", align_entry, lwt->text_size);
        }

        rt_kprintf("lwp text malloc : %p, size: %d!\n", align_entry, lwt->text_size);

        //复制内容
        int nbytes = read(fd, align_entry, lwt->text_size);

        if(nbytes != lwt->text_size)
        {
            result = -RT_EIO;
            goto _exit;
        }

        lwt->text_entry = align_entry;


        


        //在app里面第9位
        lwt->data_size = 0x400;//(uint8_t)*(lwt->text_entry + 9);//addr
        //申请数据空间
        //lwt->data_entry = rt_lwt_alloc_user(lwt,lwt->data_size);
        lwt->data_entry = rt_malloc(lwt->data_size);
        if (lwt->data_entry == RT_NULL)
        {
            dbg_log(DBG_ERROR, "alloc data memory faild!\n");
            result = -RT_ENOMEM;
            goto _exit;
        }

    }

    //if()

#if 0
//.bin文件不存在这些
    /* read lwp header */
    nbytes = read(fd, &header, sizeof(struct lwt_header));
    if (nbytes != sizeof(struct lwt_header))
    {
        dbg_log(DBG_ERROR, "read lwp header return error size: %d!\n", nbytes);
        result = -RT_EIO;
        goto _exit;
    }

    /* check file header */
    if (header.magic != LWT_MAGIC)
    {
        dbg_log(DBG_ERROR, "erro header magic number: 0x%02X\n", header.magic);
        result = -RT_EINVAL;
        goto _exit;
    }
#endif
    
_exit:
    if(fd >= 0)
        close(fd);
    
    if(result != RT_EOK)
    {
        //
    }
    
    return result;
}

struct rt_lwt *rt_lwt_self(void)
{
    rt_thread_t tid = rt_thread_self();
    if(tid == RT_NULL)
        return RT_NULL;
    return tid->lwp;
}

struct rt_lwt *rt_lwp_new(void)
{
    struct rt_lwt *lwt = RT_NULL;
    //关闭中断 主要是MPU保护这一块
    rt_uint32_t level = rt_hw_interrupt_disable();

    //在pidmap中找一个空闲的位置,如果不存在空闲位置,则满

    
    //如果能找到位置 那么申请变量,初始化参量
    lwt = (struct rt_lwt *)rt_malloc(sizeof(struct rt_lwt));

    if(lwt == RT_NULL)
    {
        LOG_E("no memory for lwp struct!");

    }else{

        //置位,赋0
        rt_memset(lwt, 0, sizeof(*lwt));

        //重置双向链表
        lwt->wait_list.prev = &lwt->wait_list;
        lwt->wait_list.next = &lwt->wait_list;
        lwt->object_list.prev = &lwt->object_list;
        lwt->object_list.next = &lwt->object_list;
        lwt->t_grp.prev = &lwt->t_grp;
        lwt->t_grp.next = &lwt->t_grp;
        lwt->ref = 1;//引用次数

        //lwt->pid = //申请到的pid map中的位置(下标)
        

        //把这个lwt结构体放入 pid_map相应下标形成映射关系
    }



    //重新开启保护
    rt_hw_interrupt_enable(level);

    return lwt;
}

void lwt_ref_inc(struct rt_lwt *lwt)
{
    rt_uint32_t level = rt_hw_interrupt_disable();
    
    lwt->ref++;

    rt_hw_interrupt_enable(level);
}

void lwt_ref_dec(struct rt_lwt *lwt)
{
    rt_uint32_t level = rt_hw_interrupt_disable();
    
    if(lwt->ref > 0)
    {
        lwt->ref--;
        if(lwt->ref == 0)
        {
            //无任何地方引用,执行删除操作

            //共享内存
            //引用对象
            //数据删除

            //以上操作需要防止内存溢出
        }
    }

    rt_hw_interrupt_enable(level);
}

char* lwt_get_name_from_pid(pid_t pid)
{
    struct rt_lwt *lwt;
    char *name;
    if( lwt = lwt_pid.pidmap[pid] )
    {
        //返回字符最后一次出现的指针
        if(name = strrchr(lwt->cmd, '/'))
        {
            return name + 1;
        }
    }

    return RT_NULL;
}

pid_t lwt_get_pid_from_name(char *name)
{
    //在pid_map中进行查找操作..
    for(int i = 0; i < LWP_PIDMAP_SIZE; ++i)
    {
        //
    }
    return -RT_ERROR;
}

pid_t lwt_get_pid(void)
{
    struct rt_lwt *lwt;
    if(lwt)
    {
        return 1;//rt_thread_self()->lwp->pid;
    }
    return 0;
}

void lwt_cleanup(rt_thread_t tid)
{
    struct rt_lwt *lwt;

    dbg_log(DBG_INFO, "thread: %s, stack_addr: %08X\n", tid->name, tid->stack_addr);

    lwt = (struct rt_lwt *)tid->lwp;

    if (lwt->lwt_type == LWP_TYPE_DYN_ADDR)
    {
        dbg_log(DBG_INFO, "dynamic lwp\n");
        if (lwt->text_entry)
        {
            dbg_log(DBG_LOG, "lwp text free: %p\n", lwt->text_entry);
#ifdef RT_USING_CACHE
            rt_free_align(lwt->text_entry);
#else
            rt_free(lwt->text_entry);
#endif
        }
        if (lwt->data_entry)
        {
            dbg_log(DBG_LOG, "lwp data free: %p\n", lwt->data_entry);
            rt_free(lwt->data_entry);
        }
    }

    //rt_free(lwt);


    //还需要清 LWT
}

extern void lwp_user_entry(void *args, const void *text, void *data);

void lwt_thread_entry(void* parameter)
{
    rt_thread_t thread;
    struct rt_lwt* lwt;

    thread = rt_thread_self();
    lwt = thread->lwp;
    thread->cleanup = lwt_cleanup;
    //thread->stack_addr = 0;

    lwp_user_entry(lwt->args, lwt->text_entry, lwt->data_entry);
    
}

#include "dfs_file.h"
/**
 * 执行操作
 * envp里存放的是系统的环境变量
**/
int lwt_execve(char *filename, int argc, char **argv, char **envp)
{
    struct rt_lwt *lwt;

    if (filename == RT_NULL)
        return -RT_ERROR;

    //把这里换成新建的函数体 类似于c++中的新建一个实例
    lwt = (struct rt_lwt *)rt_malloc(sizeof(struct rt_lwt));
    
    if (lwt == RT_NULL)
    {
        dbg_log(DBG_ERROR, "lwt struct out of memory!\n");
        return -RT_ENOMEM;
    }
    dbg_log(DBG_INFO, "lwt malloc : %p, size: %d!\n", lwt, sizeof(struct rt_lwt));

    //执行清空内存操作
    rt_memset(lwt, 0, sizeof(*lwt));

    //拷贝入口参数到 lwt->args
    if (lwt_argscopy(lwt, argc, argv) != 0)
    {
        rt_free(lwt);
        return -ENOMEM;
    }

    //为lwt构建空间
    if(lwt_load(filename, lwt, 0, 0) != RT_EOK)
    {
        return -RT_ERROR;
    }

    //构建线程FD表
    int fd = libc_stdio_get_console();
    //get然后put是为了ref_count不增加
    struct dfs_fd *d = fd_get(fd);
    fd_put(d);
    fd = fd - DFS_FD_OFFSET;


    char* name = strrchr(filename, '/');
    rt_thread_t thread = rt_thread_create( name ? name + 1: filename, lwt_thread_entry, NULL, 0x400, 29, 200);
    if(thread == RT_NULL)
    {
        //
    }

    int IRID = rt_hw_interrupt_disable();
    
    /* 如果当前是LWP进程,那么构造相互关系 */

    struct rt_lwt *current_lwt;
    current_lwt = rt_thread_self()->lwp;

    if(current_lwt)
    {
        lwt->sibling = current_lwt->first_child;
        current_lwt->first_child = lwt;
        lwt->parent = current_lwt;
    }

    thread->lwp = lwt;

    lwt->t_grp.next->prev = &thread->sibling;
    thread->sibling.next = lwt->t_grp.next;
    lwt->t_grp.next = &thread->sibling;
    thread->sibling.prev = &lwt->t_grp;

    rt_hw_interrupt_enable(IRID);

    //启动进程
    rt_thread_startup(thread);

_exit:

    return lwt_get_pid();//lwt_to_pid(lwt)

}


/* 部分测试函数 */
#include "lwp.h"

rt_thread_t sys_thread_create(const char *name,
                             void (*entry)(void *parameter),
                             void       *parameter,
                             rt_uint32_t stack_size,
                             rt_uint8_t  priority,
                             rt_uint32_t tick)
{
    struct rt_thread *thread = NULL;
    struct rt_lwp *lwp = NULL;

    //系统调用的是LWP进程
    struct rt_lwt *lwt;

    //entry要改成user_entry(即程序运行在用户态)
    thread = rt_thread_create(name, entry, parameter, stack_size, priority, tick);

    if(thread != NULL)
    {
        //CleanUp里面写了清Text_entry,所以需要判断引用次数才可以
        //thread->cleanup = lwt_cleanup;
        thread->lwp = rt_thread_self()->lwp;

    }

    //lwt->t_grp.next->prev = &thread->

    return thread;
}

rt_err_t sys_thread_startup(rt_thread_t thread)
{
    if(thread == NULL)
    {
        //
    }

    return rt_thread_startup(thread);
}