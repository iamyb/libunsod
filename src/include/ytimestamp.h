#ifndef YTS_H
#define YTS_H

//#include <sys/time.h>
//#include <stdio.h>
//#define YTS_ENABLE
#ifdef YTS_ENABLE

extern void store_timestamp(const char* const func);
extern void print_latprof();
#ifndef __cplusplus 
#define PRINT_TIMESTAMP                                      \
do                                                                 \
{                                                                  \
	store_timestamp(__FUNCTION__); \
}while(0);

#else

#define PRINT_TIMESTAMP                                      \
do                                                                 \
{                                                                  \
	store_timestamp("C++"); \
}while(0);
#endif

#define PRINT_LATPROF \
do\
{\
	print_latprof();\
}while(0);

#else
#define PRINT_TIMESTAMP 
#define PRINT_LATPROF 
#endif

#endif