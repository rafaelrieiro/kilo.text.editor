
#ifndef  _H_COMMON_
#include "common.h"
#endif

#ifndef _HLLINES_H_
#define _HLLINES_H_

#define INIT_SIZE_HL_LINES 20

/*
typedef struct {
      unsigned int line;
      unsigned int lbefore;
      unsigned int lafter;
} hlLnAttr;

typedef struct {

    hlLnAttr *list;
    unsigned int len;
    unsigned int size;

} hlLnList;
*/

void hlLnListInit(hlLnList *hlList);
void hlLnListClose(hlLnList *hlList);
void hlLnListInsert(hlLnList *hlList, unsigned int line , unsigned int lbefore, unsigned int lafter );
void hlLnListDeleteLine(hlLnList *hlList,unsigned int line);
void hlLnListResetBefore(hlLnList *hlList,unsigned int line);
void hlLnListResetAfter(hlLnList *hlList,unsigned int line);
void hlLnListGetLineRange(hlLnList *hlist, unsigned int line, unsigned int *startLine, unsigned int *numberOfLines);
int hlLnListGetAfter(hlLnList *hlList,unsigned int line);
int hlLnListGetBefore(hlLnList *hlList,unsigned int line);

#endif
