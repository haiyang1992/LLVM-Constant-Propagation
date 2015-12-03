#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int64_t foo2 (CATData d){
   CATData d2 = CAT_create_signed_value(10);
   return CAT_get_signed_value(d);;
}

int main (int argc, char *argv[]){
  CATData p = CAT_create_signed_value(5);
  int64_t v = foo2(p);
  printf("H1: 	X    = %ld\n",v);
  
  CATData p2 = CAT_create_signed_value(6);
  int64_t v2 = foo2(p2);
  printf("H1: 	X    = %ld\n",v2);
}

