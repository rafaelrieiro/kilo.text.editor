/****************** defines *****************/

// https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
#define _GNU_SOURCE 
/* If you define this macro, everything is included: 
*  ISO C89, ISO C99, POSIX.1, POSIX.2, BSD, SVID, X/Open, LFS, and GNU extensions.
*  In the cases where POSIX.1 conflicts with BSD, the POSIX definitions take precedence.*/
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
/* If you define this macro, most features are included apart from X/Open, 
*  LFS and GNU extensions: the effect is to enable features from the 2008 edition of POSIX,
*  as well as certain BSD and SVID features without a separate feature test macro to control them. 
*/#define CTRL_KEY(k) ( k & 0x1f#define ABUF_INIT { NULL , 0 }
#define KILO_VERSION "0.0.1"#define KILO_TAB_STOP 8

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


/************** data *******************/

typedef struct {

    int rsize;
    char *render;
    int size;
    char *chars;

} erow;

struct editorConfig {

    int rx;
    int cx, cy;
    int screenrows,screencols;
    int numrows;
    int rowoff;
    int coloff;
    erow *row;
    char *filename;
    char statusmsg[80];
    time_t statusmsg_time; 
    struct termios orig_termios;

};

struct abuf {
	
	char *b;
	int len;
};

struct editorConfig E;

enum editorKey {

    ARROW_LEFT=1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    END_KEY,
    HOME_KEY,
    MOUSE_SCROLL_DOWN,
    MOUSE_SCROLL_UP
};

/*************** function prototypes *************/

//// terminal
void enableRawMode();
void enableMouseTracking();
void disableRawMode();
int  editorReadKey();

/// error handling
void die(const char *s);

/// output


/************** error handling ****************/

void die(const char *s)
{
	write(STDOUT_FILENO,"\x1b[2J",4);
    write(STDOUT_FILENO,"\x1b[H",3);
	
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
	raw.c_cc[VMIN]  = 1;
	raw.c_cc[VTIME] = 0;
	
	if ( tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1 ) die("tcsetattr");
}

void disableRawMode()
{
	if ( tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios) == -1 ) die("tcsetattr");
    write (STDOUT_FILENO, "\e[?9l", 5);
	write (STDOUT_FILENO, "\e[?1000l", 8); // disable normal tracking mode - send event on press and release

}

int editorReadKey()
{
	char c = '\0';
	int nread;
	
	while  ( ( nread = read(STDIN_FILENO,&c,1) ) == 1 && c != 'q') 
    {
	  //if ( iscntrl(c)) { 
      if (!( c >= 32 && c <= 126 ) ) {
        printf("Hexa: \x3%x \r\n",c);
		
	  }else{
  	     printf("Decimal: %d, Char: ('%c'), Hexa: \x2%x\r\n",c,c,c);	
	   }  
	
    }
    
    return c;	
}

void enableMouseTracking()
{

	 write (STDOUT_FILENO, "\e[?9h", 5);
	 write (STDOUT_FILENO, "\e[?1000h", 8); // normal tracking mode - send event on press and release
}

int main(int argc, char **argv)
{

	enableRawMode();
	enableMouseTracking();
	atexit(disableRawMode);
       
        editorReadKey();
	
	return 0;

}

