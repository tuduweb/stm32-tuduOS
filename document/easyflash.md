# EasyFlash 4.0.0
[github](https://github.com/armink/EasyFlash)
## 官方介绍（[English](#1-introduction)）

[EasyFlash](https://github.com/armink/EasyFlash)是一款开源的轻量级嵌入式Flash存储器库，方便开发者更加轻松的实现基于Flash存储器的常见应用开发。非常适合智能家居、可穿戴、工控、医疗、物联网等需要断电存储功能的产品，资源占用极低，支持各种 MCU 片上存储器。该库主要包括 **三大实用功能** ：

- **ENV** 快速保存产品参数，支持 **写平衡（磨损平衡）** 及 **掉电保护** 功能

EasyFlash不仅能够实现对产品的 **设定参数** 或 **运行日志** 等信息的掉电保存功能，还封装了简洁的 **增加、删除、修改及查询** 方法， 降低了开发者对产品参数的处理难度，也保证了产品在后期升级时拥有更好的扩展性。让Flash变为NoSQL（非关系型数据库）模型的小型键值（Key-Value）存储数据库。
### ...

## 初尝试
    本人使用STM32F429野火开发板进行系统设计；已经使用了 armink 的 FAL 开源项目。固本项目在 FAL 环境下继续移植开发。
### ports/ef_fal_port.c
#### 结构体、变量
+ struct ef_env
+ default_env_set 初始化时的环境变量值

|名称|释义|
|:-|:-:|
|   struct ef_env|   KV型结构体|
|   default_env_set|   初始化时的环境变量|
|   log_buf|    临时数组|
|   struct rt_semaphore env_cache_lock|锁|

##### 初始化
`EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size)`
+ INPUT 为实参，把编程阶段的初始化环境变量值取出。
+ OUTPUT 错误类型

##### FLASH 读取数据接口
`EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size)`


##### FLASH 擦除数据接口，需要特别注意下调用该函数的部分。
`EfErrCode ef_port_erase(uint32_t addr, size_t size)`



##### FLASH 写入部分的接口
`EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size)`


##### 开锁，解锁，缓存锁。
`void ef_port_env_lock(void)`

`void ef_port_env_unlock(void)`

#### EasyFlash 调试接口
`void ef_log_debug(const char *file, const long line, const char *format, ...)`

`void ef_log_info(const char *format, ...)`

`void ef_print(const char *format, ...)`


## 入口函数 easyflash.c
    This file is part of the EasyFlash Library.

**all area sizes must be aligned with** `EF_ERASE_MIN_SIZE`

#### 初始化
`EfErrCode easyflash_init(void)`

easyflash 的整体初始化，在这里我们关注`ef_env_init(default_env_set, default_env_set_size)`

函数在不同的宏定义下有不同的表现，在这里，我们关注4.x版本，即不关注`EF_ENV_USING_LEGACY_MODE`模式下的表现

## 核心程序 src/ef_env.c
### 变量类型、变量

### 函数体
#### set/get status
write granularity 写入粒度 1/4/8

##### 设置状态

`static size_t set_status(uint8_t status_table[], size_t status_num, size_t status_index)`

##### 获取状态

`static size_t get_status(uint8_t status_table[], size_t status_num)`

##### 包含Flash操作的写入/读取操作

`static EfErrCode write_status(uint32_t addr, uint8_t status_table[], size_t status_num, size_t status_index)`

`static size_t read_status(uint32_t addr, uint8_t status_table[], size_t total_num)`

#### Cache操作
##### 更新缓存
`static void update_sector_cache(uint32_t sec_addr, uint32_t empty_addr)`

更新缓存，放置在 sector_cache_table
##### 尝试获取缓存
`static bool get_sector_from_cache(uint32_t sec_addr, uint32_t *empty_addr)`

尝试获取缓存，如果存在,匹配缓存，获取缓存到 empty_addr

##### 更新变量缓存 ENV
`static void update_env_cache(const char *name, size_t name_len, uint32_t addr)`

1. 判断当前内容 addr 是否为 FAILED_ADDR 来判断是增改操作还是删除操作，删除操作的话直接删除单元，即设置 .addr = FAIL_ADDR
2. 更新变量的缓存，采用缓存名字（name）的CRC32校验前16位作为匹配，匹配到直接更新内容（addr）。
3. 记录 FAILED_ADDR 即首个空闲的单元。
4. 在上述过程中比较、记录 active 最小的单元号为 min_activity 在缓存列表满时使用。

##### 获取变量缓存 ENV
`static bool get_env_from_cache(const char *name, size_t name_len, uint32_t *addr)`

1. 在 TABLE 中查找 name_crc 匹配的单元，且 addr 不能为 FAILED_ADDR。
2. 调用接口函数，以 table.addr 读取 FLASH 中相应 NAME 位置的值，来与传入的 name 进行比较。
3. 比较匹配成功，则将 addr 赋值 table.addr 作为输出。
4. 最后，更新 active ，更新步长为 EF_ENV_CACHE_TABLE_SIZE ， active 值不能溢出。Q:步长值怎么取的?

##### 在范围内查找接着的0XFF的FLASH内容
`static uint32_t continue_ff_addr(uint32_t start, uint32_t end)`

##### 在范围内查找临近的 ENV_MAGIC_WORD 区块,在里面获取 env 信息
`static uint32_t find_next_env_addr(uint32_t start, uint32_t end)`
1. 如果有缓存信息，且缓存头为当前 start ，无需判断直接返回 FAILED_ADDR
```
if (get_sector_from_cache(EF_ALIGN_DOWN(start, SECTOR_SIZE), &empty_env) && start == empty_env) {
    return FAILED_ADDR;
}
```

##### 在扇区内找下一个 ENV 地址
`static uint32_t get_next_env_addr(sector_meta_data_t sector, env_node_obj_t pre_env)`

##### 读取 ENV
`static EfErrCode read_env(env_node_obj_t env)`

输入、输出都是靠 env_node_obj_t 结构体指针 env

```c
struct env_node_obj {
    env_status_t status;                         /**< ENV node status, @see node_status_t */
    bool crc_is_ok;                              /**< ENV node CRC32 check is OK */
    uint8_t name_len;                            /**< name length */
    uint32_t magic;                              /**< magic word(`K`, `V`, `4`, `0`) */
    uint32_t len;                                /**< ENV node total length (header + name + value), must align by EF_WRITE_GRAN */
    uint32_t value_len;                          /**< value length */
    char name[EF_ENV_NAME_MAX];                  /**< name */
    struct {
        uint32_t start;                          /**< ENV node start address */
        uint32_t value;                          /**< value start address */
    } addr;
};
typedef struct env_node_obj *env_node_obj_t;
```
##### 读取 Sector meta 扇区信息
`static EfErrCode read_sector_meta_data(uint32_t addr, sector_meta_data_t sector, bool traversal)`

读取扇区下的信息，如 remain(size),empty_env(size) len

##### 得到下一个扇区的地址
`static uint32_t get_next_sector_addr(sector_meta_data_t pre_sec)`

会判断是否连续，用 pre_sec->combined 判断。

##### 迭代操作
`static void env_iterator(env_node_obj_t env, void *arg1, void *arg2, bool (*callback)(env_node_obj_t env, void *arg1, void *arg2))`

对所有扇区进行迭代操作，操作函数为callback，参量为arg1，arg2

##### 在分区中查找变量
`static bool find_env_cb(env_node_obj_t env, void *arg1, void *arg2)`

`static bool find_env_no_cache(const char *key, env_node_obj_t env)`

作为 env_interator 的 callback 迭代函数，作用是

##### 查找环境（变量）[优先从缓存中查找]
`static bool find_env(const char *key, env_node_obj_t env)`

优先从缓存中查找，如果没有找到缓存，再全扇区搜索，然后再更新缓存

##### 判断是否为STRING内容
`static bool ef_is_str(uint8_t *value, size_t len)`

##### 取得环境变量（值）
`static size_t get_env(const char *key, void *value_buf, size_t buf_len, size_t *value_len)`

相比于 find_env ，多出了从 env.addr.value 查找出真正的 value 这一过程。

##### 根据 KEY 查找 ENV 对象,无值
`bool ef_get_env_obj(const char *key, env_node_obj_t env)`

##### 根据 KEY 查找 BLOB BUFFER
`size_t ef_get_env_blob(const char *key, void *value_buf, size_t buf_len, size_t *saved_value_len)`

##### 根据 KEY 查找 STRING *
`char *ef_get_env(const char *key)`

##### 根据 ENV object 查找 value
`size_t ef_read_env_value(env_node_obj_t env, uint8_t *value_buf, size_t buf_len)`

##### 向指定地址中 写入 ENV Header
`static EfErrCode write_env_hdr(uint32_t addr, env_hdr_data_t env_hdr)`

##### 指定 addr 地址擦除，并写入 combined_value
`static EfErrCode format_sector(uint32_t addr, uint32_t combined_value)`

##### 更新扇区状态
`static EfErrCode update_sec_status(sector_meta_data_t sector, size_t new_env_len, bool *is_full)`

如果 is_full 非空，会被赋值 true/false

##### 扇区迭代操作
`static void sector_iterator(sector_meta_data_t sector, sector_store_status_t status, void *arg1, void *arg2, bool (*callback)(sector_meta_data_t sector, void *arg1, void *arg2), bool traversal_env)`

param travelsal_env 是否要对逐个env进行操作（获得 remain(size) 等）
对所有ENV扇区进行迭代操作

##### 统计扇区信息 empty/using sector 用于 gabage clean
`static bool sector_statistics_cb(sector_meta_data_t sector, void *arg1, void *arg2)`

`static uint32_t alloc_env(sector_meta_data_t sector, size_t env_size)`

##### 申请 ENV 空间
`static uint32_t alloc_env(sector_meta_data_t sector, size_t env_size)`

##### 删除环境变量
`static EfErrCode del_env(const char *key, env_node_obj_t old_env, bool complete_del)`

key , old_env 优先执行对 key 的删除

##### 移动 ENV
`static EfErrCode move_env(env_node_obj_t env)`

##### 在 sector 中申请新的ENV
`static uint32_t new_env(sector_meta_data_t sector, size_t env_size)`

##### 在 sector 中申请新的K->V型ENV
`static uint32_t new_env_by_kv(sector_meta_data_t sector, size_t key_len, size_t buf_len)`

##### 扇区垃圾清理
`static bool gc_check_cb(sector_meta_data_t sector, void *arg1, void *arg2)`

`static bool do_gc(sector_meta_data_t sector, void *arg1, void *arg2)`

`static void gc_collect(void)`

1. 扇区垃圾清理，把碎片化的ENV搬到新的分区中，执行完毕后执行初始化操作 format_sector
2. 清理扇区操作有阈值，需要空闲扇区小于阈值才会允许执行。


##### 对齐写入
`static EfErrCode align_write(uint32_t addr, const uint32_t *buf, size_t size)`

`static EfErrCode create_env_blob(sector_meta_data_t sector, const char *key, const void *value, size_t len)`

##### 删除操作
`EfErrCode ef_del_env(const char *key)`

##### 给 key 设置值
`static EfErrCode set_env(const char *key, const void *value_buf, size_t buf_len)`

##### 接口类 创建/删除/更改 ENV
`EfErrCode ef_set_env_blob(const char *key, const void *value_buf, size_t buf_len)`

##### 全部初始化，擦除所有扇区
`EfErrCode ef_env_set_default(void)`

##### 打印全部变量
`static bool print_env_cb(env_node_obj_t env, void *arg1, void *arg2)`

`void ef_print_env(void)`

##### 检查扇区HEADER
`static bool check_sec_hdr_cb(sector_meta_data_t sector, void *arg1, void *arg2)`

检查扇区HEADER check_ok，并初始化，统计 failed_count

##### 检查扇区状态status.dirty == SECTOR_DIRTY_GC
`static bool check_and_recovery_gc_cb(sector_meta_data_t sector, void *arg1, void *arg2)`

##### 检查ENV状态
`static bool check_and_recovery_env_cb(env_node_obj_t env, void *arg1, void *arg2)`

##### EF_LOAD_ENV
`EfErrCode ef_load_env(void)`

##### 初始化
`EfErrCode ef_env_init(ef_env const *default_env, size_t default_env_size)`

什么时候执行初始化操作呢!?


## 特性
FLASH存储器的特性，只能由1写0，并且写入的时候会判断当前写入位置是否为0XFF，所以可以以0XFF为一个状态单位，这也是函数中set_status的核心思想。