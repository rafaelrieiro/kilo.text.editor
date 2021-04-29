/*
 * kilo.c
 * 
 * Copyright 2021 Unknown <alarm@alarmpi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */
/* Tutorial found on 
 * https://viewsourcecode.org/snaptoken/
 * 
 * 
 */
/****************** defines *****************/

// https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
#define _GNU_SOURCE 
/* If you define this macro, everything is included: 
*  ISO C89, ISO C99, POSIX.1, POSIX.2, BSD, SVID, X/Open, LFS, and GNU extensions.
*  In the cases where POSIX.1 conflicts with BSD, the POSIX definitions take precedence. 
*/
#define _BSD_SOURCE    
#define _DEFAULT_SOURCE
/* If you define this macro, most features are included apart from X/Open, 
*  LFS and GNU extensions: the effect is to enable features from the 2008 edition of POSIX,
*  as well as certain BSD and SVID features without a separate feature test macro to control them. 
*/
     
#define CTRL_KEY(k) ( k & 0x1f )

#define ABUF_INIT { NULL , 0 }

#define KILO_VERSION "0.0.1"

#define KILO_TAB_STOP 8

#define KILO_QUIT_TIMES 3

/**************  Highlight ***********************/

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
#define GET_HL_ML_COMMENT(x)    ( x & 0x20000000 )
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
//#define SET_HL_ML_COMMENTOFF(x) ( x ^= 0x20000000 )
#define SET_FT_BOLD(x)          ( x |= 0x10000 )
#define SET_FT_ITALIC(x)        ( x |= 0x20000 )
#define SET_FT_UNDERLINE(x)     ( x |= 0x40000 )
#define SET_FT_REGULAR(x)       ( x |= 0x80000 )

#define MAX_BF_ESEQ_SIZE        40  
 

/*************** libraries  ***************/

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


/************** data *******************/

typedef struct {

    int  rsize;
    char *render;
    int  size;
    char *chars;
    __int32_t* hl; 

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
  	
} filetype;

typedef struct {

    int    rx;
    int    cx,cy;
    int    screenrows,screencols;
    int    numrows;
    int    rowoff,coloff;
    int    mousex,mousey;
    erow   *row;
    char   *filename;
    char   *filetype;
    char   statusmsg[80];
    time_t statusmsg_time; 
    int    dirty;
    filetype ft;
    struct termios orig_termios;

} editorConfig;

struct abuf {
	
	char *b;
	int len;
};

editorConfig E;

enum editorKey {

    BACKSPACE=127,
    ARROW_LEFT=1000,ARROW_RIGHT,ARROW_UP,ARROW_DOWN,
    DEL_KEY,PAGE_UP,PAGE_DOWN,END_KEY,HOME_KEY,RETURN,ESCAPE,TAB_KEY,
    MOUSE_SCROLL_DOWN,MOUSE_SCROLL_UP,MOUSE_LEFT_CLICK,MOUSE_RIGHT_CLICK,MOUSE_MIDDLE_CLICK
};





/*************** function prototypes *************/

//// terminal
void enableRawMode();
void disableRawMode();
void enableMouseTracking();
void disableMouseTracking();
int  getWindowSize(int *rows, int *cols);
int  editorReadKey();

/// input
void   editorProcessKey();
char * editorPrompt(char *prompt,void(*callback)(const char* input,const int key));

/// error handling
void die(const char *s);

/// output
void editorMoveCursor(int key);
void initEditor();
void editorDrawRows(struct abuf *ab);
void editorRefreshScreen();
void editorScroll();
void editorDrawStatusBar(struct abuf *ab);
void editorDrawMessageBar(struct abuf *ab);
void editorSetStatusMessage(const char *fmt, ...);

/// data handling
void abFree(struct abuf *ab);
void abAppend(struct abuf *ab,const char* s,int len);

// file IO

void editorOpen(char *filename);
char *editorRowsToString(int *buflen);
void editorSave();
void editorLoadHLTemplate();

// row operations

void editorAppendRow(char *s, size_t len,int pos);
void editorUpdateRowRender(erow *row);
void editorUpdateRowHL(erow *row);
void editorUpdateRowHLTokens(erow *row,hlType *hl);
void editorUpdateRowHLNumbers(erow *row,hlType *hl);
void editorUpdateRowHLOps(erow *row,hlType *hl);
void editorUpdateRowHLUpper(erow *row,hlType *hl);
void editorUpdateRowHLApply(erow *row,hlType *hl,int pos, int size);
int  editorRowCxToRx(erow *row,int cx);
int editorRowGetHL(erow *row,int pos,char *str);

// row operations

void editorRowInsertChar(erow *row,int at,int c);
void editorRowDelChar(erow *row,int at);
void editorRowShift(int pos, int size);
void editorRowShiftDown(int lines);
void editorRowShiftUp(int lines);
void editorRowDel(int pos);
void editorRowCut(erow *row,int newsize);
void editorRowMerge(erow *row,char *s,int len);

// editor operations

void editorInsertChar(int c);
void editorRowInsert();
void editorFind();
void editorFindCallBack(const char *input,const int key);

//  memory management
void freeBuffers();

//signal handling
void sigHandlerSIGWINCH();
void sigHandlerSIGTERM();

/************** error handling ****************/

void die(const char *s)
{
	write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO,"\x1b[H",3);
	disableRawMode();
	perror(s);
	exit(1);
}

/**************** terminal *******************/

void enableRawMode()
{
    if ( tcgetattr(STDIN_FILENO,&E.orig_termios) == -1) die("tcgetattr");
	
	struct termios raw = E.orig_termios;
	
	raw.c_iflag &= ~( IXON | ICRNL | BRKINT | INPCK | ISTRIP );
	raw.c_oflag &= ~( OPOST );
	raw.c_lflag &= ~( ECHO | ICANON | ISIG | IEXTEN );
	raw.c_cflag |= ( CS8 );
	raw.c_cc[VMIN]  = 0;
	raw.c_cc[VTIME] = 1;

    if ( tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1 ) die("tcsetattr");

    enableMouseTracking();	
}


void disableRawMode()
{
	if ( tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios) == -1 ) die("tcsetattr");

    disableMouseTracking();

}

void enableMouseTracking()
{
  //write (STDOUT_FILENO, "\e[?47h", 6); // switch alternate buffer screen
  //write (STDOUT_FILENO, "\e[?9h", 5);	// enable mouse detect
  //write (STDOUT_FILENO, "\e[?1000h", 8); // normal tracking mode - send event on press and release
}

