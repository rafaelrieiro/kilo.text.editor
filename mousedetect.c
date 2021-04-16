#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int main (void)
{
    unsigned char buff [6];
    unsigned int x, y, btn;
    struct termios original, raw;

    // Save original serial communication configuration for stdin
    tcgetattr( STDIN_FILENO, &original);

    // Put stdin in raw mode so keys get through directly without
    // requiring pressing enter.
    cfmakeraw (&raw);
    tcsetattr (STDIN_FILENO, TCSANOW, &raw);

    // Switch to the alternate buffer screen
    write (STDOUT_FILENO, "\e[?47h", 6);

    // Enable mouse tracking
    write (STDOUT_FILENO, "\e[?9h", 5);
    write (STDOUT_FILENO, "\e[?1000h", 8); // normal tracking mode - send event on press and release
    while (1) {
        read (STDIN_FILENO, &buff, 1);
        if (buff[0] == 3) {
            // User pressd Ctr+C
            break;
        } else if (buff[0] == '\x1B') {
            // We assume all escape sequences received 
            // are mouse coordinates
            read (STDIN_FILENO, &buff, 5);
            btn = buff[2] - 32;
            x = buff[3] - 32;
            y = buff[4] - 32;
            printf ("button:%u\n\rx:%u\n\ry:%u\n\n\r", btn, x, y);
        }
    }

    // Revert the terminal back to its original state
    write (STDOUT_FILENO, "\e[?9l", 5);
    write (STDOUT_FILENO, "\e[?47l", 6);
    tcsetattr (STDIN_FILENO, TCSANOW, &original);
    return 0;
}
