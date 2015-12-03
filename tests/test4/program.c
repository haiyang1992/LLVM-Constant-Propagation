#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

CATData foo2 (CATData d){
   CATData d2 = CAT_create_signed_value(10);
   return d;
}

int64_t foo (CATData d){
   CATData d2 = CAT_create_signed_value(10);
   CATData d3 = foo2(d);
   return CAT_get_signed_value(d);
}

int main (int argc, char *argv[]){
  CATData p = CAT_create_signed_value(5);
  p = foo2(p);
  int64_t v = foo(p);
  printf("H1: 	X    = %ld\n",v);  
}

