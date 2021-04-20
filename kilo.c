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

} erow;

typedef struct {

    int    rx;
    int    cx,cy;
    int    screenrows,screencols;
    int    numrows;
    int    rowoff,coloff;
    int    mousex,mousey;
    erow   *row;
    char   *filename;
    char   statusmsg[80];
    time_t statusmsg_time; 
    int    dirty;
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
    DEL_KEY,PAGE_UP,PAGE_DOWN,END_KEY,HOME_KEY,RETURN,ESCAPE,
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
void editorProcessKey();
char *editorPrompt(char *prompt);

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

// row operations
void editorAppendRow(char *s, size_t len,int pos);
void editorUpdateRow(erow *row);
int  editorRowCxToRx(erow *row,int cx);

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
	case ARROW_UP:
	  if ( E.cy > 0 ) E.cy--;
	break;
    case MOUSE_SCROLL_DOWN:
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
	default:
	  editorInsertChar(c);
	break;
    }

    quit_times = KILO_QUIT_TIMES;
}

char *editorPrompt(char *prompt)
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
    int y;
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
            if ( len < 0 ) len = 0;
            if ( len > E.screencols ) len = E.screencols;
                 
            abAppend(ab,&E.row[filerow].render[E.coloff],len); 

        	
          }
	  
	abAppend(ab,"\x1b[K",3);	  // erase until end of line
	abAppend(ab,"\r\n",2);
		  
	}		
    
   
}

void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;
    
    editorScroll();

    abAppend(&ab,"\x1b[?251",6);    // hide cursor
    //abAppend(&ab,"\x1b[2J",4);      // erase entire screen
    abAppend(&ab,"\x1b[H",3);       // put cursor at home 
        
    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);
    
    char buf[32];
    snprintf(buf,sizeof(buf),"\x1b[%d;%dH",(E.cy - E.rowoff) + 1,( E.rx - E.coloff ) + 1 ); // put cursor at pos cx,cy
    abAppend(&ab,buf,strlen(buf));
    
    
    abAppend(&ab,"\x1b[?25h",6);     // show cursor
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
    // inverted printed colors
    abAppend(ab,"\x1b[7m",4);
    int rlen = 0;
    len = snprintf(status,sizeof(status),"%.30s (%d,%d)",(E.filename?E.filename:"[No name]"),
    E.cy + 1,E.cx +1);

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
    // go back to normal color printing
    abAppend(ab,"\x1b[m",3);
    abAppend(ab,"\r\n",2);
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

void editorOpen(char *filename)
{
    free(E.filename);
    E.filename = strdup(filename);
   
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
      E.filename = editorPrompt("Save as:");

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
           
    editorUpdateRow(&E.row[pos]);
    
	E.numrows++;
}

void editorUpdateRow(erow* row)
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
    editorUpdateRow(row); 
}

void editorRowDelChar(erow *row,int at)
{
    if ( at < 0 || at > row->size  ) return;

    if ( row->size > 0) {

      if ( at < row->size) {

        memmove(&row->chars[at],&row->chars[at + 1],row->size - at );    
        if ( ( row->chars = realloc(row->chars, row->size ) ) == NULL ) die("realloc");
        row->size--;
        editorUpdateRow(row);  
        E.dirty++; 
            	
      } else if ( at == row->size ) {

          if ( E.cy != ( E.numrows - 1 ) ) {
          	
            editorRowMerge(&E.row[E.cy],E.row[E.cy+1].chars,E.row[E.cy+1].size);  
            editorRowDel(E.cy+1);
            editorUpdateRow(row);
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
  editorUpdateRow(&E.row[E.cy]);
  E.dirty++;
}
/*******************   memory management  **************/

void freeBuffers()
{
    while ( ( --E.numrows ) >= 0 ) {

      free(E.row[E.numrows].chars);
      free(E.row[E.numrows].render);
    } 

	free(E.row);
	free(E.filename);
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

