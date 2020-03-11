/**
 * XIP文件系统
 * 主要是存放一些可以执行xip操作的文件
 * 参考FrostOS
**/
#include <xipfs.h>
#include <rtthread.h>

#include <dfs_fs.h>
#include <dfs_file.h>

#include <easyflash.h>

//---->以下为日志单元的配置项
#define LOG_TAG     "xipfs"     // 该模块对应的标签。不定义时，默认：NO_TAG
//#define LOG_LVL     LOG_LVL_DBG   // 该模块对应的日志输出级别。不定义时，默认：调试级别
#include <ulog.h>                 // 必须在 LOG_TAG 与 LOG_LVL 下面
//日志单元配置项结束<----


//跟easyflash原始文件的区别是，把一些全局变量封装到了这个结构体中
struct ef_env_dev{
    uint32_t env_start_addr;
    const ef_env *default_env_set;//key->value关系变量
    size_t default_env_set_size;
    _Bool init_ok;
    _Bool gc_request;
    _Bool in_recovery_check;
    void *flash;    //此结构体等同于初始化时候的addr，用于ef_port的首地址
    size_t sector_size;
    struct env_cache_node env_cache_table[16];
    struct sector_cache_node sector_cache_table[4];
};
typedef struct ef_env_dev *ef_env_dev_t;


enum env_status {
    ENV_UNUSED = 0x0,
    ENV_PRE_WRITE = 0x1,
    ENV_WRITE = 0x2,
    ENV_PRE_DELETE = 0x3,
    ENV_DELETED = 0x4,
    ENV_ERR_HDR = 0x5,
    ENV_STATUS_NUM = 0x6,
};
typedef enum env_status env_status_t;


struct root_dirent{
    struct ef_env_dev env_dev;
    struct env_meta_data env;
    uint32_t sec_addr;
};

typedef struct root_dirent *root_dirent_t;


struct root_dirent* xip_mount_table[2] = {RT_NULL};

/**
 * 一些变量 可能还没有初始化 需要注意
**/
rt_mutex_t ef_write_lock;//写入操作mutex锁
rt_event_t ef_write_event;//写入操作事件


struct root_dirent* get_env_by_dev(rt_device_t dev_id)
{
    void* deviceType = dev_id->parent.type;//type of kernel object

    for(int table_id = 0; table_id < 2; ++table_id)
    {
        if(xip_mount_table[table_id]->env_dev.flash == dev_id->parent.type)
            return xip_mount_table[table_id];
    }

    return 0;
}

/**
 * 文件系统 挂载
 * 把fs->path下挂载文件系统,其中含有私有数据data
 * fs   : path,ops,dev_id
 * data : 文件系统的私有数据 在dfs_mount调用中自行传入
**/
int mount_index = 0;
int dfs_xipfs_mount(struct dfs_filesystem *fs,
                    unsigned long          rwflag,
                    const void            *data)
{
    if(fs->dev_id->type == RT_NULL)
    {
        rt_kprintf("The flash device type must be Char!\n");
        /* Not a character device */
        return -ENOTTY;
    }

    //已经挂在过了 在ef里面存在这些信息!?
    if( get_env_by_dev(fs->dev_id) )
        return RT_EOK;
    
    if(mount_index >= 2u)
        return -RT_ERROR;
    mount_index++;

    /* 如果没有挂载表 那么在这里新建挂载表 */
    for(int table_id = 0; table_id < 2; ++table_id)
    {
        ef_env_dev_t ef_env_dev = rt_malloc(sizeof(ef_env_dev_t));

        //类型 void *flash;
        ef_env_dev->flash = fs->dev_id->parent.type;//得到flash?
        //sector_size保存在哪里?在挂载的物理硬件的信息里
        ef_env_dev->sector_size = fs->dev_id;//得到sector_size

        //ef_env_init_by_flash(ef_env_dev);

    }

    //如果挂载表新建成功 那么需要执行清理等步骤?
    //清理:就是把app分区里面的碎片给整理了,
    //换句话说 可以把app一个个的搬运到新分区中,这样可以形成新表
    return RT_EOK;

}

