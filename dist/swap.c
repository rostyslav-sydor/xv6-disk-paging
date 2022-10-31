#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"



//Swaps

int
createswap(struct proc* p){

    char path[16] = "/.swap";

    // mini itoa
    int i = 0, n = p->pid;
    do{
        path[6+i++] = (n % 10) + '0';
    } while((n /= 10) > 0);

    while(i < 16)
        path[i++] = 0;

    begin_op();
    struct inode* node = create(path, T_FILE, 0, 0);
    if(node == 0){
        end_op();
        return -1;
    }
    struct file* f;
    if((f = filealloc()) == 0){
        iunlockput(node);
        end_op();
        return -1;
    }
    iunlock(node);
    end_op();

    p->swap = f;

    p->swap->type = FD_INODE;
    p->swap->ip = node;
    p->swap->off = 0;
    p->swap->readable = 1;
    p->swap->writable = 1;
    return 0;
}

int
deleteswap(struct proc* p){
    struct inode* ip = p->swap->ip;
    begin_op();
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    end_op();
    ip = 0;
    return 0;
}

int
readswap(struct proc* p, char* buf, int size, uint offset){
    p->swap->off = offset;
    fileread(p->swap, buf, size);
    return 0;
}

int
writeswap(struct proc* p, char* buf, int size, uint offset){
    p->swap->off = offset;
    filewrite(p->swap, buf, size);
    return 0;
}
