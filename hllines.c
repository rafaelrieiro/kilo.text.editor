#include <stdio.h>
#include <stdlib.h>

#include "hllines.h"

static void hlLnListUpdate(hlLnList *hlList, unsigned int pos, unsigned int lbefore, unsigned int lafter);
static void hlLnListPrint(hlLnList *hlList);
static void hlLnListDelete(hlLnList *hlList, unsigned int pos);
static int  hlLnListFind(hlLnList *hlList, unsigned int line, unsigned int *pos);

void hlLnListInit(hlLnList *hlList)
{
    //hlList->list = malloc(INIT_SIZE_HL_LINES * sizeof(__int32_t));
    hlList->list = malloc(INIT_SIZE_HL_LINES * sizeof(hlLnAttr));
    hlList->len = 0;
    hlList->size = INIT_SIZE_HL_LINES;
}

void hlLnListClose(hlLnList *hlList)
{
  free(hlList->list);
}
static void hlLnListUpdate(hlLnList *hlList, unsigned int pos, unsigned int lbefore, unsigned int lafter)
{
   if ( lbefore > 0 ) hlList->list[pos].lbefore = lbefore;
   if ( lafter  > 0 ) hlList->list[pos].lafter  = lafter; 	
   //hlList->list[pos].lbefore = lbefore;
   //hlList->list[pos].lafter  = lafter;
}

void hlLnListInsert(hlLnList *hlList, unsigned int line , unsigned int lbefore, unsigned int lafter )
{
   unsigned int i;
   unsigned int pos = 0;
   
   //check if item is in list and position to insert
   for (i=0;i<hlList->len;i++){
     if (line == hlList->list[i].line) {
         hlLnListUpdate(hlList,i,lbefore,lafter);
         return;
     } 
     if ( line > hlList->list[i].line) pos++;
   }
     // if list full increase size
     if (hlList->size == hlList->len) {
     	
       hlList->list = realloc(hlList->list,(hlList->size + 10)*sizeof(hlLnAttr));
       hlList->size += 10;
     }
     //reallocate other elements in ordered list
     for (i = hlList->len; i>pos;i--){
        hlList->list[i].line=hlList->list[i-1].line;
        hlList->list[i].lbefore=hlList->list[i-1].lbefore;
        hlList->list[i].lafter=hlList->list[i-1].lafter;
     }  
     //insert item
     hlList->list[pos].line=line;
     hlList->list[pos].lbefore=lbefore;
     hlList->list[pos].lafter=lafter;
     hlList->len++;
}

static void hlLnListPrint(hlLnList *hlList)
{
    unsigned int i;
    
    printf("Line list:\n");

    for (i=0;i<hlList->len;i++){
     
       printf("Line Number: %d, lBef: %d, lAft: %d\n",
       hlList->list[i].line,hlList->list[i].lbefore,hlList->list[i].lafter);
    }
 
}

static void hlLnListDelete(hlLnList *hlList, unsigned int pos)
{
   unsigned int i;
      
   for (i = pos; i < hlList->len - 1;i++){
            hlList->list[i].line=hlList->list[i+1].line;
            hlList->list[i].lbefore=hlList->list[i+1].lbefore;
            hlList->list[i].lafter=hlList->list[i+1].lafter;
         }  
           
   hlList->len--;
}

void hlLnListDeleteLine(hlLnList *hlList,unsigned int line)
{
   unsigned int pos;
   int found = 0;
   
   //check if item is in list and position to reset lbefore
   for (pos=0;pos<hlList->len;pos++){
     if (line == hlList->list[pos].line) {
       found = 1;
       break;
     }
       
   }
   if (found == 0) return;
   hlLnListDelete(hlList,pos);
}

int hlLnListGetBefore(hlLnList *hlList,unsigned int line)
{
   unsigned int pos;
   int found = 0;
   
   //check if item is in list and position to reset lbefore
   for (pos=0;pos<hlList->len;pos++){
     if (line == hlList->list[pos].line) {
       found = 1;
       break;
     }
       
   }
   if (found == 0) return 0;
   return (hlList[pos].list->lbefore);
}

int hlLnListGetAfter(hlLnList *hlList,unsigned int line)
{
   unsigned int pos;
   int found = 0;
   
   //check if item is in list and position to getafter
   for (pos=0;pos<hlList->len;pos++){
     if (line == hlList->list[pos].line) {
       found = 1;
       break;
     }
       
   }
   if (found == 0) return 0;
   //return (hlList[pos].list->lafter);
   return (hlList->list[pos].lafter);
}
void hlLnListResetBefore(hlLnList *hlList,unsigned int line)
{
   unsigned int pos;
   int found = 0;
   
   //check if item is in list and position to reset lbefore
   for (pos=0;pos<hlList->len;pos++){
     if (line == hlList->list[pos].line) {
       found = 1;
       break;
     }
       
   }
   if (found == 0) return;
   //hlList[pos].list->lbefore = 0;
   hlList->list[pos].lbefore = 0;
   if (hlList->list[pos].lafter == 0) hlLnListDelete(hlList,pos);
}

void hlLnListResetAfter(hlLnList *hlList,unsigned int line)
{
   unsigned int pos;
   int found = 0;
   
   //check if item is in list and position to reset lafter
   for (pos=0;pos<hlList->len;pos++){
     if (line == hlList->list[pos].line) {
       found = 1;
       break;
     }
       
   }
   if (found == 0) return;
   //hlList[pos].list->lafter = 0;
   hlList->list[pos].lafter = 0;
   if (hlList->list[pos].lbefore == 0) hlLnListDelete(hlList,pos);
}