/**
 * 取消挂载
 * 在这里需要停止所有xip操作吗?
**/
int dfs_xipfs_unmount(struct dfs_filesystem *fs)
{
    root_dirent_t root_dirent = get_env_by_dev(fs->dev_id);
    
    if(root_dirent)
        rt_free(root_dirent);

    return RT_EOK;
}

/**
 * 在某个dev_id上初始化xipfs文件系统
 * 如果配置表中没有这个devid的信息 那么直接返回
 * 如果配置表中有这个devid信息 那么把信息配置到ef系统
**/
int dfs_xipfs_mkfs(rt_device_t dev_id)
{
    struct root_dirent* mount_table;
    mount_table = get_env_by_dev(dev_id);
    
    //以下函数需要改造
    if(mount_table)
        ef_env_set_default();//ef_env_set_default(mount_table->env_dev);

    return RT_EOK;
}

#include <fal.h>
/**
 * 获取文件磁盘信息 放在buf中
**/
int dfs_xipfs_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    size_t block_size, total_size,free_size;
    struct root_dirent* root_dirent;

    //参数键入env中的name

    root_dirent = get_env_by_dev(fs->dev_id);
    //env_table->env_dev.flash从这里面搞出分区名字?
    //需要找到正确的变量 ..... block_size & f_blocks
    block_size = fal_flash_device_find( root_dirent->env.name )->blk_size;
    total_size = 512;//在env中获取已占用大小
    buf->f_bsize  = block_size;
    buf->f_blocks = root_dirent->env.len / block_size;//ramfs->memheap.pool_size / 512;
    buf->f_bfree  = 512;//ramfs->memheap.available_size / 512;

    return RT_EOK;
}


/**
 * 文件系统 改名
**/
int dfs_xipfs_rename(struct dfs_filesystem *fs,
                     const char            *oldpath,
                     const char            *newpath)
{
    //当前知道是可以在“物理”上，对app进行更名的
    //用二进制查看工具 如 HxD,可以看到在特定位置有APP的名称

    //但是更名操作势必要进行很多步骤 所以谨慎编写使用
    //!注意 如果直接替换存储器上的内容,那么需要重新擦除存储器内容,禁止。

    return -RT_ERROR;
}


/**
 * XIP文件系统 删除操作
**/
int dfs_xipfs_unlink(struct dfs_filesystem *fs, const char *path)
{
    root_dirent_t root_dirent;
    
    root_dirent = get_env_by_dev(fs->dev_id);

    if(root_dirent)
    {
        //在easyflash中删除这个变量,没有了这个的信息,代表删除操作
        //这种方法称为软删除 所以会造成“硬件”上的碎片化
        // ef_dev_env env_dev filename[fs->path + 1]
    }

    return -RT_ERROR;
}

/**
 * XIP文件系统 获得文件状态
**/
int dfs_xipfs_stat(struct dfs_filesystem *fs,
                   const char            *path,
                   struct stat           *st)
{
    //需要判断path是目录,还是文件:目录和文件返回的数据是不一样的
    //返回的数据是在st这个实参中的

    struct root_dirent* root_dirent = get_env_by_dev(fs->dev_id);
    //envmetadata

    //如果这个device_id下有配置的xip环境 也就是 是xip环境
    if(root_dirent)
    {
        st->st_dev = 0;
        st->st_size = 0;
        st->st_mtime = 0;

        //如果是获取的文件夹信息"根目录"
        if(*path == '/' && !*(path+1))
        {
            //文件夹
            st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                        S_IWUSR | S_IWGRP | S_IWOTH;
            st->st_mode &= ~S_IFREG;
            st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
            return RT_EOK;
        }
        //如果在env中找到了这个"path"也就是文件,那么返回文件. 如果没找到 那么返回错误"文件不存在"
        //if( find_env( &root_dirent->env_dev, path+1, &env))

        //获取的文件信息 也就是app的信息

        //find_env root_dirent->env_dev path+1 &env

        //更改文件size
        //更改文件属性 mode 只读等等
    }