void disableMouseTracking()
{
   //write (STDOUT_FILENO, "\e[?9l", 5);    
   //write (STDOUT_FILENO, "\e[?47l", 6); // disable normal tracking mode - send event on press and release
   //write (STDOUT_FILENO, "\e[?1000l", 8); // normal tracking mode - send event on press and release
    
}

int editorReadKey()
{
	char c = '\0';
	int nread;
	
	while  ( ( nread = read(STDIN_FILENO,&c,1) ) != 1 ) {
		
	  if ( nread == -1 && errno != EAGAIN ) die("read");
		
	}	

    if ( c == '\x05')  return MOUSE_SCROLL_DOWN;
    if ( c == '\x19')  return MOUSE_SCROLL_UP;

    if ( c == '\x7f')  return BACKSPACE;
    if ( c == '\x0d')  return RETURN;
    if ( c == '\x09')  return TAB_KEY;
    

    if ( c == '\x1b') {

      char seq[5];

      if ( ( nread = read(STDIN_FILENO,&seq[0],1) ) != 1 ) return ESCAPE;
      if ( ( nread = read(STDIN_FILENO,&seq[1],1) ) != 1 ) return ESCAPE;

      if ( seq[0] == '[') {

        if ( ( seq[1] >= '0') && ( seq[1] <= '9' ) ) {

          if ( ( nread = read(STDIN_FILENO,&seq[2],1) ) != 1 ) return ESCAPE;

          if ( seq[2] == '~') {

            switch (seq[1]){
              case '1': return HOME_KEY; 
              break;
              case '3': return DEL_KEY;
              break;
              case '4': return END_KEY;
              break;
              case '5': return PAGE_UP;
              break;
              case '6': return PAGE_DOWN;
              break;
              case '7': return HOME_KEY;
              break;
              case '8': return END_KEY;
              break;
            }

          } 
                  	
        
        } else if ( seq[1] == 'M' ) { 

            if ( ( nread = read(STDIN_FILENO,&seq[2],1) ) != 1 ) return ESCAPE;
            if ( ( nread = read(STDIN_FILENO,&seq[3],1) ) != 1 ) return ESCAPE;
            if ( ( nread = read(STDIN_FILENO,&seq[4],1) ) != 1 ) return ESCAPE;
            
            E.mousex = seq[3] - 32;
            E.mousey = seq[4] - 32;             
               
            switch (seq[2]) {
              case ' ': return MOUSE_LEFT_CLICK;
              break;
              case '"': return MOUSE_RIGHT_CLICK;
              break;
              case '!': return MOUSE_MIDDLE_CLICK;
              break; 
              case 'a': return ARROW_DOWN;
              break;
              case '`': return ARROW_UP;
              break;	
            }   
           
            
          } else {
       
        switch (seq[1]){
          case 'A': return ARROW_UP;
          break;
          case 'B': return ARROW_DOWN;
          break; 
          case 'C': return ARROW_RIGHT;
          break;
          case 'D': return ARROW_LEFT;
          break;
          case 'H': return HOME_KEY;
          break;
          case 'F': return END_KEY;
          break;
          case 'P': return DEL_KEY;
          break;
         }

       }
   	
      } else if ( seq[0] == 'O'){

          switch (seq[1]){
            case 'H': return HOME_KEY;
            break;
            case 'F': return END_KEY;
            break;
	
          }
      	
      }
    	
    }  
    	
	return c;	
}

int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;
	
	if ( ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0 ) {
      
      return -1;
	
    } else {
	  
	  *rows = ws.ws_row;	
	  *cols = ws.ws_col;	
      return 0;     
    }	
}

/******************* input ********************/
void editorMoveCursor(int key)
{
    erow *row = (E.cy >= E.numrows ? NULL : &E.row[E.cy]);

    switch(key) {
		
    case MOUSE_SCROLL_UP:
      if ( E.rowoff > 0 ) E.rowoff--;
      if ( E.cy > 0 ) E.cy--;
    break;
	case ARROW_UP:
	  if ( E.cy > 0 ) E.cy--;
	break;
    case MOUSE_SCROLL_DOWN:
      if ( E.rowoff < ( E.numrows - E.screenrows)  ) E.rowoff++;
      if ( E.cy < ( E.numrows - 1) ) E.cy++;
    break;
	case ARROW_DOWN:
	  if ( E.cy < ( E.numrows - 1) ) E.cy++;
	break;
	case ARROW_LEFT:
	  if ( E.cx > 0 ) E.cx--;
	  else if ( E.cy > 0 ) { 
        E.cy--; 
        E.cx = E.row[E.cy].size;
	  }
	break;
	case ARROW_RIGHT:
      if ( row && E.cx < row->size) {
	    E.cx++;	
	  }
	  else if ( E.cy < ( E.numrows - 1 ) ) {
        E.cy++;
        E.cx=0;
	  }
    break;
    case PAGE_UP:
      if ( E.cy == E.rowoff ) {
        E.cy -= E.screenrows;
        if ( E.cy < 0 ) E.cy = 0;
      }
      else E.cy = E.rowoff; 
    break;
    case PAGE_DOWN:
      if ( E.cy == ( E.screenrows - 1 + E.rowoff ) ) E.cy += E.screenrows;
      else E.cy = E.screenrows - 1 + E.rowoff;
              
      if ( E.cy > ( E.numrows - 1 ) ) E.cy = E.numrows - 1;
    break; 
    case HOME_KEY:
      if ( E.cy < E.numrows) E.cx = 0;
    break;
    case END_KEY:
     if ( E.cy < E.numrows ) E.cx = E.row[E.cy].size;
    break;
    case MOUSE_LEFT_CLICK:
      E.cy = E.mousey + E.rowoff - 1;
      if ( E.cy < 0) E.cy = 0;
      if ( E.cy > ( E.numrows - 1)) E.cy = E.numrows - 1;
      E.cx = E.rx = E.mousex + E.coloff - 1;
    break;
    case MOUSE_RIGHT_CLICK:
      E.cy = E.mousey + E.rowoff - 1;
      E.cx = E.mousex + E.coloff - 1;
    break;
    case MOUSE_MIDDLE_CLICK:
      E.cy = E.mousey + E.rowoff - 1;
      E.cx = E.mousex + E.coloff - 1;
	break;
    }			
	
    row = (E.cy >= E.numrows ? NULL : &E.row[E.cy]);
    int rowlen = ( row ? row->size :0);
    if ( E.cx > rowlen ) E.cx=rowlen;

}
   
