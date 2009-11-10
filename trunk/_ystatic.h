#ifndef YSTATIC_H
#define YSTATIC_H

#ifndef _MSC_VER
#include "stdint.h"
#endif

// static pool sizes
#define FL_PIT_SIZE 1000
#define FL_CTX_SIZE 100
#define HT_PIT_SIZE 10
#define HT_PIT_GROW_FACTOR 2
#define HT_CTX_SIZE 5
#define HT_CTX_GROW_FACTOR 2
#define HT_CS_COUNT_SIZE 5

#define HT_CS_COUNT_GROW_FACTOR 1




// stat related
#define MAX_LINE_LEN 80
#define MAX_FUNC_NAME_LEN 34
#define MAX_TIME_COLUMN_LEN 12
#define MAX_CALLCOUNT_COLUMN_LEN 8
#define ZIP_RIGHT_MARGIN_LEN 2
#define ZIP_DOT_COUNT 2
#define STAT_SORT_TYPE_MAX 4
#define STAT_SORT_ORDER_MAX 1
#define STAT_SORT_FUNC_NAME 0
#define STAT_SORT_CALL_COUNT 1
#define STAT_SORT_TIME_TOTAL 2
#define STAT_SORT_TIME_SUB 3
#define STAT_SORT_TIME_AVG 4
#define STAT_SORT_ASCENDING 0
#define STAT_SORT_DESCENDING 1
#define STAT_SHOW_ALL -1

#define STAT_CALLCOUNT_COLUMN_IDX MAX_FUNC_NAME_LEN
#define STAT_TTOT_COLUMN_IDX STAT_CALLCOUNT_COLUMN_IDX+MAX_CALLCOUNT_COLUMN_LEN-1
#define STAT_TSUB_COLUMN_IDX STAT_TTOT_COLUMN_IDX+MAX_TIME_COLUMN_LEN
#define STAT_TAVG_COLUMN_IDX STAT_TSUB_COLUMN_IDX+MAX_TIME_COLUMN_LEN
#define STAT_HEADER_STR "\nname                              ccnt   ttot        tsub        tavg"
#define STAT_FOOTER_STR "\nthreads"
#define STAT_FOOTER_STR2 "\nprofiler"
#define STAT_MAX_FOOTSTR_LEN 255 //max. overhead calc. string len

#endif