    return -RT_ERROR;
}


/**
 * XIPFS 文件系统下 文件的操作 打开
**/
int dfs_xipfs_open(struct dfs_fd *file)
{
    root_dirent_t root_dirent;
    struct dfs_filesystem *fs = (struct dfs_filesystem *)file->data; 

    //取或操作
    //如果打开的是根目录,那么->false
    //如果打开的操作为目录,那么false
    //上述 不能打开目录
    if( !file->path[1] ||  !(file->flags & O_PATH))//获得一个能表示文件在文件系统中位置的文件描述符
    {
        root_dirent = get_env_by_dev(fs->dev_id);

        if(root_dirent)
        {
            if(file->flags & 0x0000)
            {
                //find_env root_dirent->env_dev file->path+1 &root_dirent->dev
                root_dirent->env.addr.start = -1;
                root_dirent->sec_addr = -1;
                file->pos = 0;
                file->data = root_dirent;
                file->size = 0;
                return RT_EOK;
            }
        }else{
            LOG_E("can't find dev_id!");
        }
    }

    return -RT_ERROR;
}

int dfs_xipfs_close(struct dfs_fd *file)
{
    //主要是销毁文件资源 free
    //要看看在这里有没有文件资源呢

    rt_mutex_take(ef_write_lock, 0);
    
    if(ef_write_event)
    {
        //ef_buf_count = 0;
        rt_event_send(ef_write_event, 1u << 1);
    }

    rt_mutex_release(ef_write_lock);
    rt_thread_delay(1u);

    return RT_EOK;
}

/**
 * XIPFS IO操作
 * 根据CMD命令字进行系统级的一些操作[获取剩余存储大小 / ]
**/
int dfs_xipfs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    ef_env_dev_t env_dev;
    //根据命令字cmd
    switch (cmd)
    {
    case 0x0000:
        env_dev = (ef_env_dev_t)file->data;
        if(env_dev)
        {
            //获取remain_size 剩余大小
        }
        break;

    case 0x0001:
        env_dev = (ef_env_dev_t)file->data;
        if(env_dev)
        {
            //那么find_env
            //find_env(env_dev, file->path+1,)
            
            //查找start_addr
            //把start_addr 放入 arg 返回
        }
        break;

    
    default:
        //命令字错误
        break;
    }

    LOG_E("can't find data!");    

    return -ENOSYS;
}

int dfs_xipfs_read(struct dfs_fd *file, void *buf, size_t len)
{
    size_t save_size;
    rt_size_t length;
    int result;
    //result = ef_get_env_blob()

    //从file->pos开始读 len位,如果len超过文件size,那么需要处理

    //result = ef_get_env_stream()
    //实现主要是 根据env_dev->flash 读取 n字节到buf中!?

    //READ功能主要是在读取一些字节信息的时候用到

    //pos指针移动length长度
    file->pos += length;

    return length;
}


/**
 * XIPFS 线程写入函数实体
 **/
void xipfs_write_entry(struct dfs_fd* file)
{
    if(ef_set_env_blob(file->path + 1, 0, file->size))
    {
        //出现错误 发送事件
        rt_event_send(ef_write_event, 1u << 1);
    }else{
        //
        rt_event_delete(ef_write_event);
        ef_write_event = 0;
    }
}

