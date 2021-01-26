#include <math.h>
#include <string.h>
#include "jpeg.h"
#include "dct.h"

#define RCP_SQRT2 0.7071067811865475
#define PI_DIV16 0.19634954084936207

int zigzag[8][8] = {{0, 1, 5, 6, 14, 15, 27, 28},
		  {2, 4, 7, 13, 16, 26, 29, 42},
		  {3, 8, 12, 17, 25, 30, 41, 43},
		  {9, 11, 18, 24, 31, 40, 44, 53},
		  {10, 19, 23, 32, 39, 45, 52, 54},
		  {20, 22, 33, 38, 46, 51, 55, 60},
		  {21, 34, 37, 47, 50, 56, 59, 61},
		  {35, 36, 48, 49, 57, 58, 62, 63}};

void unzip(short *coef, short block[8][8])
{
  int x, y;
  for (x = 0; x < 8; x++)
    for (y = 0; y < 8; y++)
      block[x][y] = coef[zigzag[y][x]];
}

void dequant(short *coef, byte *quant_table)
{
  for (int i = 0; i < 64; i++) coef[i] *= quant_table[i];
}

void idct(short (*block)[8][8])
{
  short matrix2[8][8];
  double s = 0;
  double Cv = 1;
  double Cu = 1;
  for (int x = 0; x < 8; x++)
    for (int y = 0; y < 8; y++)
      {
	s = 0;
	for (int u = 0; u < 8; u++)
	  for (int v = 0; v < 8; v++)
	    {
	      Cv = 1;
	      Cu = 1;
	      if (u == 0) Cu =RCP_SQRT2 ;
	      if (v == 0) Cv = RCP_SQRT2;
	      short sh = (*block)[v][u];
	      double vv = (double)sh;
	      s += Cu * Cv * (vv * cos((2 * x + 1) * u * PI_DIV16) * cos((2 * y + 1) * v * PI_DIV16));
	    }
	matrix2[y][x] = (short)round(s / 4.0);
      }
  memcpy(block, &matrix2, 128);
}

/* byte clamp(int i) */
/* { */
/*   if (i > 255) return 255; */
/*   if (i < 0) return 0; */
/*   return (byte)i; */
/* } */

void level_shift(short (*block)[8][8])
{
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      (*block)[i][j] = clamp((*block)[i][j] + 128);
}
