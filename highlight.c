
#define _GNU_SOURCE 
#define _BSD_SOURCE    
#define _DEFAULT_SOURCE

#include "highlight.h"
#ifndef _HLLINES_H_
#include "hllines.h"
#endif

static void hlUpdateUpper(erow *row,hlType *hl, char *s, int initPos);
static void hlUpdateReset(erow *row, int offsetPos, int size);
static void hlUpdateToken(erow *row,hlType* hl,char *s, int initPos);
static void hlUpdateNumber(erow *row,hlType* hl, char *s, int initPos);
static void hlUpdateOp(erow *row,hlType *hl, char *s ,int initPos);
static int  hlUpdateCommentRec(erow *row, fileType *ft, unsigned int initPos);
static void hlUpdateApply(erow *row, hlType *hl,int initPos, int size);
static int  hlUpdateStrRec(erow *row, hlType *hl, char *s, unsigned int offSetPos,unsigned int initPos);
static int  hlUpdateNonComment(erow *row, fileType* ft, char *s,const int offsetPos);
static void hlUpdateUntilFindString(editorConfig *text,hlType *hl,int* lineNumber,char *s,int* lastPos);


int editorRowGetHL(erow *row,int pos,char *str,int screenpos) 
{
    char temp[MAX_BF_ESEQ_SIZE];
    static char oldStr[MAX_BF_ESEQ_SIZE];

    strcpy(str,"\0");
    strcpy(temp,"\0");
    if ( screenpos == 0 ) strcpy(oldStr,"\0");

    if ( GET_HL_CHANGE(row->hl[pos]) ) { 

      if  ( !( GET_HL_RESET(row->hl[pos]) ) ) {

        ESEQ_SET_BG_COLOR(temp,GET_BG_COLOR(row->hl[pos]));
        strcat(str,temp);
      
        ESEQ_SET_FG_COLOR(temp,GET_FG_COLOR(row->hl[pos]));
        strcat(str,temp);
     
        if (!( GET_FT_REGULAR(row->hl[pos]))) {

         
          if ( GET_FT_BOLD(row->hl[pos]) ) {

            ESEQ_SET_FONT_BOLD(temp);
            strcat(str,temp);
          } else {

            ESEQ_SET_FONT_NOT_BOLD(temp);
          	strcat(str,temp);
          }

          if ( GET_FT_ITALIC(row->hl[pos]) ){

            ESEQ_SET_FONT_ITALIC(temp);
            strcat(str,temp);
          } else {

          	ESEQ_SET_FONT_NOT_ITALIC(temp);
          	strcat(str,temp);
          }

          if ( GET_FT_UNDERLINE(row->hl[pos]) ){
      
            ESEQ_SET_FONT_UNDERLINE(temp);
            strcat(str,temp);
          } else {

            ESEQ_SET_FONT_NOT_UNDERLINE(temp);
            strcat(str,temp);
          	
          }
          
          if ( strcmp(oldStr,str) != 0 ) {
            strcpy(oldStr,str);
            return 1;
          }  
          else return 0;
            
                     
            
       } else {

       	  ESEQ_SET_FONT_REGULAR(temp);
          strcat(str,temp);

         }
        
        if ( strcmp(oldStr,str) != 0 ) {
          strcpy(oldStr,str);
          return 1;
        }  
        else return 0;

        //return 1;
      }

      else {

        ESEQ_SET_RESET(str);
        strcat(str,temp); 
        
        if ( strcmp(oldStr,str) != 0 ) {
          strcpy(oldStr,str);
          return 1;
        }  
        else return 0;


      	//return 1;
      }
    
    } 

    return 0;
   
}

    
static void hlUpdateUpper(erow *row, hlType *hl,char *s, int initPos)
{
    unsigned int j;
    int initUpperPos;
    int hlSize;
    int pos;
              
    for ( j = 0; j < strlen(s) ; j++) {

      if ( isupper(s[j]) ) {
        hlSize = 1;
        initUpperPos = j;
        while (isupper(s[++j])) 
          hlSize++;

        if ( ( s[j] == ' ' || s[j] == '\0' || ispunct(s[j])) 
            &&
           ( initUpperPos == 0 || s[initUpperPos-1] == '_' || s[initUpperPos-1] == ' ' || ispunct(s[j]))     
            && 
           ( !( islower(s[(initUpperPos>0?initUpperPos-1:initUpperPos)]  ) ) ) 
                                                                    ) {
           pos = j;
           
           while ( j < strlen(s) && s[j++] == '_' ) 
             hlSize++;
           while ( initUpperPos != 0 && s[initUpperPos-1] == '_') {
             initUpperPos--;
             hlSize++;
           }

           j = pos; 
                      
           if ( hlSize > 1 )
             hlUpdateApply(row,hl,initUpperPos + initPos ,hlSize);
             
       }
              
     } 
   }

  
}

