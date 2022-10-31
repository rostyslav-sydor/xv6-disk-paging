#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char buf[512];

int main(int argc, char* argv[]){
  if(argc < 3){
    printf(2, "cp: not enough arguments\n");
    exit();
  }
  
  if(strcmp(argv[1], argv[2])==0){
    printf(2, "cp: cannot copy to the same file\n");
    exit();
  }

  int orig, copy;
  if((orig = open(argv[1], O_RDONLY)) < 0){
    printf(2, "cp: cannot open %s\n", argv[1]);
    exit();
  }

  if((copy = open(argv[2], O_WRONLY | O_CREATE)) < 0){
    printf(2, "cp: cannot open %s\n", argv[1]);
    exit();
  }


  int n;
  while((n = read(orig, buf, sizeof(buf))) > 0) {
   if(write(copy, buf, n) != n){
     printf(1, "cp: write error\n");
     exit();
   }
   if(n < 0){
     printf(1, "cp: read error\n");
     exit();
   }
  }
  close(orig);
  close(copy);
  
  exit();
}
