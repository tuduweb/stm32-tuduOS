/* Generated by mkromfs. Edit with caution. */
#include <rtthread.h>
#include <dfs_romfs.h>







static const rt_uint8_t _romfs_root_directory_hello2_txt[] = {
0x68,0x65,0x6c,0x6c,0x6f,0x20,0x64,0x69,0x72,0x65,0x63,0x74,0x6f,0x72,0x79,0x21
};

static const struct romfs_dirent _romfs_root_directory[] = {
    {ROMFS_DIRENT_FILE, "hello2.txt", (rt_uint8_t *)_romfs_root_directory_hello2_txt, sizeof(_romfs_root_directory_hello2_txt)/sizeof(_romfs_root_directory_hello2_txt[0])}
};

static const rt_uint8_t _romfs_root_hello_txt[] = {
0x68,0x65,0x6c,0x6c,0x6f,0x20,0x74,0x65,0x78,0x74,0x20,0x77,0x6f,0x72,0x6c,0x64,0x21
};





static const struct romfs_dirent _romfs_root[] = {
    {ROMFS_DIRENT_DIR, "bin", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "dev0", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "dev1", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "directory", (rt_uint8_t *)_romfs_root_directory, sizeof(_romfs_root_directory)/sizeof(_romfs_root_directory[0])},
    {ROMFS_DIRENT_FILE, "hello.txt", (rt_uint8_t *)_romfs_root_hello_txt, sizeof(_romfs_root_hello_txt)/sizeof(_romfs_root_hello_txt[0])},
    {ROMFS_DIRENT_DIR, "ram", RT_NULL, 0},
    {ROMFS_DIRENT_DIR, "spi", RT_NULL, 0}
};

const struct romfs_dirent romfs_root = {
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_romfs_root, sizeof(_romfs_root)/sizeof(_romfs_root[0])
};