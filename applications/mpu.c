#include <rtthread.h>
#include <mpu.h>
#include <rthw.h>

#include <lwp.h>
#define LOG_TAG     "mpu"     // 该模块对应的标签。不定义时，默认：NO_TAG
//#define LOG_LVL     LOG_LVL_DBG   // 该模块对应的日志输出级别。不定义时，默认：调试级别
#include <ulog.h>                 // 必须在 LOG_TAG 与 LOG_LVL 下面

void bin_lwp_mpu_switch(struct rt_thread *thread)
{
    //这里是mpu的轮转片
    rt_kprintf("%s",thread->name);
}

void bin_mpu_enable()
{
    //
}

void bin_mpu_disable()
{
    //
}


/**
 * 硬件错误处理 在硬件错误的时候会调用此方法
 **/
rt_err_t exception_handle(struct exception_stack_frame *context)
{
    rt_uint32_t level;
    level = rt_hw_interrupt_disable();

    rt_thread_t self = rt_thread_self();
    struct rt_lwp* lwp = (struct rt_lwp *)self->lwp;
    

    if(lwp != RT_NULL)
    {
        LOG_I("process:%s hardfault",self->name);//这里填入命令参数
        rt_kprintf("psr: 0x%08x\n", context->psr);

        rt_kprintf("r00: 0x%08x\n", context->r0);
        rt_kprintf("r01: 0x%08x\n", context->r1);
        rt_kprintf("r02: 0x%08x\n", context->r2);
        rt_kprintf("r03: 0x%08x\n", context->r3);
        rt_kprintf("r12: 0x%08x\n", context->r12);
        rt_kprintf(" lr: 0x%08x\n", context->lr);
        rt_kprintf(" pc: 0x%08x\n", context->pc);


        uint8_t* text_entry = lwp->text_entry;
        //判断一下 如果这里运行的是用户态app,那么打印app入口地址等
        //如何判断是用户态app?


        //用户态app发生错误 终止本线程即可 内核态不受影响
        //线程停止运行

        //执行调度器
        rt_schedule();

        rt_hw_interrupt_enable(level);
        return -1;
    }else{
        //内核发生错误 系统终止 无法挽回
        LOG_E("thread:%s hard fault in kernel",self);
        rt_hw_interrupt_enable(level);
        return 0;
    }

}


void bin_mpu_init()
{
    bin_mpu_disable();
    bin_mpu_enable();
}