static void hlUpdateNumber(erow *row,hlType* hl,char *s, int initPos)
{
    unsigned int j;
    char *current,*last;
    int pos;
    int initNPos;
    int offset;
    unsigned int hlSize;
    
    for ( j = 0; j < strlen(s) ; j++) {
       if ( isdigit(s[j]) ) {
        
         hlSize = 1;
         initNPos = j;
          
         while (isdigit(s[++j])) hlSize++;
                                  
            if ( ( ( s[j] == '\0' || s[j] == ' ' || 
                   ispunct(s[j]) ) && s[j] != '_' && ( !( isalpha(s[j] ) ) ) ) 
                   &&
                 ( initNPos == 0 || ( s[initNPos-1] != '_' && 
                 ( ( !( isalpha(s[initNPos -1] ) ) ) ) ) ) ) {

              hlUpdateApply(row,hl,initNPos+initPos,hlSize);
            }
              
        } 
    }
   
   offset = 0;

   last = current = s;
   
   while ( ( current = strstr(last,"0x") ) != NULL ) {

     hlSize = 2;
     
     pos = ( current - last ) + offset; 

     initNPos = pos;

     while ( isxdigit( s[ pos + 2 ] )) {
       hlSize++; 
       pos++;
     }
    
     if (hlSize > 2 ) {
       hlUpdateApply(row,hl,initNPos+initPos,hlSize);
     }
      
   offset = initNPos + 1;
   last = current + 1;
   
  }

  
}
 
static void hlUpdateReset(erow *row, int offsetPos, int size)
{
    int j;
    //for ( j = 0; j < row->rsize ; j++) {
    for ( j = 0 ; j < size ; j++){
      	  SET_HL_CHANGEON(row->hl[j + offsetPos]);
    	  SET_HL_RESET(row->hl[j + offsetPos]);	
    }
}

static char *hlGetSubStringBeforeComment(erow *row,int initPos,int commentStart)
{
  int strSize = commentStart - initPos;
  char *subStr = malloc( strSize + 1);
  if (subStr == NULL) die("malloc - hlGetSubStringBeforeComment");
  strncpy(subStr,row->render + initPos, strSize );
  subStr[ strSize ]='\0'; 
  return subStr;
  
} 

