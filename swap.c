#include "types.h"
#include "defs.h"
#include "stat.h"
#include "mmu.h"
#include "fs.h"
#include "fcntl.h"
#include "memlayout.h"
#include "swap.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "file.h"

void swapinit()
{
// :)
}

// copy of walkpgdir from vm.c
pte_t*
va2pte(const char* va, pde_t* pgdir, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

struct syspages swappg = {0}; // Struct to track all user-allocated pages for swap
struct swaplayout swapbytemap = {0}; // Bytemap to find where to put page in file

// add page to global page list
int
addtopglist(char* va, pde_t* pgdir)
{
  struct page pg = {0};
  pg.va = va;
  pg.pgdir = pgdir;
  pg.swap_index = 0;
  pg.used = 1;
  for(int i = 0; i < MAXSYSPAGES; ++i){
    if(swappg.pages[i].used != 0) // find free space
      continue;
    swappg.pages[i] = pg;
    break;
  }
  cprintf("adding page %x from page table at 0x%x\n", va, pgdir);
  return 1;
}

// remove from global page list
// removes from swap file if page is on swap
int
rmfrompglist(const char* va, pde_t* pgdir)
{
  for(int i = 0; i < MAXSYSPAGES; ++i){
    struct page* pg = &swappg.pages[i];
    if(pg->pgdir == pgdir && pg->va == va) {
      cprintf("removed page 0x%x from page table at 0x%x  at phys addr 0x%x\n",
              va, pgdir, P2V(PTE_ADDR(*va2pte(va, pgdir, 0))));
      pg->used = 0;
      swapbytemap.pg[pg->swap_index] = 0;
      return 1;
    }
  }
  return 0;
}

// page selection algorithm
// first tries pages without both accessed and dirty bits
// then without at least one
// then any other pages
struct page*
selpgtoswap()
{
  struct page* pg;
  for(int i = 0; i < MAXSYSPAGES; ++i){
    pg = &swappg.pages[i];
    if(pg->swap_index)
      continue;
    pte_t *pa = va2pte(pg->va, pg->pgdir, 0);

    if(*pa & PTE_U){
      if(!(*pa & PTE_D) && !(*pa & PTE_A))
        return pg;
    }
  }

  for(int i = 0; i < MAXSYSPAGES; ++i){
    pg = &swappg.pages[i];
    if(pg->swap_index)
      continue;
    pte_t *pa = va2pte(pg->va, pg->pgdir, 0);

    if(*pa & PTE_U){
      if(!(*pa & PTE_D) || !(*pa & PTE_A))
        return pg;
    }
  }

  for(int i = 0; i < MAXSYSPAGES; ++i){
    pg = &swappg.pages[i];
    if(pg->swap_index)
      continue;
    pte_t *pa = va2pte(pg->va, pg->pgdir, 0);

    if(*pa & PTE_U){
        return pg;
    }
  }

  return 0;
}

// select and swap page to disk
int
swaptodisk()
{
  struct page *pg = selpgtoswap();

  pte_t *pa = va2pte(pg->va, pg->pgdir, 0);

  for(int i = 1; i < MAXSWAPPAGES; ++i){
    if(swapbytemap.pg[i] != 0)
      continue;

    swapbytemap.pg[i] = 1;
    *pa |= PTE_S; // set swapped bit
    *pa &= ~PTE_P; // remove present bit so that dereferencing would cause pagefault

    pg->swap_index = i;

    begin_op();

    struct inode* swapi = namei("/.swap");

    ilock(swapi);
    if(writei(swapi, (char*)P2V(PTE_ADDR(*pa)), (i-1)*PGSIZE, PGSIZE) != PGSIZE){
      iunlockput(swapi);
      return 0;
    }
    iunlockput(swapi);

    end_op();
    kfree((char*)PTE_ADDR(P2V(*pa)));

    cprintf("swapped page va: 0x%x from table at 0x%x with phys addr 0x%x\n", pg->va, pg->pgdir, P2V(PTE_ADDR(*pa)));
    break;
  }
  return 1;
}

// retrieve page from swap back into memory
int
loadfromswap(const char* va, const pde_t* pgdir)
{
  struct page *pg = 0;

  for(int i = 0; i < MAXSYSPAGES; ++i) {
    if (swappg.pages[i].pgdir == pgdir && swappg.pages[i].va == (char*) PTE_ADDR(va)) {
      pg = &swappg.pages[i];
      break;
    }
  }

  char* mem = kalloc(); // allocate new page in memory to load from disk

  pte_t *pa = va2pte(pg->va, pg->pgdir, 0);
  uint pflags = PTE_FLAGS(*pa);
  *pa = (pte_t)V2P(mem) | PTE_P | pflags; // update physical address
  *pa &= ~PTE_S; // remove swapped bit

  begin_op();

  struct inode *swapi = namei("/.swap");
  ilock(swapi);
  if(readi(swapi, mem, (pg->swap_index-1)*PGSIZE, PGSIZE) != PGSIZE){
    iunlockput(swapi);
    return 0;
  }

  iunlockput(swapi);

  end_op();

  swapbytemap.pg[pg->swap_index] = 0; // return space to swap file, no cleaning tho
  cprintf("loaded page va: 0x%x\n", pg->va);

  return 1;
}
