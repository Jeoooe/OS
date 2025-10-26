/*
    文件名相关的实现
*/

#include <os.h>
#include <assert.h>
#include <thread.h>
#include <fs/fs.h>
#include <fs/stat.h>
#include <fs/fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define DIR_ENTRIES_PER_BLOCK BLOCK_SIZE / 32

//访问模式宏
#define  ACC_MODE(x) ("\004\002\006\377"[(x) & O_ACCMODE])

#define MAY_EXEC 1
#define MAY_WRITE 2
#define MAY_READ 4


//检查文件访问权限
static bool permission(inode_t* inode, int mask) {
    int mode = inode->mode;
    task_block_t* cur_task = running_task();
    //不可读写被删除的文件
    //进程有效用户id与组id
    if (inode->dev  && !inode->nlinks) {
        return false;
    }
    else if (cur_task->euid == inode->uid) {
        mode >>= 6;
    }
    else if (cur_task->gid == inode->gid) {
        mode >>= 3;
    }

    //访问权限与屏蔽码相同或者超级用户
    if (((mode & mask & 0007) == mask) ||  cur_task->uid == UID_KERNEL) {
        return true;
    }
    return false;
}

/// @brief 在目录inode中查找对应名字的目录项
/// @param dir 目录指针
/// @param name 文件名
/// @param namelen 文件名长度, 不要超过`MAX_FILE_NAME_LEN`
/// @param res_dir 返回的目录项
/// @return 目录项高速缓冲区指针
static buffer_t* find_entry(inode_t** dir, const char* name, int namelen, dir_entry_t** res_dir) {
    if (namelen == 0 || namelen > MAX_FILE_NAME_LEN) {
        return NULL;
    }
    
    int block, entries;
    buffer_t* bh;
    super_block_t* sb;
    task_block_t* current = running_task();
    entries = (*dir)->size / (sizeof(dir_entry_t));
    *res_dir = NULL;

    //.. 伪根处理
    if (namelen == 2 && (name[0] == '.' && name[1] == '.')) {
        if ((*dir) == current->root) namelen = 1;
        else if ((*dir)->i_num == ROOT_INODE) {
            //已经在根目录
            //则其父目录应该是该根目录对应的块设备挂载的inode
            sb = get_super((*dir)->dev);
            if (sb->imount) {
                iput(*dir);
                (*dir) = sb->imount;
                (*dir)->count ++;
            }
        }
    }

    //处理正常的操作
    block = (*dir)->zones[0];
    if (!block) return NULL;
    bh = bread((*dir)->dev, block);
    if (!bh) return NULL;
    
    //搜索目录项
    int i = 0;
    dir_entry_t* de;
    de = (dir_entry_t*)bh->data;
    while (i < entries) {
        //搜索完了一个数据块
        if ((char*) de >= BLOCK_SIZE + bh->data) {
            brelse(bh);
            bh = NULL;
            if (
                !(block = get_block(*dir, i / DIR_ENTRIES_PER_BLOCK)) ||
                !(bh = bread((*dir)->dev, block))
            ) {
                //这一块数据块读不出来
                //直接跳过
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (dir_entry_t*)bh->data;
        }
        if (strncmp(name, de->filename, namelen) == 0) {   //名字相同
            *res_dir = de;
            return bh;
        }
        de++;
        i++;
    }
    //都搜索完了没找到
    brelse(bh);
    return NULL;
}

//添加一个目录项
static buffer_t* add_entry(inode_t* dir, const char* name, int namelen, dir_entry_t** res_dir) {
    int block, i;
    buffer_t* bh;
    dir_entry_t* de;

    *res_dir = NULL;
    if (namelen > MAX_FILE_NAME_LEN) return NULL;
    
    if (!namelen) return NULL;  //没有名字
    if (!(block = dir->zones[0])) return NULL;  //目录没有数据
    if (!(bh = bread(dir->dev, block))) return NULL;
    
    i = 0;
    de = (dir_entry_t*)bh->data;
    while (1) {
        if ((char*)de >= BLOCK_SIZE + bh->data) {
            brelse(bh);
            bh = NULL;
            block = create_block(dir, i / DIR_ENTRIES_PER_BLOCK);
            if (!block) return NULL;
            if (!(bh = bread(dir->dev, block))) {
                i += DIR_ENTRIES_PER_BLOCK;
                continue;
            }
            de = (dir_entry_t*)bh->data;
        }
        //i * 目录项大小超出inode指出的size, 说明没有因为删除文件留下的空位
        if (i * sizeof(dir_entry_t) >= dir->size) {
            de->i_no = 0;
            dir->size = (i + 1) * sizeof(dir_entry_t);
            dir->dirty = 1;
            dir->c_time = CURRENT_TIME;
        }
        //当前de为空, 则找到一个空闲目录项
        if (!de->i_no) {
            dir->mtime = CURRENT_TIME;
            //复制文件名
            strncpy(de->filename, name, namelen);
            bh->dirty = 1;
            *res_dir = de;
            return bh;
        }
        de++;
        i++;
    }
}

// 根据路径名搜索最顶端目录的inode
// 失败返回NULL
static inode_t* get_dir(const char* pathname) {
    task_block_t* current = running_task();

    if (!current->root || !current->root->count) {
        panic("No root inode");
    }
    if (!current->pwd || !current->pwd->count) {
        panic("No cwd inode");
    }
    char c;
    const char* thisname;
    inode_t* inode;
    buffer_t* bh;
    int namelen, inr, idev;
    dir_entry_t* de;

    //第一个字符是/, 绝对路径
    if ((c = *pathname) == '/') {
        inode = current->root;
        pathname++;
    }
    else if (c) {
        inode = current->pwd;
    }
    else return NULL;   //空路径名

    inode->count++;
    while (1) {
        thisname = pathname;
        if (!S_ISDIR(inode->mode) || !permission(inode, MAY_EXEC)) {
            iput(inode);
            return NULL;
        }
        //分离一个目录名
        for (namelen = 0; (c = *pathname++) && (c != '/');namelen++)
            ;
        //最后一个名字是一个目录名,但是没有加上'/',则直接返回上一级
        //例如 /usr/bin 会返回 usr
        if (!c)
            return inode;
        if (!(bh = find_entry(&inode, thisname, namelen, &de))) {
            iput(inode);
            return NULL;
        }
        inr = de->i_no;
        idev = inode->dev;
        brelse(bh);
        iput(inode);
        if (!(inode = iget(idev, inr))) return NULL;
    }
}


/// @brief 返回指定目录名的顶层目录inode, 及最顶层目录名. 
/// 若最后一个符号是'/' 例如/usr/bin/, 那么返回空目录名, 但inode为bin
/// @param pathname 路径
/// @param namelen 路径名长度
/// @param name 最顶层目录名
/// @return 最顶层目录inode, 失败为 `NULL`
static inode_t* dir_namei(const char* pathname, int* namelen, const char **name) {
    char c;
    const char* basename;
    inode_t* dir;

    //取得最顶层目录inode
    if (!(dir = get_dir(pathname))) return NULL;
    basename = pathname;
    while ((c = *pathname++)) {
        if (c == '/') {
            basename = pathname;
        }
    }
    *namelen = pathname - basename - 1;
    *name = basename;
    return dir;
}


inode_t* namei(const char* pathname) {
    const char* basename;

    int nr, dev, namelen;
    inode_t* dir;
    buffer_t* bh;
    dir_entry_t* de;

    if (!(dir = dir_namei(pathname, &namelen, &basename))) 
        return NULL;
    if (!namelen) // 最后是个目录 ,例如/usr/, 直接返回usr
        return dir;
    
    bh = find_entry(&dir, basename, namelen, &de);
    if (!bh) {
        iput(dir);
        return NULL;
    }
    nr = de->i_no;
    dev = dir->dev;
    brelse(bh);
    iput(dir);
    dir = iget(dev, nr);
    if (dir) {
        dir->a_time = CURRENT_TIME;
        dir->dirty = 1;
    }
    return dir;
}

/// @brief `open()`使用的namei函数
/// @param pathname 文件名
/// @param flag 打开标志, 见 `fcntl.h`
/// @param mode 若创建文件, 则指定文件的许可属性 见`stat.h`
/// @param res_inode 对应文件的inode
/// @return 成功返回0, 否则返回出错码
int open_namei(const char* pathname, int flag, int mode, inode_t** res_inode) {
    const char* basename;
    int nr, dev, namelen;
    inode_t* dir, *inode;
    buffer_t* bh;
    dir_entry_t* de;
    task_block_t* current = running_task();

    //访问模式的处理
    //如果只读 但是有截断标志, 则要添加写标志
    if ((flag & O_TRUNC) && !(flag & O_ACCMODE)) {
        flag |= O_WRONLY;
    }
    //然后用当前进程的文件访问许可码屏蔽给定模式
    mode &= ~current->umask & 0777;
    mode |= I_REGULAR;  //打开文件不存在时默认的新文件属性
    
    //寻找inode
    dir = dir_namei(pathname, &namelen, &basename);
    if (!dir) { //路径不存在
        return -ENOENT;
    }
    //如果目录名为0 (/usr/这种), 如果操作没有写,创建和文件长度截0,则是在打开一个目录名文件操作
    if (!namelen) {
        if (!(flag & (O_ACCMODE | O_CREAT | O_TRUNC))) {
            *res_inode = dir;
            return 0;
        }
        iput(dir);
        return -EISDIR;
    }
    //根据最顶层目录名, 查找路径最后文件名对应的de
    bh = find_entry(&dir, basename, namelen, &de);
    if (!bh) {
        if (!(flag & O_CREAT)) {
            iput(dir);
            return -ENOENT;
        }
        if (!permission(dir, MAY_WRITE)) {
            iput(dir);
            return -EACCES;
        }
        //确定是创建操作并具备了写权限
        //申请新inode, 失败则放回并返回没有空间
        inode = new_inode(dir->dev);
        if (!inode) {
            iput(dir);
            return -ENOSPC; //没有空间
        }
        inode->uid = current->euid;
        inode->mode = mode;
        inode->dirty = 1;
        bh = add_entry(dir, basename, namelen, &de);
        //创建新目录失败
        if (!bh) {
            inode->nlinks --;
            iput(inode);
            iput(dir);
            return -ENOSPC;
        }
        de->i_no = inode->i_num;
        bh->dirty = 1;
        brelse(bh);
        iput(dir);
        *res_inode = inode;
        return 0;
    }
    //取出目录项成功
    nr = de->i_no;
    dev = dir->dev;
    brelse(bh);
    iput(dir);

    //如果独占操作置位, 但是文件已存在, 则返回错误码
    if (flag & O_EXCL) {
        return -EEXIST;
    }

    //读取inode内容 
    inode = iget(dev, nr);
    if (!inode) {
        return -EACCES;
    }
    //如果是目录, 但是访问模式是写或者读写, 或者没有权限, 则返回错误
    if ((S_ISDIR(inode->mode) && (flag & O_ACCMODE)) ||
     !permission(inode, ACC_MODE(flag))) {
        iput(inode);
        return -EPERM;
    }
    //更新inode访问时间
    inode->a_time = CURRENT_TIME;
    //如果有截零标志则截零
    if (flag & O_TRUNC) {
        truncate(inode);
    }
    *res_inode = inode;
    return 0;
}