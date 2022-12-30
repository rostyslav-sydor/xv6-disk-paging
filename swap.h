#define MAXSWAPPAGES 4096
#define MAXSYSPAGES 4096

struct swaplayout {
  char pg[MAXSWAPPAGES];
};

struct page {
  char* va;
  pde_t* pgdir;
  ushort swap_index;
  char used;
};

struct syspages {
  struct page pages[MAXSYSPAGES];
};

