#include <supervision.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "font.h"

#define BYTES_PER_LINE 48

#define BLACK 0xFF
#define WHITE 0x00
#define L_GRAY 0x55
#define D_GRAY 0xAA

#define HEIGHT 0xA0
#define WIDTH 0xA0

#define BOARD_SIZE 4
#define FIELD_SIZE_BYTE 9   // bytes / 36 pixels
#define FIELD_BYTE_OFFSET 2 //  bytes / 8 pixel
#define BLOCK_PIX_SIZE 34   //  pixels
#define FIELD_AREA_PIX 36   //  pixels
#define BORDER 2            //  pixels
#define TOP_MARGIN 8        // pixels

unsigned long score;
unsigned int rundFrameCount = 0;

unsigned int get_random(void)
{
  short feedback;

  feedback = rundFrameCount & 1;
  rundFrameCount >>= 1;
  if (feedback == 1)
    rundFrameCount ^= 0x5432;
  return rundFrameCount;
}

unsigned char gameBoard[BOARD_SIZE][BOARD_SIZE];

static void fillScreen(unsigned char val)
{
  memset(SV_VIDEO, val, 0x2000);
}

/* Necessary conversion to have 2 bits per pixel with darkest hue */
/* Remark: The Supervision uses 2 bits per pixel, and bits are mapped into pixels in regular order */
static unsigned int __fastcall__ double_bits(unsigned char)
{
  __asm__("stz ptr2");
  __asm__("stz ptr2+1");
  __asm__("ldy #$08");
L1:
  __asm__("lsr a");
  __asm__("php");
  __asm__("ror ptr2");
  __asm__("ror ptr2+1");
  __asm__("plp");
  __asm__("ror ptr2");
  __asm__("ror ptr2+1");
  __asm__("dey");
  __asm__("bne %g", L1);
  __asm__("lda ptr2+1");
  __asm__("ldx ptr2");
  return __AX__;
}

static void display_char(const unsigned char x, const unsigned char y, const unsigned char *ch, const char color)
{
  unsigned char i, offset;
  unsigned int addr, word;

  addr = y * BYTES_PER_LINE + (x >> 2);
  offset = (x & 3) << 1;

  for (i = 0; i < 8; ++i)
  {
    word = double_bits(ch[i]);
    *(unsigned int *)(0x4000 | addr + i * BYTES_PER_LINE) &= ~((word & ~color) << offset);
    *(unsigned int *)(0x4000 | addr + i * BYTES_PER_LINE) |= (word & color) << offset;
    word >>= 8;
    *(unsigned int *)(0x4000 | addr + i * BYTES_PER_LINE + 1) &= ~((word & ~color) << offset);
    *(unsigned int *)(0x4000 | addr + i * BYTES_PER_LINE + 1) |= (word & color) << offset;
  }
}

static void init(void)
{
  SV_LCD.width = 160;
  SV_LCD.height = 160;
}

static void print(char *str, int len, unsigned char x, unsigned char y, char invert)
{
  unsigned char i;

  for (i = 0; i < len; i++)
  {
    char character = *(str + i * sizeof(char));
    if(character == '\0') break;
    if (character < 0x30 || character > 0x3A)
    {
      display_char(x + i * 8, y, font + (character - 0x20) * sizeof(unsigned char) * 8, invert);
    }
    else
    {
      display_char(x + i * 8, y, SV_VIDEO + 0x1FB0 + (character - 0x30) * sizeof(unsigned char) * 8, invert);
    }
  }
}

void title()
{
  init();
  fillScreen(L_GRAY);
  print("2048", 4, 64, 72, BLACK);
  print("created by vrodin", 17, 16, 152, BLACK);
}

short getLeftNumMargin(const char val)
{
  if (val > 9)
    return 1;
  if (val > 6)
    return 5;
  if (val > 3)
    return 9;
  return 13;
}

void clearScore()
{
  memset(SV_VIDEO + 0x1C8D, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1CBD, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1CED, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1D1D, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1D4D, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1D7D, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1DAD, D_GRAY, 0x1B);
  memset(SV_VIDEO + 0x1DDD, D_GRAY, 0x1B);
}

void drawBlank(unsigned short i, unsigned short j)
{
  unsigned char k;
  memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN) * BYTES_PER_LINE, SV_VIDEO + 0x1E00, FIELD_SIZE_BYTE);
  memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN + 33) * BYTES_PER_LINE, SV_VIDEO + 0x1E00, FIELD_SIZE_BYTE);

  for (k = 1; k < 33; k++)
  {
    memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN + k) * BYTES_PER_LINE, SV_VIDEO + 0x1E09, FIELD_SIZE_BYTE);
  }
}