void editorProcessKey()
{
    static int quit_times = KILO_QUIT_TIMES;
    int c = editorReadKey();
	
	switch (c) {
		
	case CTRL_KEY('q'):
      if ( E.dirty > 0 && quit_times > 0)
      {
        editorSetStatusMessage("File has unsaved changes:\
        To quit anyway,press CRTL-Q more %d times",quit_times);   
        quit_times--;
        return;
          	
      } else {
      
          write(STDOUT_FILENO,"\x1b[2J",4);  //clear screen
          write(STDOUT_FILENO,"\x1b[H",3);   // put cursor at home
          exit(0);
        }
	break;
    case CTRL_KEY('s'):
      editorSave();
    break;
    case CTRL_KEY('f'):
      editorFind();
    break;
	
	case ARROW_LEFT:
	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_RIGHT:
	case MOUSE_SCROLL_DOWN:
	case MOUSE_SCROLL_UP:
    case PAGE_UP:
    case PAGE_DOWN:
    case HOME_KEY:
    case END_KEY:
    case MOUSE_LEFT_CLICK:
    case MOUSE_RIGHT_CLICK:
    case MOUSE_MIDDLE_CLICK:
	  editorMoveCursor(c);
	break;
    case DEL_KEY:
    case CTRL_KEY('h'):
      editorRowDelChar(&E.row[E.cy],E.cx);
    break;	
    case BACKSPACE:
      if ( E.cx > 0 ){

        editorRowDelChar(&E.row[E.cy],( E.cx - 1 ) );
        editorMoveCursor(ARROW_LEFT);
      } 
      else {

      	editorMoveCursor(ARROW_LEFT);
      	editorRowDelChar(&E.row[E.cy],E.cx);
      }
      
    break;
    case CTRL_KEY('l'):
    case ESCAPE:
    break;
    case RETURN:
      editorRowInsert();
      editorMoveCursor(ARROW_DOWN);
    break;
    case TAB_KEY:
      editorInsertChar('\t');
    break;
	default:
	   if ( !iscntrl(c) && c < 127 ) editorInsertChar(c);
	break;
    }

    quit_times = KILO_QUIT_TIMES;
}

char *editorPrompt(char *prompt,void(*callback)(const char *input, const int key))
{
   size_t bufsize = 128;

   int c;
   
   char *buf = malloc(bufsize);

   size_t buflen = 0;

   buf[0]='\0';
      
   while (1) {

     editorSetStatusMessage("%s%s",prompt,buf);
     editorRefreshScreen();

     c = editorReadKey();

     switch (c){

       case RETURN:
         editorSetStatusMessage("");
         if ( callback != NULL ) callback(buf,c); 
         return buf;
       break;
       case ESCAPE:
         editorSetStatusMessage("");
         free(buf);
         buf = NULL;
         return buf;
       case DEL_KEY:
       case BACKSPACE:
         if ( buflen > 0 )
           buf[--buflen]='\0'; 
       break;    
       default:

        if ( !iscntrl(c) && c < 127 ) {

          if ( buflen == bufsize - 1 ) {

            bufsize *= 2;
            buf = realloc(buf,bufsize);  
          }
                  
         buf[buflen++]  = c;
         buf[buflen] = 0;
        }
            
      break;
     	
     } 

   }

}   


/******************** init ********************/

void initEditor()
{
  E.rx = 0;
  E.cx = 0;
  E.cy = 0;	
  E.numrows = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0]='\0';
  E.statusmsg_time = 0;
  if ( getWindowSize(&E.screenrows,&E.screencols) == -1 ) die("getWindowSize");
  E.screenrows -= 2; // one line for status bar and other for status messages - this is the cause of resize offset!!

  E.ft.operator.tokens = NULL;
  E.ft.str.tokens = NULL;
  E.ft.var.tokens = NULL;
  E.ft.preprocessor.tokens = NULL;
  E.ft.number.tokens = NULL;
  E.ft.upper.tokens = NULL;
  E.ft.keyword.tokens = NULL;

  editorLoadHLTemplate();
}

/******************* append buffer **************/

void abFree(struct abuf *ab)
{
	free(ab->b);
}

void abAppend(struct abuf *ab,const char* s,int len)
{
	char *new=realloc(ab->b, ab->len + len);
	
	if ( new == NULL ) die("realloc");
	
	memcpy(&new[ab->len],s,len);

    ab->b    = new;
	ab->len += len; 
	
}

/******************* output ********************/


void editorDrawRows(struct abuf* ab)
{
    char str[MAX_BF_ESEQ_SIZE];   
    int y;

    int multiLineComment = 0;
    
    for ( y=0;y<E.screenrows;y++) {

       int filerow = E.rowoff + y;
    	   
         if ( filerow >=  E.numrows  ) {

	       if ( E.numrows == 0 && y == E.screenrows / 3) {
         	  	  	  
	  	     char welcome[80];
	  	     int welcomelen = snprintf(welcome,sizeof(welcome),"KILO EDITOR -- version %s",KILO_VERSION);
	  	     if ( welcomelen > E.screencols ) welcomelen = E.screencols;
	  	     int padding = ( E.screencols - welcomelen ) / 2;
	  	     if ( padding ) {
	  	       abAppend(ab,"~",1);  
	  	       padding--;  
	  	     }  
	  	     while ( padding-- ) abAppend(ab," ",1);
	  	     abAppend(ab,welcome,welcomelen);
	  	 
	  	   }
	  	   else {
	  	   abAppend(ab,"~",1);
	  	   }	

        } else {

            int len = E.row[filerow].rsize - E.coloff;
            int j;
            char c;
            int hlresult;
                        
            if ( len < 0 ) len = 0;
            if ( len > E.screencols ) len = E.screencols;

            
            for ( j = 0 ; j < len ; j++) {

            /*
            hlresult = editorRowGetHighLight(&E.row[filerow],E.coloff + j,&str[0]);
                                    
            switch(hlresult){

            case (0):
              c = E.row[filerow].render[E.coloff + j];
              abAppend(ab,&c,1);   
                         
            break;
            case (1):

              abAppend(ab,&str[0],strlen(str));
              c = E.row[filerow].render[E.coloff + j];
              abAppend(ab,&c,1);

            break;
             
            case (2):
              c = E.row[filerow].render[E.coloff + j];
              abAppend(ab,&c,1);   
              multiLineComment = 1;

            break;  
            
            	
            }

            */ 
            if ( editorRowGetHL(&E.row[filerow],E.coloff + j,&str[0])  == 1 ) {
                 
              abAppend(ab,&str[0],strlen(str));
              c = E.row[filerow].render[E.coloff + j];
              abAppend(ab,&c,1);
                            	
            } else {

              c = E.row[filerow].render[E.coloff + j];
              abAppend(ab,&c,1);   
              multiLineComment = 1;	
              }
               
                          	
           }  
            
           //abAppend(ab,&E.row[filerow].render[E.coloff],len);
        	
          }
	  
	if (( multiLineComment != 1 )) abAppend(ab,"\x1b[0m",5);     // reset all hightlight modes
    abAppend(ab,"\x1b[K",3);	                            // erase until end of line
    abAppend(ab,"\r\n",2);
		  
  }		
    
  
}  