int dfs_xipfs_write(struct dfs_fd *file, const void *buf, size_t len)
{
    ef_env_dev_t env_dev;

    rt_thread_t tid;

    file->path;

    //把类型强制转换
    env_dev = (ef_env_dev_t)file->data;

    //取得锁的控制权
    if( !rt_mutex_take(ef_write_lock, 0) )
    {
        //无错误 那么取得了控制权

        if(file->size > 0)
        {
            if(!ef_write_event)
            {
                ef_write_event = rt_event_create("ef_e", 0);

                if(!ef_write_event || !(tid = rt_thread_create("write_t",xipfs_write_entry,file,0x1000u,5,12)))
                {
                    //创建不了 那么是有错误的 内存不足
                    return -RT_ENOMEM;
                }
                //开辟一个内核线程用来写入
                rt_thread_startup(tid);

            }
            //存在ef_write_event事件
            rt_event_send(ef_write_event,1u << 1);
            
            rt_uint32_t recv;
            //等待接收
            if(rt_event_recv(ef_write_event,1u << 1,RT_EVENT_FLAG_OR || RT_EVENT_FLAG_CLEAR,0,&recv))
            {
                //报错了
            }
            if(recv & 1u << 2)
            {
                //事件2
            }
            if(recv & 1u << 1)
            {
                //事件1
                rt_event_delete(ef_write_event);
                ef_write_event = RT_NULL;
            }

        }else{
            //size == 0 文件夹形式 那么写入环境变量!?
            int errCode = ef_set_env_blob(file->path+1, buf, len);
            if(!errCode)
            {
                //无错误
            }else if(errCode == 6)
            {
                //
            }else{
                //
            }
        }

    }

    //释放控制权
    rt_mutex_release(ef_write_lock);


    return RT_EOK;
}


/**
 * XIPFS get directory entry
 * 似乎是获取目录?
 * https://linux.die.net/man/2/getdents
**/
int dfs_xipfs_getdents(struct dfs_fd *file, struct dirent *dirp, uint32_t count)
{
    root_dirent_t dir;
    rt_uint32_t index;
    struct dirent *d;

    //getdents may convert from the native format to fill the linux_dirent.

    /* make integer count */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0)
        return -EINVAL;
    
    index = 0;

    dir = (root_dirent_t) file->data;

    while(1)
    {
        //递推当前的d
        d = dirp + index;

        //遍历 转换出需要的大小

        //从env_dev中获取目录信息 input: dir
        //env_get_fs_getdents

        d->d_type = DT_REG;//文件
        d->d_namlen = dir->env.name_len;
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, dir->env.name, dir->env.name_len);
        d->d_name[d->d_namlen] = 0;//添加结束符

        index ++;
        if(index * sizeof(struct dirent) >= count)
            break;
    }

    //index = 0的情况是在执行循环体前出错 这时候没有数据
    if (index == 0)
        return -RT_ERROR;


    //pos递增 这里需要吗?
    //file->pos += index * sizeof(struct dirent);

    return index * sizeof(struct dirent);
}

/**
 * XIP下 文件操作方法
**/
static const struct dfs_file_ops _dfs_xip_fops =
{
    dfs_xipfs_open,
    dfs_xipfs_close,
    dfs_xipfs_ioctl,
    dfs_xipfs_read,
    dfs_xipfs_write,
    RT_NULL,//dfs_elm_flush,//把缓存刷新到存储介质中
    RT_NULL,//dfs_elm_lseek,//更改pos的偏移量
    dfs_xipfs_getdents,
    RT_NULL, /* poll interface */
};

static const struct dfs_filesystem_ops _dfs_xipfs =
{
    "xip",
    DFS_FS_FLAG_DEFAULT,
    &_dfs_xip_fops,

    dfs_xipfs_mount,// 挂载操作, 实际上是把带有文件系统(已mkfs)的设备device附加到dir上, 然后我们就可以通过访问dir来访问这个设备.
    dfs_xipfs_unmount,
    dfs_xipfs_mkfs,//用于全片擦除后创建文件系统,只是存储器介质上有这个文件系统? 其参数为 dev_id 是存储器的id
    dfs_xipfs_statfs,

    dfs_xipfs_unlink,
    dfs_xipfs_stat,
    dfs_xipfs_rename,
};

int dfs_xipfs_init(void)
{
    /* register ram file system */
    dfs_register(&_dfs_xipfs);

    return 0;
}
//INIT_COMPONENT_EXPORT(dfs_xipfs_init);
