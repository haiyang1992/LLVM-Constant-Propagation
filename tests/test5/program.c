#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int64_t foo2 (CATData d){
   CATData d2 = CAT_create_signed_value(10);
   return CAT_get_signed_value(d);
}

int64_t foo (CATData d){
   CATData d2 = CAT_create_signed_value(10);
   return foo2(d);
}

int main (int argc, char *argv[]){
  CATData p = CAT_create_signed_value(5);
  int64_t v = foo(p);
  printf("H1: 	X    = %ld\n",v);  
}