void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;
    
    editorScroll();

    abAppend(&ab,ESEQ_CURSOR_HIDE,strlen(ESEQ_CURSOR_HIDE));    
    abAppend(&ab,ESEQ_CURSOR_GOHOME,strlen(ESEQ_CURSOR_GOHOME));  
        
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);
    
    char buf[32];
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",(E.cy - E.rowoff) + 1,( E.rx - E.coloff ) + 1 ); // put cursor at pos cx,cy
    abAppend(&ab,buf,strlen(buf));
    
    abAppend(&ab,ESEQ_CURSOR_SHOW,strlen(ESEQ_CURSOR_SHOW));  
    write(STDOUT_FILENO,ab.b,ab.len);
    
    abFree(&ab);	
}

void editorScroll()
{
  E.rx = E.cx;
  
  if ( E.cy < E.numrows )                E.rx = editorRowCxToRx(&E.row[E.cy],E.cx);    

  if ( E.cy < E.rowoff )                 E.rowoff = E.cy;  
    
  if ( E.cy >= E.rowoff + E.screenrows ) E.rowoff = E.cy - E.screenrows + 1;

  if ( E.rx < E.coloff)                  E.coloff = E.rx;
  	
  if ( E.rx >= E.coloff + E.screencols)  E.coloff = E.rx - E.screencols + 1;
	
}

void editorDrawStatusBar(struct abuf *ab)
{
    int len = 0;
    char status[80],rstatus[80];
    int rlen = 0;
    char *shortFileName = E.filename;

    if (shortFileName) {

      if ( strlen(shortFileName) > 30 ) {
        while ( strlen(shortFileName) > 30 )
          shortFileName++;
        shortFileName[0]='.';
        shortFileName[1]='.';
        shortFileName[2]='.';
      }
    }
      
    abAppend(ab,ESEQ_INVERT_COLOR,strlen(ESEQ_INVERT_COLOR));  
    
    len = snprintf(status,sizeof(status),"%.30s (%d,%d) ft:%s",(shortFileName?shortFileName:"[No name]"),
    E.cy + 1,E.cx +1,(E.filetype?E.filetype:""));
    
    if ( len > E.screencols ) len = E.screencols;

    abAppend(ab,status,len); 

    rlen = snprintf(rstatus,sizeof(rstatus),"%d/%d",E.cy+1,E.numrows);

    while ( len < E.screencols ){

      if ( len == ( E.screencols - rlen ) ){

        abAppend(ab,rstatus,rlen);
      	break;
      }
      else {

        abAppend(ab," ",1);
        len++;
      	
      }
    
    }
        
    abAppend(ab,ESEQ_RESET_MODE,strlen(ESEQ_RESET_MODE));
    abAppend(ab,ESEQ_CR_NL,strlen(ESEQ_CR_NL));
    
}

void editorSetStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(E.statusmsg,sizeof(E.statusmsg),fmt,ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

void editorDrawMessageBar(struct abuf *ab)
{
   abAppend(ab,"\x1b[K",3); // erase until end of line
   int msglen = strlen(E.statusmsg);
   if ( msglen > E.screencols ) msglen = E.screencols;
   if ( msglen && ( time(NULL) - E.statusmsg_time ) < 5 ) 
     abAppend(ab,E.statusmsg,msglen);

}

/**************** file IO *********************/

void editorLoadHLTemplate(){

     strcpy(E.ft.name,"C");
 
     E.ft.var.tokens = malloc (90);
 
     E.ft.var.tokens[0]="int";
     E.ft.var.tokens[1]="float";
     E.ft.var.tokens[2]="double";
     E.ft.var.tokens[3]="char";
     E.ft.var.tokens[4]="short";
     E.ft.var.tokens[5]="long";
     E.ft.var.tokens[6]="void";
     E.ft.var.tokens[7]="signed";
     E.ft.var.tokens[8]="unsigned";
     E.ft.var.tokens[9]="size_t";
     E.ft.var.tokens[10]="ssize_t";
     E.ft.var.tokens[11]="\0";
 
     E.ft.var.fstyle.colorfg =  10;
     E.ft.var.fstyle.colorbg =  0;
     E.ft.var.fstyle.bold    =  'Y';
     E.ft.var.fstyle.italic  =  'N';
     E.ft.var.fstyle.underline= 'N';
     E.ft.var.fstyle.regular=   'N';  
 
     E.ft.keyword.tokens = malloc(200);
 
     E.ft.keyword.tokens[0]="auto";
     E.ft.keyword.tokens[1]="break";
     E.ft.keyword.tokens[2]="case";
     E.ft.keyword.tokens[3]="const";
     E.ft.keyword.tokens[4]="continue";
     E.ft.keyword.tokens[5]="default";
     E.ft.keyword.tokens[6]="do";
     E.ft.keyword.tokens[7]="else";
     E.ft.keyword.tokens[8]="enum";
     E.ft.keyword.tokens[9]="extern";
     E.ft.keyword.tokens[10]="goto";
     E.ft.keyword.tokens[11]="if";
     E.ft.keyword.tokens[12]="register";
     E.ft.keyword.tokens[13]="return";
     E.ft.keyword.tokens[14]="sizeof";
     E.ft.keyword.tokens[15]="static";
     E.ft.keyword.tokens[16]="struct";
     E.ft.keyword.tokens[17]="switch";
     E.ft.keyword.tokens[18]="typedef";
     E.ft.keyword.tokens[19]="union";
     E.ft.keyword.tokens[20]="volatile";
     E.ft.keyword.tokens[21]="while";
     E.ft.keyword.tokens[22]="\0";
 
     E.ft.keyword.fstyle.colorfg =  15;
     E.ft.keyword.fstyle.colorbg =  0;
     E.ft.keyword.fstyle.bold    =  'Y';
     E.ft.keyword.fstyle.italic  =  'N';
     E.ft.keyword.fstyle.underline= 'N';
     E.ft.keyword.fstyle.regular=   'N';
     
     E.ft.preprocessor.tokens = malloc(120);
 
     E.ft.preprocessor.tokens[0]="#define";
     E.ft.preprocessor.tokens[1]="#include";
     E.ft.preprocessor.tokens[2]="#ifdef";
     E.ft.preprocessor.tokens[3]="#undef";
     E.ft.preprocessor.tokens[4]="#ifndef";
     E.ft.preprocessor.tokens[5]="#if";
     E.ft.preprocessor.tokens[6]="#else";
     E.ft.preprocessor.tokens[7]="#elif";
     E.ft.preprocessor.tokens[8]="#endif";
     E.ft.preprocessor.tokens[9]="#error";
     E.ft.preprocessor.tokens[10]="#pragma";
     E.ft.preprocessor.tokens[11]="\0";
 
     E.ft.preprocessor.fstyle.colorfg =    15;
     E.ft.preprocessor.fstyle.colorbg =     0;
     E.ft.preprocessor.fstyle.bold    =   'Y';
     E.ft.preprocessor.fstyle.italic =    'N';
     E.ft.preprocessor.fstyle.underline = 'N';
     E.ft.preprocessor.fstyle.regular =   'N';
 
     E.ft.number.fstyle.colorfg =  156;
     E.ft.number.fstyle.colorbg =  0;
     E.ft.number.fstyle.bold    =  'N';
     E.ft.number.fstyle.italic  =  'N';
     E.ft.number.fstyle.underline= 'N';
     E.ft.number.fstyle.regular=   'N'; 
 
     
     E.ft.operator.tokens = malloc(200);
         
     E.ft.operator.tokens[0]="+";
     E.ft.operator.tokens[1]="-";
     E.ft.operator.tokens[2]="*";
     E.ft.operator.tokens[3]="/";
     E.ft.operator.tokens[4]="%";
     E.ft.operator.tokens[5]="++";
     E.ft.operator.tokens[6]="--";
     E.ft.operator.tokens[7]="==";
     E.ft.operator.tokens[8]="!=";
     E.ft.operator.tokens[9]=">";
     E.ft.operator.tokens[10]="<";
     E.ft.operator.tokens[11]=">=";
     E.ft.operator.tokens[12]="<=";
     E.ft.operator.tokens[13]="<=>";
     E.ft.operator.tokens[14]="!";
     E.ft.operator.tokens[15]="&&";
     E.ft.operator.tokens[16]="||";
     E.ft.operator.tokens[17]="~";
     E.ft.operator.tokens[18]="&";
     E.ft.operator.tokens[19]="|";
     E.ft.operator.tokens[20]="^";
     E.ft.operator.tokens[21]="<<";
     E.ft.operator.tokens[22]=">>";
     E.ft.operator.tokens[23]="=";
     E.ft.operator.tokens[24]="+=";
     E.ft.operator.tokens[25]="-=";
     E.ft.operator.tokens[26]="*=";
     E.ft.operator.tokens[27]="/=";
     E.ft.operator.tokens[28]="%=";
     E.ft.operator.tokens[29]="&=";
     E.ft.operator.tokens[30]="|=";
     E.ft.operator.tokens[31]="<<=";
     E.ft.operator.tokens[32]=">>=";
     E.ft.operator.tokens[33]="->";
     E.ft.operator.tokens[34]=".";
     E.ft.operator.tokens[35]="->*";
     E.ft.operator.tokens[36]=".*";
     E.ft.operator.tokens[37]=":";
     E.ft.operator.tokens[38]=";";
     E.ft.operator.tokens[39]=",";
     E.ft.operator.tokens[40]="\0";
 
     E.ft.operator.fstyle.colorfg =  14;
     E.ft.operator.fstyle.colorbg =  0;
     E.ft.operator.fstyle.bold    =  'N';
     E.ft.operator.fstyle.italic  =  'N';
     E.ft.operator.fstyle.underline= 'N';
     E.ft.operator.fstyle.regular=   'Y'; 
 
     E.ft.upper.fstyle.colorfg =  9;
     E.ft.upper.fstyle.colorbg =  0;
     E.ft.upper.fstyle.bold    =  'Y';
     E.ft.upper.fstyle.italic  =  'N';
     E.ft.upper.fstyle.underline= 'N';
     E.ft.upper.fstyle.regular=   'N';
 
     E.ft.str.fstyle.colorfg =  2;
     E.ft.str.fstyle.colorbg =  0;
     E.ft.str.fstyle.bold    =  'N';
     E.ft.str.fstyle.italic  =  'N';
     E.ft.str.fstyle.underline= 'N';
     E.ft.str.fstyle.regular=   'Y';

}

void editorOpen(char *filename)
{
    free(E.filename);
    E.filename = strdup(filename);

    //TODO : match against known file types
    E.filetype = strrchr(filename,'.');
    if ( E.filetype != NULL ) {
     E.filetype++;
     if ( *E.filetype == '\0')
      E.filetype = NULL;
    } else
      E.filetype = NULL; 
      
    FILE *fp = fopen(filename,"r");

    if (!fp) die("fopen");

    char *line= NULL;

    ssize_t linelen;
    size_t  linecap;

    while ( ( linelen = getline(&line,&linecap,fp) ) != -1 ) {

      while ( ( linelen > 0 ) && ( ( line[ linelen - 1 ] == '\n'  ) || ( line[linelen -1] ) == '\r' ) ) 
        linelen--;
    	
      editorAppendRow(line,linelen,E.numrows);    
    }

    
    
    E.dirty = 0;
    free(line);
    fclose(fp);
}

char *editorRowsToString(int *buflen)
{
    int j;
    int totlen = 0;
    char *filestr, *p;
    
    for ( j = 0; j < E.numrows; j++) 
      totlen += E.row[j].size + 1;  

    p = filestr = malloc(totlen);

    for ( j = 0; j < E.numrows; j++) {
    
      memcpy(p,E.row[j].chars,E.row[j].size);
      p += E.row[j].size;
      *p='\n';
      p++;
    }
    
    *buflen = totlen;
    return filestr;
}

void editorSave()
{
    if ( E.filename  == NULL )  
      E.filename = editorPrompt("Save as:",NULL);

    if ( E.filename == NULL ) {

      editorSetStatusMessage("Save Aborted");
      return;
    	
    }
    
    int len;
    int fd;
    char *buf=editorRowsToString(&len);

    if ( ( fd = open(E.filename, O_RDWR | O_CREAT, 0644) ) != -1) {

      if ( ftruncate(fd,len) != -1) { 

        if ( write(fd,buf,len) == len) {

          editorSetStatusMessage("%d Bytes Written to Disk",len);
          close(fd);
          free(buf);
          E.dirty = 0;
          return;
        }
      }
        
      close(fd);
    }
    editorSetStatusMessage("Cant Save. I/O Error : %s ",strerror(errno)); 
    free(buf);
}

/*******************   row operations   ***************/
void editorAppendRow(char *s, size_t len,int pos)
{
    int linesToShiftDown = E.numrows - pos;

    if ( ( E.row = realloc(E.row,sizeof(erow)*(E.numrows+1)) ) == NULL) die("realloc");
    
    if ( linesToShiftDown > 0) editorRowShiftDown(linesToShiftDown);  
    
    E.row[pos].size = len;
    E.row[pos].chars = malloc(len + 1);
    memcpy(E.row[pos].chars,s,len);
    E.row[pos].chars[len]='\0';
    E.row[pos].rsize = 0;
    E.row[pos].render = NULL; 
    E.row[pos].hl = NULL;
           
    editorUpdateRowRender(&E.row[pos]);
    editorUpdateRowHL(&E.row[pos]);
    
	E.numrows++;
}

void editorUpdateRowRender(erow* row)
{
    int j;
    int tabs = 0;
    
    for ( j =0;j<row->size;j++){
      if ( row->chars[j] == '\t' ) tabs++;	
    } 

    free(row->render);
    row->render = malloc(row->size + ( tabs * ( KILO_TAB_STOP - 1 )  + 1 ) );

    int idx = 0;

    for (j=0;j<row->size;j++){

      if ( row->chars[j] == '\t') {

        row->render[idx++] = ' ';	

        while ( idx % KILO_TAB_STOP != 0 ) {
          row->render[idx++] = ' ';	
        }  
        
      }           	
      else 
        row->render[idx++] = row->chars[j];	
    }

    row->render[idx]='\0';
    row->rsize = idx;
    
}


int editorRowCxToRx(erow *row,int cx)
{
    int rx = 0;
    int j;

    for (j=0;j<cx;j++){

      if ( row->chars[j] == '\t')
        rx += ( KILO_TAB_STOP - 1) - ( rx % KILO_TAB_STOP );
      rx++;         
    }

    return rx;
}



/**********************  row operations   ********************/

void editorRowInsertChar(erow *row,int at,int c)
{
    if ( at < 0 || at > row->size ) at = row->size;
    if ( ( row->chars = realloc(row->chars, row->size + 2) ) == NULL ) die("realloc");
    memmove(&row->chars[at + 1],&row->chars[at],row->size + 1 - at );
    row->size++;
    row->chars[at] = c;
    editorUpdateRowRender(row);
    editorUpdateRowHL(row); 
}

void editorRowDelChar(erow *row,int at)
{
    if ( at < 0 || at > row->size  ) return;

    if ( row->size > 0) {

      if ( at < row->size) {

        memmove(&row->chars[at],&row->chars[at + 1],row->size - at );    
        if ( ( row->chars = realloc(row->chars, row->size ) ) == NULL ) die("realloc");
        row->size--;
        editorUpdateRowRender(row);
        editorUpdateRowHL(row);  
        E.dirty++; 
            	
      } else if ( at == row->size ) {

          if ( E.cy != ( E.numrows - 1 ) ) {
          	
            editorRowMerge(&E.row[E.cy],E.row[E.cy+1].chars,E.row[E.cy+1].size);  
            editorUpdateRowRender(row);
            editorUpdateRowHL(row);
            editorRowDel(E.cy+1);
            E.dirty++;      	
          } 
        }

      	
    } else if ( row->size == 0 && E.numrows > 1) {

        editorRowDel(E.cy);
        if ( E.cy == E.numrows ) editorMoveCursor(ARROW_UP);
    	E.dirty++;
      }
   	
}

void editorRowShiftDown(int lines)
{
    int j;

     for ( j=1;j <= lines;j++)
        E.row[ E.numrows - j + 1 ] = E.row[ E.numrows - j];
}

void editorRowShiftUp(int lines)
{
    int j;   
    for ( j=0;j < lines;j++)
            E.row[ E.numrows - lines + j - 1 ] = E.row[ E.numrows - lines + j ];
}

void editorRowCut(erow *row,int newsize)
{
    row->chars = realloc(row->chars,newsize + 1); 
    row->size = newsize;
    row->chars[newsize]='\0';
}

void editorRowDel(int pos)
{
    int lines = E.numrows - pos - 1;
    
    free(E.row[pos].chars);
    free(E.row[pos].render);
      
    editorRowShiftUp(lines);
    
    if ( ( E.row = realloc(E.row,sizeof(erow)*(E.numrows)) ) == NULL) die("realloc");

    E.numrows--; 
	
}

void editorRowMerge(erow *row,char *s,int len)
{
    row->chars = realloc(row->chars,row->size + len + 1 );
    memmove(&row->chars[ (row->size) ],s,len);
    row->chars[row->size + len ] = '\0';
    row->size += len;       
}

int editorRowGetHL(erow *row,int pos,char *str) 
{
    char temp[MAX_BF_ESEQ_SIZE];

    strcpy(str,"\0");
    strcpy(temp,"\0");

    if ( GET_HL_CHANGE(row->hl[pos]) ) { 

      if ( GET_HL_ML_COMMENT(row->hl[pos]) ) 
        return 2;
             
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
            
       } else {

       	  ESEQ_SET_FONT_REGULAR(temp);
          strcat(str,temp);

         }
        

        return 1;
      }

      else {

        ESEQ_SET_RESET(str);
        strcat(str,temp); 
      	return 1;
      }
    
    } 

    return 0;
   
}

