#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

int main (void)
{
        int i = 0;
        struct timeval *start, *end;
	unsigned long tz;
	int repeat = 20;
	unsigned long sec;
	long usec;

	printf("Starting test program.\n");	

        start = malloc((sizeof *start) * repeat);
	end   = malloc((sizeof *end) * repeat);

	for (i = 0; i < repeat; i++)
        {
           gettimeofday(&start[i],&tz);
	   /* Run a loop of some sort */
	   system("ping -c 2 -s 128 10.1.10.12");
	   gettimeofday(&end[i],&tz);
	}
	printf ("------------------------\n");

        for (i=0; i < repeat; i++)
	{
	   printf ("%d.%d -- %d.%d\n",start[i].tv_sec,start[i].tv_usec,end[i].tv_sec, end[i].tv_usec);
	   sec  = 0;
	   usec = 0;
	   sec = end[i].tv_sec - start[i].tv_sec;
	   usec = end[i].tv_usec - start[i].tv_usec;
	   if(usec < 0) {usec += 1000000; sec -= 1; }
	   printf ("+%d.%03d %d\n",sec,usec/1000,i);
	}

	free (start);
	free (end);

	return 0;
}