static int hlUpdateCommentRec(erow *row,fileType *ft,unsigned int initPos)
{

  char *t1,*t2;
  char *t3;
  unsigned int nextPos;
  unsigned int commentStart;
  unsigned int commentSize;
  int hlResult;
  char *subStr;
  char *s = row->render;
      
  if ( initPos >= strlen(s) ) return 0; // end or past end string
  
  t1 = strstr(s + initPos,"/*");
  t2 = strstr(s + initPos,"//");
  t3 = strchr(s + initPos,'\"');
  //t4 = strchr(s + initPos,'\'');
    
  if ( t1 && t3 && (t3 < t1) ) { // discard /* between double quotes
       t3 = strchr(t3+1,'\"');
     	if (t3) t1 = NULL;
  } 
  
  if ( t2 && t3 && (t3 < t2) ) { // discard // between double quotes
         t3 = strchr(t3+1,'\"');
       	if (t3) t2 = NULL;
  }

  if (!(t1) && !(t2) )  // no comments in line
    return hlUpdateNonComment(row,ft,s+initPos,initPos);
 
   
  else if ( ( t1 == NULL && t2 != NULL ) || ( ( t1 != NULL && t2 != NULL ) && ( t1 > t2 ) ) ) {
    // found // before /* or just // 

      commentStart = ( t2 - (s + initPos)) + initPos;
      commentSize  = strlen(s) - commentStart;
      
      if (commentStart > initPos) {
        
        subStr = hlGetSubStringBeforeComment(row,initPos,commentStart);
        hlResult = hlUpdateNonComment(row,ft,subStr,initPos);
        free(subStr);
      }
      
      hlUpdateApply(row,&ft->comment,commentStart,commentSize);
  	  return 0;
    }
    else if ( ( t1 != NULL && t2 == NULL ) || ( ( t1 != NULL && t2 != NULL ) && ( t1 < t2 ) )  ) {
    // found /* before // or /* found alone ->  look for */ terminating

      commentStart = ( t1 - ( s + initPos ) ) + initPos;
      t2 = strstr( t1+1,"*/");

      if (t2) 
        commentSize  = (t2 - t1) + 1 + 1;  // ( +1 due to t1+1 in str, +1 for size of */ )
      else
        commentSize = strlen(s) - commentStart;
        
      if ( commentStart > initPos) {

        subStr = hlGetSubStringBeforeComment(row,initPos,commentStart);
        hlResult = hlUpdateNonComment(row,ft,subStr,initPos);
        free(subStr);
         
        if ( hlResult != 0 ) return hlResult; 
     
      }
          
      if (t2) {  // "found */, fill until */"

         hlUpdateApply(row,&ft->comment,commentStart,commentSize);
         nextPos = ( t1 - ( s + initPos) ) + ( t2 - t1 ) + initPos + 2;
         hlUpdateCommentRec(row,ft,nextPos); // run again
  	
      } else { // */ not found,fill until end of line and return 1
        
         if ( commentStart > initPos){

           subStr = hlGetSubStringBeforeComment(row,initPos,commentStart);
           hlResult = hlUpdateNonComment(row,ft,subStr,initPos);
           free(subStr);
         }
        
         hlUpdateApply(row,&ft->comment,commentStart,commentSize);
  	     return 1;
          
        }  

     } else return 0; 

 return 0;   
}

// offset is just used internally to index hl[+offsetPos]
static int hlUpdateNonComment(erow *row, fileType* ft, char *s,const int offsetPos)
{
 
  hlUpdateReset(row,offsetPos,strlen(s));
  hlUpdateToken(row,&ft->var,s,offsetPos);
  hlUpdateToken(row,&ft->keyword,s,offsetPos);
  hlUpdateToken(row,&ft->preprocessor,s,offsetPos);
  hlUpdateNumber(row,&ft->number,s,offsetPos); 
  hlUpdateOp(row,&ft->operator,s,offsetPos);
  hlUpdateUpper(row,&ft->upper,s,offsetPos);
  return hlUpdateStrRec(row,&ft->str,s,offsetPos,0);
  return 0;	
}


static void hlUpdateApply(erow *row,hlType *hl,int initPos, int size)
{
  int i;
  if ( initPos < 0 || size <= 0 || initPos + size > row->rsize ) return;
   
  for ( i = 0; i < size; i++) {
    SET_HL_CHANGEON(row->hl[initPos+i]);
    if (hl->fstyle.bold        == 'Y' ) SET_FT_BOLD(row->hl[initPos+i]);
    if (hl->fstyle.italic      == 'Y' ) SET_FT_ITALIC(row->hl[initPos+i]);
    if (hl->fstyle.underline   == 'Y' ) SET_FT_UNDERLINE(row->hl[initPos+i]);
    if (hl->fstyle.regular     == 'Y' ) SET_FT_REGULAR(row->hl[initPos+i]);
    SET_FG_COLOR(row->hl[initPos+i],hl->fstyle.colorfg);
    SET_BG_COLOR(row->hl[initPos+i],hl->fstyle.colorbg);  
  }
}

