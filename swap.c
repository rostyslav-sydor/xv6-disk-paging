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
#include "syscall.h"
#include "memlayout.h"


void
itoamini(char* path, int n){
    int i = 6;
    do{
        path[i++] = (n % 10) + '0';
    } while((n /= 10) > 0);

    while(i < 16)
        path[i++] = 0;

}

int
createswap(struct proc* p){

    char path[16] = "/.swap";
    itoamini(path, p->pid);

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
    struct inode *ip, *dp;
    struct dirent de;
    char name[DIRSIZ], path[16] = "/.swap";
    itoamini(path, p->pid);
    uint off;

    fileclose(p->swap);

    begin_op();
    if((dp = nameiparent(path, name)) == 0){
        end_op();
        return -1;
    }

    ilock(dp);

    // Cannot unlink "." or "..".
    if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
        goto bad;

    if((ip = dirlookup(dp, name, &off)) == 0)
        goto bad;
    ilock(ip);

    if(ip->nlink < 1)
        panic("unlink: nlink < 1");

    memset(&de, 0, sizeof(de));
    if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
        panic("unlink: writei");
    if(ip->type == T_DIR){
        dp->nlink--;
        iupdate(dp);
    }
    iunlockput(dp);

    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);

    end_op();

    return 0;

    bad:
    iunlockput(dp);
    end_op();
    return -1;

}

int
readswap(struct proc* p, char* buf, int size, uint offset){
    p->swap->off = offset;
    return fileread(p->swap, buf, size);
}

int
writeswap(struct proc* p, char* buf, int size, uint offset){
    p->swap->off = offset;
    return filewrite(p->swap, buf, size);
}

int
swaptodisk(struct proc* p, uint index){
  char *pg = p->pages[index].addr;
  *pg &= ~PTE_P;
  *pg |= PTE_S;
  if(!writeswap(p, (char*) V2P(PTE_ADDR(pg)), PGSIZE, p->pages[index].diskoffset))
    panic("swaptodisk: error writing page to swap.");
  return 0;
}

int
loadfromdisk(struct proc* p, uint index){
  return 0;
}