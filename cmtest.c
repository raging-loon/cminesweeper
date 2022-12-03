// This file is licensed under GPLv3 <https://www.gnu.org/licenses/>
// Copyright Conner Macolley, 2022
#include <stdio.h>
#include <ncurses.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define BOX_TYPE_EMPTY    0
#define BOX_TYPE_MINE     1

#define EASY_NUM_MINES    10
#define EASY_COLS         8
#define EASY_ROWS         8

#define MED_NUM_MINES    40
#define MED_COLS         16
#define MED_ROWS         16

#define HARD_NUM_MINES    99
#define HARD_COLS         30
#define HARD_ROWS         16

#define REVEALED_COLOR    0x19
#define FLAG_COLOR        0x90

typedef struct gbox
{ 
  int box_type;
  unsigned int num_mines_around;
	bool is_revealed;
	bool is_flagged;
} gbox;

typedef struct gameboard
{
  gbox * board;
  gbox ** mines;
  unsigned int columns;
  unsigned int size;
  unsigned int rows;
  unsigned int number_mines;
  unsigned int num_places_revealed;
  unsigned int flags_placed;
  unsigned int num_mines_flagged;
} gameboard;  

gameboard gboard;

// GLOBALS
FILE * random_number_bag;
WINDOW * gamewindow;

// https://github.com/GNOME/gnome-mines/blob/master/src/minefield.vala#L49
const int neighbor_map[8][2] = {
    {-1, -1},
    {-1,  0},
    {-1,  1},
    { 0, -1},
    { 0,  1},
    { 1, -1},
    { 1,  0},
    { 1,  1}
};



// #define GET_LOC(ROW, COL) gameboard[(ROW  + (COL))]
#define GET_LOC(r, c) (gboard.board[((r) * (gboard.columns)) + (c)])
// #define GET_LOC(ROW, COL) gameboard[((ROW + COL + sizeof(gbox)) * sizeof(gbox))]

/**
 * Rules:
 *  1. If the player hits a mine, the game is over
 *  2. If the player places flags on all of the mines, 
 *     the game is won.
 *  3. If the player uncovers all non-mine boxes, the game is won
 * 
 */

void debug_dump_board_info();
int generate_board(unsigned int num_mines, unsigned int num_cols, unsigned int num_rows);
void init_board(unsigned int num_cols, unsigned int num_rows);
void free_board();
int calculate_surrounding_mines(const gbox * const loc, unsigned int row, unsigned int col);
void get_surrounding_mines(unsigned int row, unsigned int col);
void parse_options(int argc, char ** argv);
unsigned int get_random_number(unsigned int max);
void init_window();
void cleanup();
void gameover();
void nc_print_board(WINDOW * win, int curx, int cury);
void movement_handler();

void set_flag(int x, int y);
void reveal_location(int x, int y);

bool checkwin();
void wingame();


int main(int argc, char ** argv)
{
  random_number_bag = fopen("/dev/urandom","rb");
  if(!random_number_bag)
  {
    printf("Failed to open random number bag: cminesweeper.c:%d\n",__LINE__);
    exit(1);
  }
  parse_options(argc, argv);
	get_surrounding_mines(gboard.rows, gboard.columns);

	initscr();
  clear();
  noecho();
  cbreak();
	init_window();

	keypad(gamewindow, true);
  start_color();

  init_pair(REVEALED_COLOR, COLOR_BLACK, COLOR_BLUE);
  init_pair(FLAG_COLOR, COLOR_BLACK, COLOR_RED);

	movement_handler();
	getch();

	cleanup();
} 







void parse_options(int argc, char ** argv)
{
  int ccol = 0, crow = 0, cmines = 0;

  if(argc == 1 || strcmp(argv[1], "medium") == 0)
  {
    ccol =   MED_COLS;
    crow =   MED_ROWS;
    cmines = MED_NUM_MINES;
  }
  else if(strcmp(argv[1], "hard") == 0)
  {
    ccol =   HARD_COLS;
    crow =   HARD_ROWS;
    cmines = HARD_NUM_MINES;
  } 
  else if(strcmp(argv[1], "easy") == 0)
  {
    ccol =   EASY_COLS;
    crow =   EASY_ROWS;
    cmines = EASY_NUM_MINES;
  }
  else if(strcmp(argv[1], "help") == 0)
  {
    printf("'a' -> clear spot\n'f' -> place a flag\n'q' -> exit\n");
    cleanup();
    exit(0);
  } else {
    printf("Unknown difficulty\n");
    exit(1);
  }

  init_board(ccol, crow);
  
  if(generate_board(cmines, ccol, crow) == -1)
  {
    printf("Failed to generate board\n.");
    free_board(); // just to be safe.
    exit(1);
  }
  
}


