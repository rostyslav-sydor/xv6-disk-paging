#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
  int *nums[1000];
  for(int i = 0; i < 1000; ++i){
    nums[i] = malloc(4);
    *(nums[i]) = 4;
  }
  for(int i = 0; i < 1000; ++i){
    free(nums[i]);
  }


//  char* ptrs[1001];
//  for(int i = 0; i < 1000; ++i){
//    ptrs[i] = malloc(1);
//    *ptrs[i] = '1';
//  }
//  for(int i = 0; i < 1000; ++i){
//    free(ptrs[i]);
//  }

  exit();
}