static void hlUpdateOp(erow *row,hlType *hl, char *s ,int initPos)
{
  
    int count = 0;
    int hlSize;
    int offset;
    int initOpPos;
    char *current,*last;

    last = current = s;
    
    offset = 0;

    if ( hl->tokens == NULL ) return;
    
    while  ( strcmp(hl->tokens[count],"\0") != 0 ){

      while ( ( current = strstr(last,hl->tokens[count]) ) != NULL ){

        initOpPos = (current - last) + offset;
        hlSize = strlen(hl->tokens[count]);
               
        hlUpdateApply(row,hl,initOpPos+initPos,hlSize);
      
        offset = initOpPos + 1;
        last = current + 1;
        
      }   

      last = s;
      offset = 0;
      count++; 
   }
  
}

static void hlUpdateToken(erow *row, hlType* hl, char *s, int initPos)
{

    int count = 0;
    int hlSize;
    int offset;
    int pos;
    char *current,*last;
    
    last = current = s;
    
    offset = 0;

    if ( hl->tokens == NULL ) return;
    
    while  ( strcmp(hl->tokens[count],"\0") != 0 ){

      while ( ( current = strstr(last,hl->tokens[count]) ) != NULL ){

        pos = ( current - last ) + offset;
        hlSize = strlen(hl->tokens[count]);
        char leftchr,rightchr;

        unsigned int ptrDiff = current - last;
        
        leftchr  = (ptrDiff>0?last[(current - last) - 1]:'*');
        rightchr = ((( ptrDiff + hlSize ) < strlen(last))?last[ptrDiff + hlSize]:'*');

        if (  ( isalnum(leftchr) == 0 ) && ( isalnum(rightchr) == 0 ) )  
          hlUpdateApply(row,hl,pos+initPos,hlSize);
   
        offset = pos + 1;
        last = current + 1;
        
      }   
     
     last = s;
     offset = 0;
     count++; 
   }

}    