void editorUpdateRowHLStr(erow *row,hlType *hl)
{
    int j;
    int initpos;
    int size;
          
    for ( j = 0; j < row->rsize ; j++) {

      if ( row->render[j] == '\'' ) {
        size = 1;
        initpos = j;
        while ( row->render[++j] != '\'' && ( size + initpos < row->rsize) ) 
          size++;
        editorUpdateRowHLApply(row,hl,initpos,size+1);
      } 
   }

   for ( j = 0; j < row->rsize ; j++) {
   
         if ( row->render[j] == '\"' ) {
           size = 1;
           initpos = j;
           while ( row->render[++j] != '\"' && ( size + initpos < row->rsize) ) 
             size++;
           editorUpdateRowHLApply(row,hl,initpos,size+1);
           
         } 
      }
}

void editorUpdateRowHLUpper(erow *row,hlType *hl)
{
    int j;
    int initpos;
    int size;
          
    for ( j = 0; j < row->rsize ; j++) {

      if ( isupper(row->render[j]) ) {
        size = 1;
        initpos = j;
        while (isupper(row->render[++j])) 
          size++;

        if ( ( row->render[j] == ' ' || row->render[j] == '\0' || ispunct(row->render[j])) 
            &&
           ( initpos == 0 || row->render[initpos-1] == '_' ||row->render[initpos-1] == ' ' || ispunct(row->render[j]))     
            && 
           ( !( islower(row->render[(initpos>0?initpos-1:initpos)]  ) ) ) ) {

           editorUpdateRowHLApply(row,hl,initpos,size);
             
       }
              
     } 
   }

}

