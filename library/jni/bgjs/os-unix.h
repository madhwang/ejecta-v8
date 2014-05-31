#ifndef __OS_UNIX_H
#define __OS_UNIX_H	1

#include <stdio.h>

#define LOG_DEBUG	1
#define LOG_INFO	2
#define LOG_ERROR	3



#define  LOGD(...)  printf("debug: %s: " ,LOG_TAG); printf (__VA_ARGS__); printf ("\n")
#define  LOGI(...)  printf("Info : %s: " ,LOG_TAG); printf (__VA_ARGS__); printf ("\n")
#define  LOGE(...)  printf("_ERR_: %s: " ,LOG_TAG); printf (__VA_ARGS__); printf ("\n")

#define LOG(LOG_LEVEL, TAG, ...)  printf ("%s: %s: ", LOG_LEVEL == 1 ? "debug" : (LOG_LEVEL == 2 ? "Info " : "_ERR_"), TAG); printf (__VA_ARGS__); printf ("\n")

#endif
