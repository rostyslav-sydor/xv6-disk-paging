#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
  if(argc < 3){
    printf(2, "mv: not enough arguments\n");
    exit();
  }

  if(argv[1] == argv[2]){
    printf(2, "filenames are identical\n");
    exit();
  }

  int a;
  if(!(a = link(argv[1], argv[2]))){
//    printf(1, "success\n");
    unlink(argv[1]);
  } else {
    printf(2, "mv: error creating link\n");
  } 

  exit();
}