static int hlUpdateStrRec(erow *row, hlType *hl,  char *s, unsigned int offSetPos,unsigned int initPos)
{

  char *t1,*t2;
  int hlStart,hlSize;

  if (initPos >= strlen(s)) return 0; 

  t1 = strchr(s + initPos,'\'');
  t2 = strchr(s + initPos,'\"');

  //fix escape seq
  if (t1 != NULL &&  t1>(s+initPos) && t1[-1] == '\\'  ) {
      
      t1 = strchr(t1+1,'\'');
  }
   
    //if (t2 != NULL )  // fix escape sequence \"

  if (t1 == NULL && t2 == NULL ) return 0; // nothing to do
  
    else if (  t1 == NULL && t2 != NULL   ) {

      // found only "\""
      hlStart = ( t2 - ( s+initPos ) ) + initPos;
      t1 = strchr(t2+1,'\"');

      //fix escape seq
      if (t1 != NULL &&  t1>(s+initPos) && t1[-1] == '\\' && t1>(s+initPos+1) && t1[-2]!='\\')   {
        t1 = strchr(t1+1,'\"');
      }

      if (t1) { // found terminating "\""
        //hlSize  =  hlStart + ( t1 - t2 );
        hlSize = (t1 - t2) + 1;
        hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);
        // run again
        //hlUpdateStrRec(row,hl,s,offSetPos,hlStart+hlSize);
        return hlUpdateStrRec(row,hl,s,offSetPos,hlStart+hlSize);
      } else { // terminating "\"" not found. fill until EOL and return 2
        hlSize = strlen(s) - hlStart;
        hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);  
        return 2;       	
        }

    }
    else if ( ( t1 != NULL && t2 == NULL ) ) {

      // found only '\''

      hlStart = ( t1 - s );

      t2 = strchr(t1+1,'\''); 

      //fix escape seq
      if (t2 != NULL &&  t2>(s+initPos) && t2[-1] == '\\' && t2>(s+initPos+1) && t2[-2]!='\\')   {
        t2 = strchr(t2+1,'\'');
      }

            
      if (t2) {
         
         hlSize  = (t2 - t1) + 1 ;  // ( +1 to jump from * to / )
         // found '\'', fill until it
         hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);
               
         // run again
         return hlUpdateStrRec(row,hl,s,offSetPos,hlStart+hlSize); 
  	
      } else {
         // terminating "\'" not found. fill until EOL and return 3
         
         hlSize = strlen(s) - hlStart;
         hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);  
         return 3;
      
      }  

     } else if ( t1 != NULL && t2 != NULL && t2 > t1){
      // found  '\'' before '\"'
      hlStart = ( t1 - s );
      t2 = strchr(t1+1,'\'');  
      
        if (t2) {
         
           hlSize  = (t2 - t1) + 1 ;  // ( +1 due to t1+1 in str, +1 for size of */ )
           // found '\'', fill until it
           hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);
           // run again
           return hlUpdateStrRec(row,hl,s,offSetPos,hlStart+hlSize);
     	
       } else {
           // terminating "\'" not found. fill until EOL and return 3
           hlSize = strlen(s) - hlStart;
           hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);  
           return 3; 
       }

     } else if ( t1 != NULL && t2 != NULL && t2 < t1){

        hlStart = ( t2 - s );
      
        t1 = strchr(t2+1,'\"');

        if (t1) { // found terminating "\""
          //hlSize  =  ( t1 - t2 );
          hlSize  =  ( t1 - t2 ) + 1;
          hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);
          // run again
          return hlUpdateStrRec(row,hl,s,offSetPos,hlStart+hlSize);
        } else { // terminating "\"" not found. fill until EOL and return 2
          hlSize = strlen(s) - hlStart;
          hlUpdateApply(row,hl,hlStart+offSetPos,hlSize);  
          return 2;       	
          }
 	
      }

 return 0;   
}



static void hlUpdateUntilFindString(editorConfig *text,hlType *hl,int* lineNumber,char *s,int* lastPos)
{
    char *pos;
    
    while ( (*lineNumber < text->numrows ) && ( ( pos = strstr(text->row[*lineNumber].render,s) ) == NULL ) ) {  
      hlUpdateApply(&text->row[*lineNumber],hl,0,text->row[*lineNumber].rsize);

      *lineNumber += 1;	
    }
    if (*lineNumber > text->numrows - 1 ) *lineNumber = text->numrows - 1;
    //if (*lineNumber < text->numrows )
    //if (*lineNumber < text->numrows )
    //{
      *lastPos = (pos - text->row[*lineNumber].render);
      *lastPos += strlen(s); 
      hlUpdateApply(&text->row[*lineNumber],hl,0,*lastPos);
      
    //} 
    
}



void hlUpdateAll(editorConfig *text)
{
   int multiLine;
   hlType hl;
   char str[3];
   int lastPos = 0;
   int line,endLine;
   //unsigned initLine,numberOfLines;
   int beginML,endML;
   int i;
   int sizePrevML;

   line = 0;
   endLine = text->numrows - 1; 

   while ( line <= endLine) {
       
       //reset lafter from init line
       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,0); 
       if (multiLine == 0) {
         
         sizePrevML=hlLnListGetAfter(&text->hlist,line);
         if (sizePrevML != 0) {
            hlLnListResetAfter(&text->hlist,line);
            hlLnListResetBefore(&text->hlist,line+sizePrevML);
         }
         
       }
       while (multiLine){
           beginML = line;
       //fill until terminating string and do hl in the rest of line
         switch (multiLine){
         case 1:
           strcpy(str,"*/");
           hl = text->ft.comment;
         break;
         case 2:
           strcpy(str,"\"");
           hl = text->ft.str;
         break;
         case 3:
           strcpy(str,"\'");
           hl = text->ft.str;
         break;
       	}	
         if (line > text->numrows - 2) return;
         line++; 
         hlUpdateUntilFindString(text,&hl,&line,str,&lastPos);// update lineNumber and lastPos 
           endML = line;
           for (i=beginML+1;i<endML;i++)
             hlLnListDeleteLine(&text->hlist,i);

           hlLnListInsert(&text->hlist,beginML,0,endML-beginML);
           hlLnListInsert(&text->hlist,endML,endML-beginML,0);
         
         if (line > text->numrows - 1) return;
         multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,lastPos);
         if (multiLine == 0) {
           
           sizePrevML=hlLnListGetAfter(&text->hlist,line);
           if (sizePrevML != 0) {
           hlLnListResetAfter(&text->hlist,line);
           hlLnListResetBefore(&text->hlist,line+sizePrevML);
           }
         }
       }  
       line++;
       
    }
}


