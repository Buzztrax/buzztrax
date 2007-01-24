/* determine cpuload
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` cpuload.c -o cpuload
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>

#include <gst/gst.h>

int main (int argc, char **argv)
{
  int k,l;
  float i, j, r, x, y;
  struct rusage ru;
  struct tms tms;
  GstClockTime tuser,tsys,tbeg,tend,treal;
  struct timeval beg,end;
  long clk;
  int cpuload;
  
  gettimeofday(&beg,NULL);
  tbeg=GST_TIMEVAL_TO_TIME(beg);
  clk=sysconf(_SC_CLK_TCK);
  
  for(l=0;l<100;l++) {
    y = -16; k = 0;
    while (y++ < 15) {
      //puts ("");
      for (x = 0; x++ < 84; ) {
        //putchar (" .:-;!/>)|&IH%*#"[k & 15]);
        for (i = k = r = 0; j = r * r - i * i - 2 + x / 25, i = 2 * r * i + y / 10, j * j + i * i < 11 && k++ < 111; r = j);
  	  }
  	}
  	//usleep(50);
  	gettimeofday(&end,NULL);
  	tend=GST_TIMEVAL_TO_TIME(end);
  	treal=GST_CLOCK_DIFF(tbeg,tend);
  	// version 1
  	getrusage(RUSAGE_SELF,&ru);
  	tuser=GST_TIMEVAL_TO_TIME(ru.ru_utime);
  	tsys=GST_TIMEVAL_TO_TIME(ru.ru_stime);
  	// version 2
  	times(&tms);
  	// percentage
  	cpuload=(int)gst_util_uint64_scale(tuser+tsys,G_GINT64_CONSTANT(100),treal);
  	printf("real %"GST_TIME_FORMAT", user %"GST_TIME_FORMAT", sys %"GST_TIME_FORMAT" => cpuload %d\n",GST_TIME_ARGS(treal),GST_TIME_ARGS(tuser),GST_TIME_ARGS(tsys),cpuload);
  	//printf("user %6.4lf, sys %6.4lf\n",(double)tms.tms_utime/(double)clk,(double)tms.tms_stime/(double)clk);
  }
  return(0);
}
