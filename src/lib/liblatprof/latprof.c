#include <time.h>
#include <stdio.h>
#include <stdint.h>
#define YTS_NSEC_PER_SEC	(1000ULL * 1000ULL * 1000ULL)
#define MAX_LP_ENTRY 1000
typedef struct s_latprof{
	char func_name[64];
	long unsigned int ts;
	long unsigned int la;
}t_latprof;

static t_latprof data[MAX_LP_ENTRY];
static unsigned int count = 0;
static long unsigned int prev = 0;
static unsigned int wrap = 0;
static unsigned int stop = 0;

void store_timestamp(const char* const func)
{
    struct timespec ts;                                            
	clock_gettime(CLOCK_REALTIME, &ts);                           

	if(stop == 1) return;
	snprintf(&data[count].func_name, 64, "%s", func);
	data[count].ts = ((uint64_t)(ts.tv_sec * YTS_NSEC_PER_SEC + ts.tv_nsec));
	data[count].la = data[count].ts - prev;
	prev = data[count].ts;
	count = count + 1;
	if(count >= MAX_LP_ENTRY)
	{
		wrap = 1;
		count %= MAX_LP_ENTRY;
	}
}

void print_latprof(void)
{	
    int i = 0;
	stop = 1;
	if(wrap){
		for(i = count; i < MAX_LP_ENTRY; i++)
			printf("%-50s ts: %lu  diff: %lu \n", data[i].func_name, data[i].ts, data[i].la);
	}
	
	for(i = 0; i < count; i++)
		printf("%-50s ts: %lu  diff: %lu \n", data[i].func_name, data[i].ts, data[i].la);		
}