void editorUpdateRowHLNumbers(erow *row,hlType* hl)
{
    int j;
    char *current,*last;
    int pos;
    int initpos;
    int offset;
    int size;
          
    for ( j = 0; j < row->rsize ; j++) {

        if ( isdigit(row->render[j]) ) {
        
          size = 1;

          initpos = j;
          
          while (isdigit(row->render[++j])) size++;

            if ( ( row->render[j] == ' '  || row->render[j] == '\0' || 
                   ispunct(row->render[j])) 
                   &&
                 ( initpos == 0 || row->render[initpos-1] == ' ' || 
                   ispunct(row->render[j]))      ) {

              editorUpdateRowHLApply(row,hl,initpos,size);
             
            }

              
        } 
    }
  
   last = row->render;
   offset = 0;
   
   while ( ( current = strstr(last,"0x") ) != NULL ){

     size = 2;

     pos = ( current - last) + offset; 

     initpos = pos;

     while (isxdigit(row->render[(pos++) + 2])) size++;

     pos--;
     
     if ( ( row->render[(pos) + 2] == ' ' || row->render[(pos) + 2] == '\0' ) && 
          ( initpos == 0 || row->render[initpos - 1] == ' ' )  ) {

       editorUpdateRowHLApply(row,hl,initpos,size);
      
     	
     } else {

         for ( j = 0;j < size ; j++ ){
           SET_HL_CHANGEON(row->hl[initpos + j]);
           SET_HL_RESET(row->hl[initpos + j]);
         }
     
       }

   last = current;
   last++;
   offset = initpos + 1;  
   }

   
}
void editorUpdateRowHL(erow *row)
{
    int j = 0;
    int pos;
    static int commented;
    char *current,*last;
        
    row->hl = realloc( row->hl,row->rsize * sizeof(__int32_t));

     
    for ( j = 0; j < row->rsize ; j++) {

      if (row->render[j] == '_')  {

        if ( isupper( row->render[j>0?j-1:j]) 
        || isupper(row->render[j<row->rsize?j+1:j])) {

          SET_HL_CHANGEON(row->hl[j]);
          SET_FT_BOLD(row->hl[j]);
          SET_FG_COLOR(row->hl[j],9);
      	}       
     
      }
      else {
      	  SET_HL_CHANGEON(row->hl[j]);
      	  SET_HL_RESET(row->hl[j]);	
      }
     }
    
      
    editorUpdateRowHLTokens(row,&E.ft.var);
    editorUpdateRowHLTokens(row,&E.ft.keyword);
    editorUpdateRowHLTokens(row,&E.ft.preprocessor);
    editorUpdateRowHLNumbers(row,&E.ft.number);
    editorUpdateRowHLOps(row,&E.ft.operator);
    editorUpdateRowHLUpper(row,&E.ft.upper);
    editorUpdateRowHLStr(row,&E.ft.str);
     

   last = row->render; 
   
   int begin,end;
   int offset = 0;
   
   //if ( ( current = strstr(last,"/*") ) != NULL ) {
   while ( ( current = strstr(last,"/*") ) != NULL ) {      

         begin = ( current - last ) + offset;

         if ( ( current = strstr(last,"*/") )!= NULL ) {

            end = ( current - last ) + offset;
                        
            if ( end > begin ) {

              end = ( current - last  ) + 2 + offset;
              commented = 0;
              offset = end - 1;

                for (j = begin ; j < end ; j++ ) {
                   SET_HL_CHANGEON(row->hl[j]);
                   SET_FG_COLOR(row->hl[j],3);
                   SET_FT_ITALIC(row->hl[j]);
                }
 
              last = current;
              last++;
              
              continue; 	

            } else {
                
                end = row->rsize;
                commented = 1;
                SET_HL_CHANGEON(row->hl[begin]);
                SET_HL_ML_COMMENT(row->hl[begin]);

                last = current;
                last++;
            }
                
         } else {
                
                end = row->rsize;
            
                for (j = begin ; j < end ; j++ ) {
                   SET_HL_CHANGEON(row->hl[j]);
                   SET_FG_COLOR(row->hl[j],3);
                   SET_FT_ITALIC(row->hl[j]);
                }

         	commented = 1;
            SET_HL_CHANGEON(row->hl[begin]);
         	SET_HL_ML_COMMENT(row->hl[begin]);
         	break;
         }

         
         for (j = begin ; j < end ; j++ ) {
           SET_HL_CHANGEON(row->hl[j]);
           SET_FG_COLOR(row->hl[j],3);
           SET_FT_ITALIC(row->hl[j]);
         }

       
    }
    
    
        
       if ( ( current = strstr(row->render,"//") ) != NULL ) {
   
         pos = current - row->render;
         for (j = pos ; j < row->size ; j++ ) {
   
         SET_HL_CHANGEON(row->hl[j]);
         SET_FG_COLOR(row->hl[j],3);
         SET_FT_ITALIC(row->hl[j]);
               	
         }
        
       return;	
       }



}   