int generate_board(unsigned int num_mines, unsigned int num_cols, unsigned int num_rows)
{
  if(!gboard.board) return -1;
  gboard.number_mines = num_mines;
  
  // we are holding pointers to the mines which is why we
  // use sizeof(gbox *) and not sizeof(gbox)
  gboard.mines = (gbox **)malloc(sizeof(gbox **) * num_mines);

  int mines_placed = 0;
  

  for(int i = 0; i < num_mines; ){
     int x = get_random_number(num_rows);
    int y = get_random_number(num_cols);
    // printf("%d:%d\n",x,y);

    gbox * loc = &GET_LOC(x,y);
    if(loc->box_type != BOX_TYPE_MINE)
    {
      
      loc->box_type = BOX_TYPE_MINE;
      gboard.mines[i] = (gbox*)&loc;
      i++;
    } else continue;
  }

  

  return 0;
}


void init_board(unsigned int num_cols, unsigned int num_rows)
{
  gboard.board = (gbox *)malloc(sizeof(gbox) * (num_cols * num_rows));
  memset(gboard.board, 0, sizeof(gbox) * (num_cols * num_rows));
  gboard.columns = num_cols;
  gboard.rows = num_rows;

  gboard.size = num_cols * num_rows;
  for(int row = 0; row < gboard.rows; row++)
  {
    for(int col = 0; col < gboard.columns; col++)
    {
      gbox * cloc = &GET_LOC(row, col);
      cloc->box_type = 0;
      cloc->num_mines_around = 0;
    }
  } 
}

void free_board()
{
  if(gboard.board) free(gboard.board);
  if(gboard.mines) free(gboard.mines);
}


unsigned int get_random_number(unsigned int max)
{
  if(!random_number_bag)
  {
    random_number_bag = fopen("/dev/urandom","rb");
    return get_random_number(max);
  }

  unsigned int num;
  fread(&num, 1, sizeof(num),random_number_bag);
  srand(num);

  return rand() % max;
}



void debug_dump_board_info()
{
  for(int row = 0; row < gboard.rows; row++)
  {
    for(int col = 0; col < gboard.columns; col++)
    {
        
      gbox * loc = &GET_LOC(row,col);
      // printf("%d\n",loc->box_type);
      
      if(loc->box_type == BOX_TYPE_MINE){
        printf("\033[31m* \033[0m");
      }
      else if(loc->box_type == BOX_TYPE_EMPTY) {
        if(loc->num_mines_around == 0)
          printf("o " );
        else 
          printf("%d ",loc->num_mines_around);
      }
      
    }
    // printf("\n");
    // for(int col = 0; col < columns * 4 + 1; col++)
    //   printf("-");
    printf("\n");
  }

}

void get_surrounding_mines(unsigned int row, unsigned int col)
{
  for(int r = 0; r < row; r++)
  {
    for(int c = 0; c < col; c++)
    {
      
      gbox * curloc = &GET_LOC(r,c);
      if(curloc->box_type == BOX_TYPE_MINE)
        calculate_surrounding_mines(curloc, r, c);
      
    }
  }
}




int calculate_surrounding_mines(const gbox * const loc, unsigned int row, unsigned int col)
{
  int num_mines_found = 0;
  if(loc->box_type == BOX_TYPE_EMPTY) return num_mines_found;

  /**
   * Assuming the current loc is @, we look here:
   * 
   *  @ o o   o @ o   o o @
   *  o o o   o o o   o o o
   *  o o o   o o o   o o o
   * 
   *  o o o   o o o   o o o
   *  @ o o   o @ o   o o @
   *  o o o   o o o   o o o
   * 
   *  o o o   o o o   o o o
   *  o o o   o o o   o o o
   *  @ o o   o @ o   o o @
   * 
   *      
   *      o o o   ((row-1, col-1),  (row-1, col),  (row-1, col+1))
   *      o @ o   ((row, col-1),    (row, col),    (row, col+1))
   *      o o o   ((row+1,col-1),   (row+1, col),  (row+1,col+1))
   * 
   */


  for(int i = 0; i < 8; i++)
  {
    int nx = neighbor_map[i][0];
    int ny = neighbor_map[i][1];
    int ncol = col + nx;
    int nrow = row + ny;
    /**
     const gbox * current_loc = &GET_LOC(ncol, nrow);
      if(current_loc->box_type == BOX_TYPE_MINE) num_mines_found++;

     * 
     */

    if(nrow >= 0 && nrow <= gboard.rows -1&& ncol >= 0 && ncol <= gboard.columns-1)
    {
      gbox * loc = &GET_LOC(nrow, ncol);
      loc->num_mines_around++; 

    }
  }

  
  
  

  return num_mines_found;
}

