/*
    Arduino Tetris
    Copyright (C) 2015  João André Esteves Vilaça 
    
    https://github.com/vilaca/Handheld-Color-Console

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TETRISCPP
#define TETRISCPP

#include "TFTv2_extended.h"
#include "Arduino.h"
#include "joystick.cpp"
#include "beeping.cpp"


#define LCD_WIDTH 319
#define LCD_HEIGHT 239

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define BOARD_WIDTH 11
#define BOARD_HEIGHT 20

#define BLOCK_SIZE MIN( (LCD_WIDTH-1) / BOARD_WIDTH, (LCD_HEIGHT-1) / BOARD_HEIGHT )

#define BOARD_LEFT      (LCD_WIDTH - BOARD_WIDTH * BLOCK_SIZE)/4*3
#define BOARD_RIGHT      BOARD_LEFT + BLOCK_SIZE * BOARD_WIDTH
#define BOARD_TOP       (LCD_HEIGHT - BOARD_HEIGHT * BLOCK_SIZE) / 2
#define BOARD_BOTTOM     BOARD_TOP + BOARD_HEIGHT * BLOCK_SIZE

#define PIT_COLOR CYAN
#define BG_COLOR BLACK

// used to clear the position from the screen
typedef struct Backup {
  byte x, y, rot;
};

#define DROP_WAIT_INIT  1100

#define INPUT_WAIT_ROT  200
#define INPUT_WAIT_MOVE 100

#define INPUT_WAIT_NEW_SHAPE 400

class Tetris
{
	// shapes definitions
	
    byte l_shape[4][4][2] {
      {{0, 0}, {0, 1}, {0, 2}, {1, 2}},
      {{0, 1}, {1, 1}, {2, 0}, {2, 1}},
      {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
      {{0, 0}, {0, 1}, {1, 0}, {2, 0}},
    };

    byte j_shape[4][4][2] {
      {{1, 0}, {1, 1}, {0, 2}, {1, 2}},
      {{0, 0}, {1, 0}, {2, 0}, {2, 1}},
      {{0, 0}, {1, 0}, {0, 1}, {0, 2}},
      {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
    };

    byte o_shape[1][4][2] {
      { {0, 0}, {0, 1}, {1, 0}, {1, 1}}
    };

    byte s_shape[2][4][2] {
      {{0, 1}, {1, 0}, {1, 1}, {2, 0}},
      {{0, 0}, {0, 1}, {1, 1}, {1, 2}}
    };

    byte z_shape[2][4][2] {
      {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
      {{1, 0}, {0, 1}, {1, 1}, {0, 2}}
    };

    byte t_shape[4][4][2] {
      {{0, 0}, {1, 0}, {2, 0}, {1, 1}},
      {{0, 0}, {0, 1}, {1, 1}, {0, 2}},
      {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
      {{1, 0}, {0, 1}, {1, 1}, {1, 2}}
    };

    byte i_shape[2][4][2] {
      {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
      {{0, 0}, {0, 1}, {0, 2}, {0, 3}}
    };

	// All game shapes and their colors

    byte *all_shapes[7] = {l_shape[0][0], j_shape[0][0], o_shape[0][0], s_shape[0][0], z_shape[0][0], t_shape[0][0], i_shape[0][0]};

    unsigned int colors[7] = {ORANGE, BLUE, YELLOW, GREEN, RED, MAGENTA, CYAN};


    // how many rotated variations each shape has
	
    byte shapes[7] = {4, 4, 1, 2, 2, 4, 2};


	// game progress
    
	int lines, level;

	
	// current and next shape
	
    byte current, next;

    unsigned long lastInput, lastDrop;

    byte board[BOARD_HEIGHT][BOARD_WIDTH];

    byte x, y, rot;
    Backup old;

    boolean newShape;

    int dropWait;

  public:
    Tetris() : newShape(true), lines(0)
    {
    }

    void run()
    {
      Tft.fillScreen(BG_COLOR);

      // clean board
      for ( int i = 0; i < BOARD_WIDTH; i++ )
        for ( int j = 0; j < BOARD_HEIGHT; j++)
          board[j][i] = 0;

      //next shape
      next = random(7);

      // initialize game logic
      lastInput = 0;
      lastDrop = 0;
      dropWait = DROP_WAIT_INIT;
      level = 1;

      // draw background
      int c = LCD_HEIGHT / 28;
      for (int i = 0; i < LCD_HEIGHT; i += 2)
      {
        if ( i < BOARD_BOTTOM)
        {
          Tft.fillRectangle(0, i, BOARD_LEFT, 2, 0x1f - i / c);
          Tft.fillRectangle(BOARD_RIGHT, i, LCD_WIDTH, 2, 0x1f - i / c);
        }
        else
          Tft.drawLine(0, i, LCD_WIDTH, i, 0x3f - i / c);
      }

      // draw board left limit
      
	  Tft.drawLine (
        BOARD_LEFT - 1,
        BOARD_TOP,
        BOARD_LEFT - 1,
        BOARD_BOTTOM,
        PIT_COLOR);

      // draw board right limit
      
	  Tft.drawLine (
        BOARD_RIGHT,
        BOARD_TOP,
        BOARD_RIGHT,
        BOARD_BOTTOM,
        PIT_COLOR);

      // draw board bottom limit
	  
      Tft.drawLine (
        BOARD_LEFT - 1,
        BOARD_BOTTOM,
        BOARD_RIGHT + 1,
        BOARD_BOTTOM,
        PIT_COLOR);

      for ( int i = BOARD_LEFT + BLOCK_SIZE - 1; i < BOARD_RIGHT; i += BLOCK_SIZE)
      {
        Tft.drawLine (
          i,
          BOARD_TOP,
          i,
          BOARD_BOTTOM - 1,
          GRAY3);
      }

      for ( int i = BOARD_TOP + BLOCK_SIZE - 1; i < BOARD_BOTTOM; i += BLOCK_SIZE)
      {
        Tft.drawLine (
          BOARD_LEFT,
          i,
          BOARD_RIGHT - 1,
          i,
          GRAY2);
      }

      scoreBoard();

      do {

        // get clock
        const unsigned long now = millis();

        // display new shape
        if ( newShape )
        {
          Joystick::waitForRelease(INPUT_WAIT_NEW_SHAPE);
          newShape = false;

          // a new shape enters the game
          chooseNewShape();

          // draw next box
          Tft.fillRectangle(30, 100, BLOCK_SIZE * 6, BLOCK_SIZE * 5, BLACK);
          Tft.drawRectangle(29, 99, BLOCK_SIZE * 6 + 1, BLOCK_SIZE * 5 + 1, WHITE);

          byte *shape = all_shapes[next];
          for ( int i = 0; i < 4; i++ )
          {
            byte *block = shape + i * 2;
            Tft.fillRectangleUseBevel(
              30 + BLOCK_SIZE + block[0]*BLOCK_SIZE,
              100 + BLOCK_SIZE + block[1]*BLOCK_SIZE,
              BLOCK_SIZE - 2,
              BLOCK_SIZE - 2 ,
              colors[next]);
          }

          // check if new shape is placed over other shape(s)
          // on the board
          if ( touches(0, 0, 0 ))
          {
            // draw shape to screen
            draw();
            return;
          }

          // draw shape to screen
          draw();
        }
        else
        {
          // check if enough time has passed since last time the shape
          // was moved down the board
          if ( now - lastDrop > dropWait || Joystick::getY() > 0)
          {
            // update clock
            lastDrop = now;

            moveDown();
          }
        }

        if (!newShape && now - lastInput > INPUT_WAIT_MOVE)
        {
          userInput(now);
        }

      } while (true);
    }

  private:

    void chooseNewShape()
    {
      current = next;
      next = random(7);

      // new shape must be postioned at the middle of
      // the top of the board
      // with zero rotation
      rot = 0;
      y = 0;
      x = BOARD_WIDTH / 2;

      old.rot = rot;
      old.y = y;
      old.x = x;
    }

    void userInput(unsigned long now)
    {
      int jx = Joystick::getX();
      if (jx < 0 && x > 0 && !touches(-1, 0, 0))
      {
        x--;
      }
      else if (jx > 0 && x < BOARD_WIDTH && !touches(1, 0, 0))
      {
        x++;
      }
      else if ( Joystick::fire())
      {
        while ( !touches(0, 1, 0 ))
        {
          y++;
        }
      }
      else if (now - lastInput > INPUT_WAIT_ROT)
      {
        if (Joystick::getY() < 0 && !touches(0, 0, 1))
        {
          rot++;
          rot %= shapes[current];
        }
      }
      else
      {
        return;
      }
      lastInput = now;
      draw();
    }

    void moveDown()
    {
      // prepare to move down
      // check if board is clear bellow
      if ( touches(0, 1, 0 ))
      {
        // moving down touches another shape
        newShape = true;

        // this shape wont move again
        // add it to the board
        byte *shape = all_shapes[current];
        for ( int i = 0; i < 4; i++ )
        {
          byte *block = (shape + (rot * 4 + i) * 2);
          board[block[1] + y][block[0] + x] = current + 1;
        }

        // check if lines were made
        score();
        Beeping::beep(1500, 25);
      }
      else
      {
        // move shape down
        y += 1;
        draw();
      }
    }

    void draw()
    {
      byte *shape = all_shapes[current];
      for ( int i = 0; i < 4; i++ )
      {
        byte *block = (shape + (rot * 4 + i) * 2);
        Tft.fillRectangleUseBevel(
          BOARD_LEFT + block[0]*BLOCK_SIZE + BLOCK_SIZE * x,
          BOARD_TOP + block[1]*BLOCK_SIZE + BLOCK_SIZE * y,
          BLOCK_SIZE - 2,
          BLOCK_SIZE - 2 ,
          colors[current]);

        board[block[1] + y][block[0] + x] = 255;
      }

      // erase old
      for ( int i = 0; i < 4; i++ )
      {
        byte *block = (shape + (old.rot * 4 + i) * 2);

        if ( board[block[1] + old.y][block[0] + old.x] == 255 )
          continue;

        Tft.fillRectangle(
          BOARD_LEFT + block[0]*BLOCK_SIZE + BLOCK_SIZE * old.x,
          BOARD_TOP + block[1]*BLOCK_SIZE + BLOCK_SIZE * old.y,
          BLOCK_SIZE - 2,
          BLOCK_SIZE - 2 ,
          BG_COLOR);
      }

      for ( int i = 0; i < 4; i++ )
      {
        byte *block = (shape + (rot * 4 + i) * 2);
        board[block[1] + y][block[0] + x] = 0;
      }

      old.x = x;
      old.y = y;
      old.rot = rot;
    }

    boolean touches(int xi, int yi, int roti)
    {
      byte *shape = all_shapes[current];
      for ( int i = 0; i < 4; i++ )
      {
        byte *block = (shape + (((rot + roti) % shapes[current]) * 4 + i) * 2);

        int x2 = x + block[0] + xi;
        int y2 = y + block[1] + yi;

        if ( y2 == BOARD_HEIGHT || x2 == BOARD_WIDTH || board[y2][x2] != 0 )
        {
          return true;
        }
      }
      return false;
    }

    void score()
    {
      // we scan a max of 4 lines
      int ll;
      if ( y + 3 >= BOARD_HEIGHT )
      {
        ll = BOARD_HEIGHT - 1;
      }

      // scan board from current position
      for (int l = y; l <= ll; l++)
      {
        // check if there's a complete line on the board
        boolean line = true;
        for ( int c = 0; c < BOARD_WIDTH; c++ )
        {
          if (board[l][c] == 0)
          {
            line = false;
            break;
          }
        }

        if ( !line )
        {
          // move to next line
          continue;
        }

        Beeping::beep(3000, 50);

        lines++;

        if ( lines % 10 == 0 )
        {
          level++;
          dropWait /= 2;
        }

        scoreBoard();

        // move board down
        for ( int row = l; row > 0; row -- )
        {
          for ( int c = 0; c < BOARD_WIDTH; c++ )
          {
            byte v = board[row - 1][c];

            board[row][c] = v;
            Tft.fillRectangleUseBevel(
              BOARD_LEFT + BLOCK_SIZE * c,
              BOARD_TOP + BLOCK_SIZE * row,
              BLOCK_SIZE - 2,
              BLOCK_SIZE - 2 ,
              v == 0 ? BLACK : colors[v - 1] ) ;
          }
        }

        // clear top line
        for ( int c = 0; c < BOARD_WIDTH; c++ )
        {
          board[0][c] = 0;
        }

        Tft.fillRectangle(
          BOARD_LEFT,
          0,
          BOARD_RIGHT - BOARD_LEFT,
          BLOCK_SIZE,
          BLACK ) ;
      }

      delay(350);
    }

    void scoreBoard()
    {
      Tft.fillRectangle(6, 3, 128, 50, BLACK);
      Tft.drawString("Level", 8, 8, 2, YELLOW);
      Tft.drawString("Lines", 8, 32, 2, 0x3f);
      Tft.drawNumber(level, 74, 8, 2, YELLOW);
      Tft.drawNumber(lines, 74, 32, 2, 0x3f);
      Tft.drawRectangle(5, 2, 130, 52, 0xffff);
    }
};


#endif