void drawField(unsigned short i, unsigned short j, unsigned short val)
{
  unsigned int startAddr;
  unsigned short k;

  memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN) * BYTES_PER_LINE, SV_VIDEO + 0x1E12, FIELD_SIZE_BYTE);
  memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN + 33) * BYTES_PER_LINE, SV_VIDEO + 0x1E12, FIELD_SIZE_BYTE);
  if (val > 8)
  {
    startAddr = 0x1E39;
  }
  else if (val > 5)
  {
    startAddr = 0x1E25;
  }
  else if (val > 2)
  {
    startAddr = 0x1E2F;
  }
  else
  {
    startAddr = 0x1E1B;
  }

  for (k = 1; k < 33; k++)
  {
    memcpy(SV_VIDEO + FIELD_BYTE_OFFSET + FIELD_SIZE_BYTE * j + (i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN + k) * BYTES_PER_LINE, SV_VIDEO + startAddr, FIELD_SIZE_BYTE);
  }
}

void drawBoard(void)
{
  unsigned char leftMargin = 11, numMargin = 13, color;
  unsigned char i, j;
  unsigned char str[14];

  for (i = 0; i < BOARD_SIZE; i++)
  {
    for (j = 0; j < BOARD_SIZE; j++)
    {

      if (gameBoard[BOARD_SIZE - 1 - i][j])
      {
        drawField(i, j, gameBoard[BOARD_SIZE - 1 - i][j]);

        color = gameBoard[BOARD_SIZE - 1 - i][j] > 5 ? WHITE : BLACK;
        memset(str,' ',4);
        utoa(1 << gameBoard[BOARD_SIZE - 1 - i][j], str, 10);
        print(str, 4, leftMargin + (BLOCK_PIX_SIZE + BORDER) * j + getLeftNumMargin(gameBoard[BOARD_SIZE - 1 - i][j]), i * (BLOCK_PIX_SIZE + BORDER) + TOP_MARGIN + numMargin, color);
      }
      else
      {
        drawBlank(i, j);
      }
    }
  }

  clearScore();
  
  utoa(score, str, 10);
  print(str, 13, 56, 152, WHITE);
}

void predrawBoard()
{
  // BLANK
  *(unsigned long *)(SV_VIDEO + 0x1E00) = 0x5555556A;
  *(unsigned long *)(SV_VIDEO + 0x1E04) = 0x55555555;
  *(unsigned char *)(SV_VIDEO + 0x1E08) = 0x95;

  *(unsigned long *)(SV_VIDEO + 0x1E09) = 0x5555555A;
  *(unsigned long *)(SV_VIDEO + 0x1E0D) = 0x55555555;
  *(unsigned int *)(SV_VIDEO + 0x1E11) = 0xAA55;

  // FIRST LINE
  *(unsigned short *)(SV_VIDEO + 0x1E12) = 0xEA;
  *(unsigned short *)(SV_VIDEO + 0x1E12) = 0xFFEA;
  *(unsigned short *)(SV_VIDEO + 0x1E14) = 0xFFFF;
  *(unsigned long *)(SV_VIDEO + 0x1E16) = 0xFFFFFFFF;
  *(unsigned char *)(SV_VIDEO + 0x1E1A) = 0xBF;

  // WHITE
  *(unsigned short *)(SV_VIDEO + 0x1E1B) = 0x3A;
  *(unsigned short *)(SV_VIDEO + 0x1E1D) = 0x0;
  *(unsigned long *)(SV_VIDEO + 0x1E1F) = 0x0;
  *(unsigned short *)(SV_VIDEO + 0x1E23) = 0xAAD0;

  // D_GRAY
  *(unsigned short *)(SV_VIDEO + 0x1E25) = 0xAA7A;
  *(unsigned short *)(SV_VIDEO + 0x1E27) = 0xAAAA;
  *(unsigned long *)(SV_VIDEO + 0x1E29) = 0xAAAAAAAA;
  *(unsigned short *)(SV_VIDEO + 0x1E2D) = 0xAAFA;

  // L_GRAY
  *(unsigned short *)(SV_VIDEO + 0x1E2F) = 0x553A;
  *(unsigned short *)(SV_VIDEO + 0x1E31) = 0x5555;
  *(unsigned long *)(SV_VIDEO + 0x1E33) = 0x55555555;
  *(unsigned short *)(SV_VIDEO + 0x1E37) = 0xAAE5;

  // BLACK
  *(unsigned short *)(SV_VIDEO + 0x1E39) = 0xFFBA;
  *(unsigned short *)(SV_VIDEO + 0x1E3B) = 0xFFFF;
  *(unsigned long *)(SV_VIDEO + 0x1E3D) = 0xFFFFFFFF;
  *(unsigned short *)(SV_VIDEO + 0x1E41) = 0xAAFF;

  // NUMBERS last 80 byte of video buffer
  memcpy(SV_VIDEO + 0x1FB0, font + 128, 80);
}

void putRand()
{
  unsigned char cell = get_random() % 16;
  if (!gameBoard[cell / 4][cell % 4])
  {
    gameBoard[cell / 4][cell % 4] = get_random() % 32 == 0 ? 2 : 1;
  }
  else
  {
    putRand();
  }
}

