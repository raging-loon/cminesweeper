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



typedef struct box_t
{ 
  int box_type;
  unsigned int num_mines_around;
} __attribute__((packed)) box_t;

// GLOBALS
box_t * gameboard = NULL;
unsigned int columns = 0;
unsigned int rows = 0;
unsigned int number_mines = 0;
FILE * random_number_bag;

// https://github.com/GNOME/gnome-mines/blob/master/src/minefield.vala#L49
int neighbor_map[8][2] = {
    { -1,  1 },
    {  0,  1 },
    {  1,  1 },
    {  1,  0 },
    {  1, -1 },
    {  0, -1 },
    { -1, -1 },
    { -1,  0 }
};



// #define GET_LOC(ROW, COL) gameboard[(ROW * sizeof(box_t)) + (COL * sizeof(box_t)) * sizeof(box_t)]
#define GET_LOC(ROW, COL) gameboard[((ROW + COL + sizeof(box_t)) * sizeof(box_t))]

/**
 * Rules:
 *  1. If the player hits a mine, the game is over
 *  2. If the player places flags on all of the mines, 
 *     the game is won.
 *  3. If the player uncovers all non-mine boxes, the game is won
 * 
 */

static void debug_dump_board_info();
static int generate_board(unsigned int num_mines, unsigned int num_cols, unsigned int num_rows);
static void init_board(unsigned int num_cols, unsigned int num_rows);
static void free_board();
static int calculate_surrounding_mines(const box_t * const loc, unsigned int row, unsigned int col);
static void get_surrounding_mines(unsigned int row, unsigned int col);
static void parse_options(int argc, char ** argv);
static unsigned int get_random_number(unsigned int max);


int main(int argc, char ** argv)
{
  random_number_bag = fopen("/dev/urandom","rb");
  if(!random_number_bag)
  {
    printf("Failed to open random number bag: cminesweeper.c:%d\n",__LINE__);
    exit(1);
  }
  init_board(EASY_COLS, EASY_ROWS);
  
  if(generate_board(EASY_NUM_MINES, EASY_COLS, EASY_ROWS) == -1)
  {
    printf("Failed to generate board\n.");
    free_board(); // just to be safe.
    exit(1);
  }
  
  printf("%p\n",gameboard);

  get_surrounding_mines(rows, columns);
  debug_dump_board_info();
  free_board();
  fclose(random_number_bag);
} 


int generate_board(unsigned int num_mines, unsigned int num_cols, unsigned int num_rows)
{
  if(!gameboard) return -1;
  number_mines = num_mines;
  
  
  int mines_placed = 0;
  while(mines_placed != num_mines)
  {
    
    int x = get_random_number(num_rows);
    int y = get_random_number(num_cols);
    // printf("%d:%d\n",x,y);

    box_t * loc = &GET_LOC(x,y);
    if(loc->box_type != BOX_TYPE_MINE)
    {
      
      loc->box_type = BOX_TYPE_MINE;

      mines_placed++;
    } else continue;
  }

  

  return 0;
}


void init_board(unsigned int num_cols, unsigned int num_rows)
{
  gameboard = (box_t *)malloc(sizeof(box_t) * (num_cols * num_rows));
  memset(gameboard, 0, sizeof(box_t) * (num_cols * num_rows));
  columns = num_cols;
  rows = num_rows;
  for(int row = 0; row < rows; row++)
  {
    for(int col = 0; col < columns; col++)
    {
      box_t * cloc = &GET_LOC(row, col);
      cloc->box_type = 0;
      cloc->num_mines_around = 0;
    }
  } 
}

void free_board()
{
  if(gameboard) free(gameboard);
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
  for(int row = 0; row < rows; row++)
  {
    for(int col = 0; col < columns; col++)
    {
        
      box_t * loc = &GET_LOC(row,col);
      // printf("%d\n",loc->box_type);
      if(loc->box_type == BOX_TYPE_MINE){
        printf("\033[31m* \033[0m");
      }
      else {
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
      
      box_t * curloc = &GET_LOC(r,c);
      if(curloc->box_type != BOX_TYPE_MINE) curloc->box_type = BOX_TYPE_EMPTY;
      curloc->num_mines_around = 0;
      if(curloc->box_type == BOX_TYPE_EMPTY)
        curloc->num_mines_around = calculate_surrounding_mines(curloc, r, c);
    }
  }
}




int calculate_surrounding_mines(const box_t * const loc, unsigned int row, unsigned int col)
{
  int num_mines_found = 0;
  if(loc->box_type == BOX_TYPE_MINE) return num_mines_found;

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


  for(int i = 0; i < 9; i++)
  {
    int nx = neighbor_map[i][0];
    int ny = neighbor_map[i][1];

    int ncol = ny + col;
    int nrow = nx + row;

    if((ncol < 0 || ncol > columns) || (nrow < 0 || nrow > rows)) 
    {
      // do nothign
    }
    else {
      const box_t * current_loc = &GET_LOC(ncol, nrow);
      if(current_loc->box_type == BOX_TYPE_MINE) num_mines_found++;

    }
  }

  
  
  

  return num_mines_found;
}