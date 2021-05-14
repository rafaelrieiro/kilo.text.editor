#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>

#ifndef _H_COMMON_
#include "common.h"
#endif

#ifndef _H_HIGHLIGHT_
#define _H_HIGHLIGHT_

#define ESEQ_INVERT_COLOR     "\x1b[7m"
#define ESEQ_RESET_MODE       "\x1b[m"
#define ESEQ_CR_NL            "\r\n"
#define ESEQ_CURSOR_HIDE      "\x1b[?251"
#define ESEQ_CURSOR_SHOW      "\x1b[?25h" 
#define ESEQ_CURSOR_GOHOME    "\x1b[H"


#define ESEQ_SET_FG_COLOR(str,color)    (sprintf(str,"\x1b[38;5;%dm",color))
#define ESEQ_SET_BG_COLOR(str,color)    (sprintf(str,"\x1b[48;5;%dm",color))
#define ESEQ_SET_FONT_BOLD(str)         (sprintf(str,"\x1b[1m"))
#define ESEQ_SET_FONT_NOT_BOLD(str)     (sprintf(str,"\x1b[22m"))
#define ESEQ_SET_FONT_ITALIC(str)       (sprintf(str,"\x1b[3m"))
#define ESEQ_SET_FONT_NOT_ITALIC(str)   (sprintf(str,"\x1b[23m"))
#define ESEQ_SET_FONT_UNDERLINE(str)    (sprintf(str,"\x1b[4m"))
#define ESEQ_SET_FONT_NOT_UNDERLINE(str)(sprintf(str,"\x1b[24m"))
#define ESEQ_SET_FONT_REGULAR(str)      (sprintf(str,"\x1b[10m"))
#define ESEQ_SET_RESET(str)             (sprintf(str,"\x1b[0m"))

#define GET_FG_COLOR(x)         ( x & 0xff )
#define GET_BG_COLOR(x)         ( ( x & 0xff00 ) >> 8 )
#define GET_HL_CHANGE(x)        ( x & 0x80000000 )
#define GET_HL_RESET(x)         ( x & 0x40000000 )
#define GET_FT_BOLD(x)          ( x & 0x10000 )
#define GET_FT_ITALIC(x)        ( x & 0x20000 )
#define GET_FT_UNDERLINE(x)     ( x & 0x40000 )
#define GET_FT_REGULAR(x)       ( x & 0x80000 ) 

#define SET_FG_COLOR(x,color)   ( x |= (  color )  )
#define SET_BG_COLOR(x,color)   ( x |= (  color << 8  )  )
#define SET_HL_CHANGEON(x)      ( x  = ( x & 0x0 ) | 0x80000000  )
#define SET_HL_CHANGEOFF(x)     ( x  = ( x & 0x0 ) )
#define SET_HL_RESET(x)         ( x |= 0x40000000 )
#define SET_HL_ML_COMMENT(x)    ( x |= 0x20000000 )
#define SET_FT_BOLD(x)          ( x |= 0x10000 )
#define SET_FT_ITALIC(x)        ( x |= 0x20000 )
#define SET_FT_UNDERLINE(x)     ( x |= 0x40000 )
#define SET_FT_REGULAR(x)       ( x |= 0x80000 )

#define MAX_BF_ESEQ_SIZE                40 

int  editorRowGetHL(erow *row,int pos,char *str);
void editorUpdateHL(editorConfig *text,char *oldline,char *newline, int lineNumber);
void hlUpdateAll(editorConfig *text);
void hlUpdateRow(editorConfig *text,int linePos);

#endif
