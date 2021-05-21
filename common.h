#ifndef _H_COMMON_
#define _H_COMMON_

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
     
#define CTRL_KEY(k) ( k & 0x1f )

#define ABUF_INIT { NULL , 0 , 0 }

#define KILO_VERSION "0.0.1"

#define KILO_TAB_STOP 8

#define KILO_QUIT_TIMES 3

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

typedef struct {

    int  rsize;
    char *render;
    int  size;
    char *chars;
    char renderInUse;
    __int32_t* hl; 
    //__int32_t hl;

} erow;

typedef struct {

    size_t colorfg;
    size_t colorbg;
    char bold;
    char italic;
    char underline;
    char regular;

} fontStyle; 

typedef struct {
    
  char **tokens;
  fontStyle fstyle;
	
} hlType;

typedef struct {

  char name[20];

  hlType var;
  hlType keyword;
  hlType preprocessor;
  hlType number;
  hlType operator;
  hlType upper;
  hlType str;
  hlType comment;
  	
} fileType;

typedef struct {

    int    rx;
    int    cx,cy;
    int    screenrows,screencols;
    int    numrows;
    int    rowoff,coloff;
    int    mousex,mousey;
    erow   *row;
    char   *filename;
    char   *fileTypeName;
    char   statusmsg[80];
    time_t statusmsg_time; 
    int    dirty;
    int    leftTabSize;
    fileType ft;
    struct termios orig_termios;
    hlLnList hlist;

} editorConfig;

struct abuf {
	
	char *b;
	int len;
	int bsize;
};


/////////////// error handling
void die(const char *s);
/////////////// terminal
// disable raw mode on exit on error
void disableRawMode();
#endif