void editorUpdateRowHLApply(erow *row,hlType *hl,int pos, int size)
{
  int i;
  if ( pos < 0 || size <= 0 || pos + size > row->rsize ) return;
   
  for ( i = 0; i < size; i++) {
    SET_HL_CHANGEON(row->hl[pos+i]);
    if (hl->fstyle.bold        == 'Y' ) SET_FT_BOLD(row->hl[pos+i]);
    if (hl->fstyle.italic      == 'Y' ) SET_FT_ITALIC(row->hl[pos+i]);
    if (hl->fstyle.underline   == 'Y' ) SET_FT_UNDERLINE(row->hl[pos+i]);
    if (hl->fstyle.regular     == 'Y' ) SET_FT_REGULAR(row->hl[pos+i]);
    SET_FG_COLOR(row->hl[pos+i],hl->fstyle.colorfg);
    SET_BG_COLOR(row->hl[pos+i],hl->fstyle.colorbg);  
  }
}

void editorUpdateRowHLOps(erow *row,hlType *hl)
{
  
    int count = 0;
    int size;
    int offset;
    int pos;
    char *last,*current;
        
    last = row->render;
    offset = 0;

    if ( hl->tokens == NULL ) return;
    
    while  ( strcmp(hl->tokens[count],"\0") != 0 ){

      while ( ( current = strstr(last,hl->tokens[count]) ) != NULL ){

        pos = current - last + offset;
        size = strlen(hl->tokens[count]);
        editorUpdateRowHLApply(row,hl,pos,size);
      
        offset = pos + 1;
        last = current;
        last++;
      }   

      last = row->render;
      offset = 0;
      count++; 
   }
}

void editorUpdateRowHLTokens(erow *row,hlType* hl)
{

    int count = 0;
    int size;
    int offset;
    int pos;
    char *last,*current;
        
    last = row->render;
    offset = 0;

    if ( hl->tokens == NULL ) return;
    
    while  ( strcmp(hl->tokens[count],"\0") != 0 ){

      while ( ( current = strstr(last,hl->tokens[count]) ) != NULL ){

        pos = current - last + offset;
        size = strlen(hl->tokens[count]);
        char leftchr,rightchr;

        leftchr  = (pos>0?row->render[pos-1]:'*');
        rightchr = (( pos + size )<row->rsize?row->render[pos+size]:'*');
                      
        if (  ( isalnum(leftchr) == 0 ) && ( isalnum(rightchr) == 0 ) )  
          editorUpdateRowHLApply(row,hl,pos,size);
   
        offset = pos + 1;
        last = current;
        last++;
      }   

     last = row->render;
     offset = 0;
     count++; 
   }

}    

/**********************  editor operations ******************/

void editorInsertChar(int c)
{
    if ( E.cy == E.numrows) editorAppendRow("",0,E.cy);
    editorRowInsertChar(&E.row[E.cy],E.cx,c);
    E.cx++;
    E.dirty++;
}

void editorRowInsert()
{
    editorAppendRow(&E.row[E.cy].chars[E.cx],strlen(&E.row[E.cy].chars[E.cx]),E.cy + 1);
    if ( E.cx < E.row[E.cy].size  ) editorRowCut(&E.row[E.cy],E.cx);
    editorUpdateRowRender(&E.row[E.cy]);
    editorUpdateRowHL(&E.row[E.cy]);
    E.dirty++;
}

void editorFind()
{
    char *promptstr = editorPrompt("Find:",editorFindCallBack);
    editorSetStatusMessage("");
    free(promptstr);
}

void editorFindCallBack(const char *input,const int key)
{

    int c;
    int j = E.cy;
    char *pos = NULL;
    int count = 0;
    size_t forward = 1;
 
    int oldCx = E.cx;
    int oldCy = E.cy;
    int oldRowOff = E.rowoff;
    int oldColOff = E.coloff;
    
    do {

      pos = strstr(E.row[j].chars,input);

      if ( pos != NULL) {

        E.cy = j;
        E.cx = pos - E.row[j].chars;
        E.rowoff = E.cy;
        E.coloff = E.cx;
        editorScroll();
        editorSetStatusMessage("Arrows to Search Again: %s, Esc to quit",input);
        editorRefreshScreen(); 
        pos = NULL;
        count = 0;
        
        c = editorReadKey();

        switch (c){

        case CTRL_KEY('q'):
        case ESCAPE:
          E.cx = oldCx;
          E.cy = oldCy;
          E.rowoff = oldRowOff;
          E.coloff = oldColOff;
          return;
        break;
        case PAGE_UP:
        case ARROW_UP:
          forward = 0;
        break;
        case ARROW_DOWN:
        default:
          forward = 1;
        break;
               	
        }
                      	
      }  
          
      j += ( forward ? 1 : -1 );
      count++;

      if ( count > E.numrows ) {
       editorSetStatusMessage("String Not Found");
       return;
     } 
    
     if ( ( j == E.numrows ) && forward )   j = 0;
     if ( ( j == -1 && forward == 0 ))      j = E.numrows - 1;                                 
               
     } while (1); 

}

/*******************   memory management  **************/

void freeBuffers()
{
    while ( ( --E.numrows ) >= 0 ) {

      free(E.row[E.numrows].chars);
      free(E.row[E.numrows].render);
      free(E.row[E.numrows].hl);
    } 

	free(E.row);
	free(E.filename);
	free(E.ft.var.tokens);
    free(E.ft.keyword.tokens);
    free(E.ft.preprocessor.tokens);
    free(E.ft.operator.tokens);
}

/********************   Signal Handling  ****************/

void sigHandlerSIGWINCH()
{
    E.cx = E.cy = E.rowoff = E.coloff = 0;
    if ( getWindowSize(&E.screenrows,&E.screencols) == -1 ) die("getWindowSize");   
    E.screenrows -= 2; // opening space for status bar and message bar
}

void sigHandlerSIGTERM()
{
    die("Terminated with SIGTERM Cleanly");
}

/********************   Main ****************************/

int main(int argc, char **argv)
{
    signal(SIGWINCH, sigHandlerSIGWINCH);
    signal(SIGTERM,  sigHandlerSIGTERM);
    enableRawMode();
	initEditor();
    
    atexit(disableRawMode);
    atexit(freeBuffers);
    
    if ( argc >= 2 ) editorOpen(argv[1]);
    else editorAppendRow("",0,E.numrows);
    
    editorSetStatusMessage("HELP: CTRL-Q to exit | CTRL-S to save");
    	
	while(1) {
	  
	  editorRefreshScreen();
	  editorProcessKey();
	}	
	
	return 0;

}

