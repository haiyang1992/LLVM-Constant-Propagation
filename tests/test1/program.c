#include <stdio.h>
#include <stdlib.h>
#include <CAT.h>

int64_t foo (CATData d, int n){
   return CAT_get_signed_value(d) + n;
}

int main (int argc, char *argv[]){
  int64_t v;
  if (argc > 10){
     CATData p = CAT_create_signed_value(5);
     v = foo(p, 3);
  } else {
    CATData p = CAT_create_signed_value(2);
     v = foo(p, 10);
  }

}