void hlLnListGetLineRange(hlLnList *hlist, unsigned int line, unsigned int *startLine, unsigned int *numberOfLines)
{
   int i;
   int found = 0;
   unsigned int pos=0;

   if (hlist->len < 2) {
     *startLine = line;
     *numberOfLines = 1; 
     return;
   }
   
   for (i=0;i<hlist->len - 1;i++){
     if ( (line <= hlist->list[i+1].line )  && ( line >= hlist->list[i].line ) ) {
       found = 1;
       break;
     }
   }
   if (found == 0) {
     *startLine = line;
     *numberOfLines = 1; 
     return;
   }

   
   if ( line == hlist->list[i].line ) { // matches line at left of interval

     if ( hlist->list[i].lbefore == 0 && hlist->list[i].lafter != 0 ) {
       *startLine = hlist->list[i].line;
       *numberOfLines = hlist->list[i].lafter + 1;
       // look for previous lines
       i--;
         while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
           *startLine -= hlist->list[i].lbefore;
           *numberOfLines += hlist->list[i].lbefore;
           i--;	
         }
       // end
     }
     else if ( hlist->list[i].lbefore != 0 && hlist->list[i].lafter == 0){
       *startLine     = (line - hlist->list[i].lbefore);
       *numberOfLines = hlist->list[i].lbefore;
       // look for previous lines
         i--;
          while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
           *startLine -= hlist->list[i].lbefore;
           *numberOfLines += hlist->list[i].lbefore;
            i--;	
          }
        // end
       
     } else { // both != 0 line both start and end of HL block
       *startLine     = (line - hlist->list[i].lbefore );
       *numberOfLines = (hlist->list[i].lbefore + hlist->list[i].lafter + 1);
       // look for previous lines
        i--;
         while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
           *startLine -= hlist->list[i].lbefore;
           *numberOfLines += hlist->list[i].lbefore;
           i--;	
         }
       // end
       }
	
   }
   else if ( line == hlist->list[i+1].line ){ // matches line at right of interval

     if ( hlist->list[i+1].lbefore == 0 && hlist->list[i+1].lafter != 0 ) {
       *startLine = hlist->list[i+1].line;
       *numberOfLines = hlist->list[i+1].lafter + 1;
       
       // look for previous lines
        while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
          *startLine -= hlist->list[i].lbefore;
          *numberOfLines += hlist->list[i].lbefore;
          i--;	
        }
             // end
     }
     else if ( hlist->list[i+1].lbefore != 0 && hlist->list[i+1].lafter == 0){
       *startLine     = (line - hlist->list[i+1].lbefore);
       *numberOfLines = hlist->list[i+1].lbefore + 1;
       // look for previous lines
         while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
           *startLine -= hlist->list[i].lbefore;
           *numberOfLines += hlist->list[i].lbefore;
           i--;	
          }
       // end
       
     } else { // both != 0 line both start and end of HL block
       *startLine     = (line - hlist->list[i+1].lbefore );
       *numberOfLines = (hlist->list[i+1].lbefore + hlist->list[i+1].lafter + 1);
       // look for previous lines
         while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
           *startLine -= hlist->list[i].lbefore;
           *numberOfLines += hlist->list[i].lbefore;
           i--;	
         }
       // end
       }
      	
   }
   else if ( hlist->list[i].lafter != 0 ){ // matches interval && lafter !=0
     *startLine = hlist->list[i].line - hlist->list[i].lbefore;
     *numberOfLines = hlist->list[i].lafter + hlist->list[i].lbefore + 1;
      // look for previous lines
      i--;
      while ( i > 0 && hlist->list[i].line == *startLine && hlist->list[i].lbefore > 0 ){
      *startLine -= hlist->list[i].lbefore;
      *numberOfLines += hlist->list[i].lbefore;
      i--;	
      }
      // end    
      	
   } 	
   else { // matches interval && lafter == 0 - it is not a real interval
     *startLine = line;
     *numberOfLines = 99;
      	
   }

}
static int hlLnListFind(hlLnList *hlist, unsigned int line, unsigned int *pos)
{
   unsigned int i;
   
   for ( i = 0; i< hlist->len;i++ ){
     if ( line == hlist[i].list->line) {
       *pos = i;
        return 1;	
     }
   	  
   }
   return 0;	
}
/*
int main(int argc, char**argv)
{
   unsigned int startLine,numberOfLines;

   unsigned int lineChanged = 7;
   
   hlLnList hlist;

   hlLnListInit(&hlist);

   hlLnListInsert(&hlist,1,0,1);
   hlLnListInsert(&hlist,2,1,0);
   hlLnListInsert(&hlist,4,0,2);
   hlLnListInsert(&hlist,4,1,0);
   hlLnListInsert(&hlist,6,2,0);
   hlLnListInsert(&hlist,6,0,2);
   hlLnListInsert(&hlist,8,2,0);
   hlLnListInsert(&hlist,11,0,2);
   hlLnListInsert(&hlist,13,2,0);
   
   hlLnListPrint(&hlist);
   
   int i;
   for (i=0;i<20;i++) {
      hlLnListGetLineRange(&hlist,i,&startLine,&numberOfLines);	
      printf("Line Changed:%d,Line Init: %d, Number of Lines %d\n",i,startLine,numberOfLines);
   }
     
    
   hlLnListClose(&hlist);
   

	
}

*/