void init_window()
{
	printf("\n%dx%d",LINES, COLS);
	if(COLS >= gboard.columns  && LINES >= gboard.rows )
	{
		
		gamewindow = newwin(gboard.rows * 2, gboard.columns * 2, 0, 0);
		refresh();
	} else {
		printf("Screen width must be greater than or equal to %dx%d.\n",gboard.rows * 2, gboard.columns * 2);
		cleanup();
		exit(1);
	}
}

void cleanup()
{
	if(random_number_bag) fclose(random_number_bag);
	if(gamewindow) delwin(gamewindow);
	endwin();
	free_board();
}

// TODO: Actually implement this
void gameover()
{
  cleanup();
  printf("Sorry, you lost :(\n");
  exit(1);
}

void nc_print_board(WINDOW * win, int curx, int cury)
{
	for(int r = 0; r < gboard.rows; r++)
	{
		for(int c = 0; c < gboard.columns; c++)
		{

			if(r == curx && c == cury) wattron(win, A_REVERSE);
			else wattron(win, COLOR_PAIR(REVEALED_COLOR));

			gbox * loc = &GET_LOC(r,c);

			if(loc->is_revealed){
				wprintw(win, "%d ",loc->num_mines_around);
				

			} else {
				if(loc->is_flagged) {
          wattron(win, COLOR_PAIR(FLAG_COLOR));
          wprintw(win,"F ");

        }
				else {
          wattroff(win, COLOR_PAIR(REVEALED_COLOR));
          wprintw(win, "o ");
        }
      }

			// wprintw(win," ");
      wattroff(win, COLOR_PAIR(FLAG_COLOR));
			wattroff(win, A_REVERSE);
      wattroff(win, COLOR_PAIR(REVEALED_COLOR));

		}
	}
	wrefresh(win);
}



void movement_handler()
{
	int ch;
	int currow = 0, curcol = 0;
	nc_print_board(gamewindow, 0, 0);

	while((ch = wgetch(gamewindow)))
	{
		int cccpy = curcol, crcpy = currow;
		switch(ch)
		{
			case 'q':
				cleanup();
				exit(0);
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
			case 'F':
			case 'f':{
				set_flag(currow, curcol);
				break;
			}
      case 'a':
        reveal_location(currow, curcol);
        break;
			default:
				continue;
		}
		
		if(curcol < 0 || curcol > gboard.columns -1 || currow < 0 || currow > gboard.rows -1)
    {
      curcol = cccpy;
      currow = crcpy;
      continue;
    }

		clear();
		wmove(gamewindow, 0, 0);
		nc_print_board(gamewindow, currow, curcol);
    if(checkwin()) wingame();
	}	
}

void set_flag(int x, int y)
{
	gbox * loc = &GET_LOC(x,y);
	// loc->is_flagged = !loc->is_flagged;
  // return;
  if(!loc->is_flagged)
  {
    loc->is_flagged = true; 
    gboard.flags_placed++;
    if(loc->box_type == BOX_TYPE_MINE) gboard.num_mines_flagged++;
  }
  else{
    loc->is_flagged = false; 
    gboard.flags_placed--;
    if(loc->box_type == BOX_TYPE_MINE) gboard.num_mines_flagged--;
  }
}


void reveal_location(int x, int y)
{
  // probably not necessary but better safe than sorry
  if(x > gboard.columns || x < 0 || y > gboard.rows || y < 0) return;

  gbox * loc = &GET_LOC(x,y);

  if(loc->is_flagged) return;

  else if(loc->is_revealed) return;
  
  else if(loc->box_type == BOX_TYPE_MINE) gameover();
  
  else{
    loc->is_revealed = true;
    gboard.num_places_revealed++;
  }
}


bool checkwin()
{
  if(gboard.num_mines_flagged == gboard.number_mines) return true;
  if(gboard.num_places_revealed == gboard.size - gboard.number_mines) return true;
  return false;
}

void wingame()
{
  cleanup();
  printf("\n\nCongrats you win :)\n");
  exit(0);
}