void hlUpdateRow(editorConfig *text,int linePos)
{
   int multiLine;
   hlType hl;
   char str[3];
   int lastPos = 0;
   int line,endLine;
   unsigned initLine,numberOfLines;
   int beginML,endML;
   int i;
   int sizePrevML;

   hlLnListGetLineRange(&text->hlist,linePos,&initLine,&numberOfLines);  
   line = (( initLine < (unsigned int) text->numrows )?initLine:0);
  // endLine = ((line+numberOfLines <(unsigned int)text->numrows)?line+numberOfLines:(unsigned int)text->numrows - 1); 
   endLine = ((initLine+numberOfLines <(unsigned int)text->numrows)?line+numberOfLines:(unsigned int)text->numrows - 1); 
   while ( line <= endLine) {
       
       //reset lafter from init line
       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,0); 
       if (multiLine == 0) {
         
         sizePrevML=hlLnListGetAfter(&text->hlist,line);
         endLine = ( (endLine + sizePrevML) < text->numrows?endLine+sizePrevML:text->numrows - 1);
         if (sizePrevML != 0) {
            hlLnListResetAfter(&text->hlist,line);
            hlLnListResetBefore(&text->hlist,line+sizePrevML);
         }
         
       }
       while (multiLine){
           beginML = line;
       //fill until terminating string and do hl in the rest of line
         switch (multiLine){
         case 1:
           strcpy(str,"*/");
           hl = text->ft.comment;
         break;
         case 2:
           strcpy(str,"\"");
           hl = text->ft.str;
         break;
         case 3:
           strcpy(str,"\'");
           hl = text->ft.str;
         break;
       	}	
         if (line > text->numrows - 2) return;
         line++; 
         hlUpdateUntilFindString(text,&hl,&line,str,&lastPos);// update lineNumber and lastPos 
           endML = line;
           for (i=beginML+1;i<endML;i++)
             hlLnListDeleteLine(&text->hlist,i);

           hlLnListInsert(&text->hlist,beginML,0,endML-beginML);
           hlLnListInsert(&text->hlist,endML,endML-beginML,0);
         
         if (line > text->numrows - 1) return;
         multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,lastPos);
         sizePrevML=hlLnListGetAfter(&text->hlist,line);
         endLine = ( (endLine + sizePrevML) < text->numrows?endLine+sizePrevML:text->numrows - 1);
         if (multiLine == 0) {
           
           //sizePrevML=hlLnListGetAfter(&text->hlist,line);
           if (sizePrevML != 0) {
           hlLnListResetAfter(&text->hlist,line);
           hlLnListResetBefore(&text->hlist,line+sizePrevML);
           //continue hl even now it is multiLine 0
           //endLine = ( (endLine + sizePrevML) < text->numrows?endLine+sizePrevML:text->numrows - 1);
           }
         }
       }  
       line++;
       
    }
}

