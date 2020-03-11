#include <rtthread.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#include "dfs_ramfs.h"
#include "dfs_romfs.h"

#include "dfs_fs.h"
#include "dfs_posix.h"
/* �ļ�ϵͳ�Զ����ر� */
const struct dfs_mount_tbl mount_table[]=
{
  //{"sd0","/","elm",0,0},
  //{"W25Q128","/dev0","elm",0,0},
  {0}
};


#include "fal.h"



int mnt_init(void)
{
	/*
		if(mkdir("/ram",0x777))
		{
			rt_kprintf("mkdir ram!\n");
		}

		
		if(mkdir("/rom",0x777))
		{
			rt_kprintf("mkdir rom!\n");
		}
*/


	if (dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root)) == 0)
    {
        rt_kprintf("ROM file system initializated!\n");
    }
    else
    {
        rt_kprintf("ROM file system initializate failed!\n");
    }
	
	
	
    if (dfs_mount(RT_NULL, "/ram", "ram", 0, dfs_ramfs_create(rt_malloc(1024),1024)) == 0)
    {
        rt_kprintf("RAM file system initializated!\n");
    }
    else
    {
        rt_kprintf("RAM file system initializate failed!\n");
    }
		
		
		
		


    return 0;
}
INIT_ENV_EXPORT(mnt_init);

#include <rtdbg.h>
#define FS_PARTITION_NAME  "exchip0"

int flash_init()
{
    /* ��ʼ�� fal ���� */
    fal_init();
	
    /* �� spi flash ����Ϊ "exchip0" �ķ����ϴ���һ�����豸 */
    struct rt_device *flash_dev = fal_blk_device_create(FS_PARTITION_NAME);
    if (flash_dev == NULL)
    {
        LOG_E("Can't create a block device on '%s' partition.", FS_PARTITION_NAME);
    }
    else
    {
        LOG_D("Create a block device on the %s partition of flash successful.", FS_PARTITION_NAME);
    }

    /* ���� spi flash ����Ϊ "dev0" �ķ����ϵ��ļ�ϵͳ */
    if (dfs_mount(flash_dev->parent.name, "/dev0", "elm", 0, 0) == 0)
    {
        LOG_I("Filesystem initialized!");
    }
    else
    {
        LOG_E("Failed to initialize filesystem!");
        LOG_D("You should create a filesystem on the block device first!");
    }
    extern void easyflash_init();
    easyflash_init();

    return 0;
}


INIT_ENV_EXPORT(flash_init);



#endif
