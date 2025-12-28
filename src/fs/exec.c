#include <fs/fs.h>



int sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    /* 1. 检验是否是elf文件
     * 2. 读取文件头, 以及各段信息
     * 3. 加载用户栈, 参数和环境变量
     * 4. 修改进程信息
     * 5. 返回到执行区域
     * 
     * 目前只实现静态链接程序
    */    
}