/*
static int hlCountChanged(char *oldline,char *newline,char *s)
{
 unsigned int newPos = 0; 
 char *pos;
 int oldCount = 0;
 int newCount = 0;
 
 while ( ( newPos < strlen(oldline) ) && ( ( pos = strstr(oldline + newPos,s) ) != NULL ) ){

    newPos += ( pos - ( oldline + newPos )) + 1;
 	oldCount++;
 }
 newPos = 0;   

 while ( ( newPos < strlen(newline) ) && ( ( pos = strstr(oldline + newPos,s) ) != NULL ) ){
 
    newPos += ( pos - ( oldline + newPos )) + 1;
  	newCount++;
 }

 if ( oldCount != newCount )  return 1;

 return 0; 
} 

//if need to hl update from start of file, 0 if not 
static int hlIsUpdateFromInitRowNeeded(char *oldline,char *newline)
{
  if (hlCountChanged(oldline,newline,"*")) return 1;
  if (hlCountChanged(oldline,newline,"\"")) return 1;
  if (hlCountChanged(oldline,newline,"\'")) return 1;
      
  return 0;    
}

static void hlUpdateFromInitRowUntil(editorConfig *text,int lineNumber)
{
   int multiLine;
   hlType hl;
   char str[3];
   int lastPos = 0;
   int line = 0;

   while ( line < lineNumber) {

       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,0); 
       while (multiLine){
       //fill until terminating string and do hl in the rest of line
         switch (multiLine){
         case 1:
           strcpy(str,"");
           hl = text->ft.comment;
         break;
         case 2:
           strcpy(str,"\"");
           hl = text->ft.str;
         break;
         case 3:
           strcpy(str,"\'");
           hl = text->ft.str;
         break;
       	}	
       if (line > text->numrows - 2) return;
       line++; 
       hlUpdateUntilFindString(text,&hl,&line,str,&lastPos);// update lineNumber and lastPos 

       if (line > text->numrows - 1) return;
       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,lastPos);
       }  
       line++;
       
    }
}

static void hlUpdateFromCurrentRow(editorConfig *text,int lineNumber)
{
   int multiLine;
   hlType hl;
   char str[3];
   int lastPos = 0;
   int line = lineNumber;

  
       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,0); 
       while (multiLine){
        //fill until terminating string and do hl in the rest of line
         switch (multiLine){
         case 1:
           strcpy(str,"");
           hl = text->ft.comment;
         break;
         case 2:
           strcpy(str,"\"");
           hl = text->ft.str;
         break;
         case 3:
           strcpy(str,"\'");
           hl = text->ft.str;
         break;
       	}	
       if (line > text->numrows - 2) return;
       line++; 
       hlUpdateUntilFindString(text,&hl,&line,str,&lastPos);// update lineNumber and lastPos 

       if (line > text->numrows - 1) return;
       multiLine = hlUpdateCommentRec(&text->row[line],&text->ft,lastPos);
       }  
        
       
}	

void editorUpdateHL(editorConfig *text,char *oldline,char *newline,int lineNumber)
{

  int b;
  
    if ( oldline == NULL && newline == NULL ){
    //full scan needed at least to lineNumber - initial file hl
    // hlUpdateFromInitRowUntil(text,text->numrows);
    hlUpdateFromCurrentRow(text,lineNumber);
    } else if ( oldline == NULL && newline != NULL  ){
    // scan from current line - no lines to compare	
      // hlUpdateFromCurrentRow(text,lineNumber);      
    } else if ( oldline != NULL && newline != NULL ){
      // compare old and newlines for scan until current line
      //if (hlIsUpdateFromInitRowNeeded(oldline,newline) ) 
      //  hlUpdateFromInitRowUntil(text,lineNumber);
      //hlUpdateFromInitRowUntil(text,text->numrows);
      //  else     
      //scan from current line
      //hlUpdateFromCurrentRow(text,lineNumber);
      }
     
}
//static void hlUpdateFromCurrentRow(editorConfig *text,int lineNumber);
//static void hlUpdateFromInitRowUntil(editorConfig *text,int lineNumber);
*/