short up()
{
  short i, j, k, valid_step = 0;
  for (j = 0; j < BOARD_SIZE; ++j)
  {
    for (i = BOARD_SIZE - 1; i >= 0; --i)
    {
      if (gameBoard[i][j] == 0)
      {
        for (k = i - 1; k >= 0; --k)
        {
          if (gameBoard[k][j] != 0)
          {
            gameBoard[i][j] = gameBoard[k][j];
            gameBoard[k][j] = 0;
            valid_step = 1;
            i++;
            break;
          }
        }
      }
      else
      {
        for (k = i - 1; k >= 0; --k)
        {
          if (gameBoard[k][j] != 0)
          {
            if (gameBoard[k][j] != gameBoard[i][j])
              break;
            gameBoard[i][j] += 1;
            gameBoard[k][j] = 0;
            valid_step = 1;
            score += 1 << gameBoard[i][j];
            break;
          }
        }
      }
    }
  }

  return valid_step;
}

short down()
{
  short i, j, k, valid_step = 0;
  for (j = 0; j < BOARD_SIZE; ++j)
  {
    for (i = 0; i < BOARD_SIZE; ++i)
    {
      if (gameBoard[i][j] == 0)
      {
        for (k = i + 1; k < BOARD_SIZE; ++k)
        {
          if (gameBoard[k][j] != 0)
          {
            gameBoard[i][j] = gameBoard[k][j];
            gameBoard[k][j] = 0;
            valid_step = 1;
            i--;
            break;
          }
        }
      }
      else
      {
        for (k = i + 1; k < BOARD_SIZE; ++k)
        {
          if (gameBoard[k][j] != 0)
          {
            if (gameBoard[k][j] != gameBoard[i][j])
              break;
            gameBoard[i][j] += 1;
            gameBoard[k][j] = 0;
            valid_step = 1;
            score += 1 << gameBoard[i][j];
            break;
          }
        }
      }
    }
  }

  return valid_step;
}

short left()
{
  short i, j, k, valid_step = 0;
  for (i = 0; i < BOARD_SIZE; ++i)
  {
    for (j = 0; j < BOARD_SIZE; ++j)
    {
      if (gameBoard[i][j] == 0)
      {
        for (k = j + 1; k < BOARD_SIZE; ++k)
        {
          if (gameBoard[i][k] != 0)
          {
            valid_step = 1;
            gameBoard[i][j] = gameBoard[i][k];
            gameBoard[i][k] = 0;
            j--;
            break;
          }
        }
      }
      else
      {
        for (k = j + 1; k < BOARD_SIZE; ++k)
        {
          if (gameBoard[i][k] != 0)
          {
            if (gameBoard[i][j] != gameBoard[i][k])
              break;
            valid_step = 1;
            gameBoard[i][j] += 1;
            gameBoard[i][k] = 0;
            score += 1 << gameBoard[i][j];
          }
        }
      }
    }
  }

  return valid_step;
}

short right()
{
  short i, j, k, valid_step = 0;
  for (i = 0; i < BOARD_SIZE; ++i)
  {
    for (j = BOARD_SIZE - 1; j >= 0; --j)
    {
      if (gameBoard[i][j] == 0)
      {
        for (k = j - 1; k >= 0; --k)
        {
          if (gameBoard[i][k] != 0)
          {
            valid_step = 1;
            gameBoard[i][j] = gameBoard[i][k];
            gameBoard[i][k] = 0;
            j++;
            break;
          }
        }
      }
      else
      {
        for (k = j - 1; k >= 0; --k)
        {
          if (gameBoard[i][k] != 0)
          {
            if (gameBoard[i][j] != gameBoard[i][k])
              break;
            valid_step = 1;
            gameBoard[i][j] += 1;
            gameBoard[i][k] = 0;
            score += 1 << gameBoard[i][j];
          }
        }
      }
    }
  }

  return valid_step;
}

void game()
{
  fillScreen(D_GRAY);
  memset(gameBoard, 0, sizeof gameBoard);
  score = 0;
  predrawBoard();
  putRand();
  putRand();
  drawBoard();
  print("SCORE", 5, 11, 152, WHITE);

  while (1)
  {
    if (~SV_CONTROL & JOY_UP_MASK && up())
    {
      putRand();
      drawBoard();
    }
    if (~SV_CONTROL & JOY_DOWN_MASK && down())
    {
      putRand();
      drawBoard();
    }
    if (~SV_CONTROL & JOY_LEFT_MASK && left())
    {
      putRand();
      drawBoard();
    }
    if (~SV_CONTROL & JOY_RIGHT_MASK && right())
    {
      putRand();
      drawBoard();
    }
  }
}

void main(void)
{
  unsigned int wait = 2600;
  title();

  while (1)
  {
    --wait;
    ++rundFrameCount;

    if (~SV_CONTROL & JOY_START_MASK)
    {
      game();

      break;
    }

    if (wait == 2000)
    {
      memset(SV_VIDEO + 0x1980, L_GRAY, 0x180);
    }
    if (!wait)
    {
      wait = 4800;
      print("press start", 11, 40, 136, WHITE);
    }
  }
}
