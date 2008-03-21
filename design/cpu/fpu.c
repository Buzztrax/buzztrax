/* determine cpuload
 * gcc -Wall -g fpu.c -o fpu
 */

#include <stdio.h>
#include <unistd.h>
#include <fpu_control.h>

int main (int argc, char **argv)
{
  fpu_control_t cw;
  
  _FPU_GETCW(cw);
  printf("cw register: 0x%08x\n",cw);
  
  printf("PC: Precision control: 0x%0x\n",(cw>>8)&0x3);
  if(cw&_FPU_EXTENDED) puts("  round to extended precision");
  else if(cw&_FPU_DOUBLE) puts("  round to double precision");
  else puts("  round to single precision");

  printf("RC: Rounding control: 0x%0x\n",(cw>>10)&0x3);
  if(cw&_FPU_RC_ZERO) puts("  rounding toward zero");
  else if(cw&_FPU_RC_DOWN) puts("  rounding down (toward -infinity)");
  else if(cw&_FPU_RC_UP) puts("  rounding up (toward +infinity)");
  else puts("  rounding to nearest");
  
  return(0);
}

