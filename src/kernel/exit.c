#include <thread.h>
#include <memory.h>
#include <debug.h>
#include <assert.h>
#include <signal.h>


extern task_block_t* all_threads[]; //from thread.c

extern void release_memory(task_block_t* task, bool preserve_pd);     //from memory.c
extern task_block_t* get_task_by_pid(pid_t pid);    //from thread.c
extern size_t get_index_by_pid(pid_t pid);   //from thread.c

extern bool get_interrupt_state();            //from interrupt_state

extern int sys_close(uint32_t fd);

//释放子进程
static void release_task(task_block_t* task) {
    size_t index = get_index_by_pid(task->pid);
    //释放PCB页
    free_kpages((uint32_t)task, 1);
    all_threads[index] = NULL;
}

static void tell_father(int pid) {
    if (pid) {
        for (int i = 0;i < MAX_THREAD_COUNT;i++) {
            if (!all_threads[i]) continue;
            if (all_threads[i]->pid != pid) continue;
            all_threads[i]->signals.pending |= (1 << (SIGCHLD - 1));
            return;
        }
    }
    panic("[tell father]: No father found");
}

[[noreturn]]void do_exit(int error_code) {
    assert(!get_interrupt_state());
    task_block_t* cur_task = running_task();
    
    //清内存
    release_memory(cur_task, false);

    // task_block_t* parent = get_task_by_pid(cur_task->ppid);
    //把子进程挂在父进程下
    for (size_t i = 1;i < MAX_THREAD_COUNT;i++) {
        task_block_t* task = all_threads[i];
        if (task == NULL) continue;
        if (task->ppid == cur_task->pid) {  //找到一个子进程
            task->ppid = cur_task->ppid;
        }
    }

    //关闭所有文件
    for (int i = 0;i < NR_OPEN; i++) {
        if (cur_task->filp[i]) sys_close(i);
    }
    iput(cur_task->pwd);
    cur_task->pwd = NULL;
    iput(cur_task->root);
    cur_task->root = NULL;

    //TODO 释放exefile的LOAD程序头表

    //设置该进程状态
    cur_task->error_code = error_code;
    cur_task->status = TASK_DIED;
    tell_father(cur_task->ppid);
    //调度
    schedule();

    panic("This place should be unreachable!");
    while(1) ;
}

[[noreturn]] void sys_exit(int error_code)  {
    do_exit((error_code & 0xff) << 8);
}

pid_t sys_waitpid(pid_t pid, int *status, [[unused]] int options) {
    task_block_t* cur_task = running_task();
    task_block_t* child;

    while (1) {
        //先找到对应子进程  
        //跳过idle进程
        for (size_t i = 1;i < MAX_THREAD_COUNT;i++) {
            child = all_threads[i];
            if (child == NULL) continue;
            if (child->ppid != cur_task->pid) continue;
            //不是指定子进程
            if (pid != -1 && child->pid != pid) continue;
            //找到了一个子进程
            if (child->status == TASK_DIED) {
                //如果子进程已经退出
                pid_t child_pid = child->pid;
                *status = child->error_code;
                release_task(child);
                return child_pid;
            }
        }
        task_block(TASK_WAITING);   //进入等待状态, 等待子进程唤醒
    }
}