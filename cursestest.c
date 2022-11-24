// gcc cursestest.c -l curses
#include <ncurses.h>
#include <stdio.h>

typedef struct loc
{
  char val;
} loc;

struct loc board[8][8];
unsigned int cols = 8;
unsigned int rows = 8;
const char boardlayout[8][8] = {
  {'o','*','o','o','*','o','o','o'},
  {'o','o','o','o','o','*','o','o'},
  {'o','o','*','o','o','*','o','o'},
  {'o','o','o','o','*','o','o','o'},
  {'o','*','o','o','o','o','*','o'},
  {'o','o','o','o','*','o','o','o'},
  {'o','o','o','o','o','*','o','o'},
  {'o','o','o','o','o','o','o','o'}
};

static void print_board(WINDOW * win, int curx, int cury);

static void initboard();

int main()
{
  initboard();
  initscr();
  clear();
  noecho();
  cbreak();
  WINDOW * gamewindows = newwin(rows *2, cols*2, 0,0);
  int ch;
  int currow = 0, curcol = 0;
  keypad(gamewindows, true);
  print_board(gamewindows, currow,curcol);
  while((ch = wgetch(gamewindows)))
  {
    int cccpy = curcol, crcpy = curcol;
    switch(ch)
    {
      case KEY_UP:
        currow--;
        break;
      case KEY_DOWN:
        currow++;
        break;
      case KEY_LEFT:
        curcol--;
        break;
      case KEY_RIGHT:
        curcol++;
        break;
      case KEY_BACKSPACE:
        goto end;
      default:
        continue;
    }
    if(curcol < 0 || curcol > cols -1 || currow < 0 || currow > rows -1)
    {
      curcol = cccpy;
      currow = crcpy;
      continue;
    }
    clear();
    wmove(gamewindows,0,0);
    print_board(gamewindows, currow,curcol);
    // wrefresh(gamewindows);

  }
 end: 

  delwin(gamewindows);
  endwin();
}


void initboard()
{
  for(int r = 0; r < rows; r++)
  {
    for(int c = 0; c < cols; c++)
    {
      board[r][c].val = boardlayout[r][c];
    }
  }
}


void print_board(WINDOW * win, int curx, int cury)
{
  for(int r = 0; r < rows; r++){
    for(int c = 0; c < cols; c++){
      if(r == curx && c == cury)
        wattron(win,A_REVERSE);
      wprintw(win, "%c ", boardlayout[r][c]);
      if(r == curx && c == cury)
        wattroff(win,A_REVERSE);
    }
    // wprintw(win,"\n");
  }
  refresh();
}
