#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"

extern byte *data;
extern struct jpeg jpeg;
extern int data_pos_x;
extern int data_pos_y;

extern int h_max;
extern int v_max;

short block2[8 * 8 * 4 * 4];

byte lerp(int x, float x0, byte y0, float x1, byte y1)
{
  float res = y0 + (float)(y1 - y0) / (x1 - x0) * (x - x0);
  if (res > 255) return 255;
  else if (res < 0) return 0;
  else return (byte)res;
}

void interpolate_block(short block[8][8], int chan, int coef_h, int coef_v, int data_x, int data_y)
{
#ifdef DEBUG
  fprintf(stderr, "interpolate %d %d %d %d\n", coef_h, coef_v, data_x, data_y);
#endif
  for (int x = 0; x < 8 * coef_h; x++) {
    int x0 = x / coef_h;
    if (x0 == 7) x0--;
    int x1 = x0 + 1;
#ifdef DEBUG
    fprintf(stderr, "x = %d x0 = %d x1 = %d\n", x, x0, x1);
#endif
    for (int y = 0; y < 8 * coef_v; y++) {
      block2[y * 8 * coef_h + x] = lerp(x, x0 * coef_h + 1.0f / coef_h, block[x0][y / coef_v], x1 * coef_h + 1.0f / coef_h, block[x1][y / coef_v]);
    }
  }
  for (int y = 0; y < 8 * coef_v; y++) {
    int x0 = y / coef_v;
    if (x0 == 7) x0--;
    int x1 = x0 + 1;
#ifdef DEBUG
    fprintf(stderr, "y = %d x0 = %d x1 = %d\n", y, x0, x1);
#endif
    for (int x = 0; x < 8 * coef_h; x++) {
      block2[y * 8 * coef_h + x] = lerp(y, x0 * coef_v + 1.0f / coef_v, block2[x0 * coef_v * 8 * coef_h + x], x1 * coef_v + 1.0f / coef_v, block2[x1 * coef_v * 8 * coef_h + x]);
    }
  }
  for (int y = 0; y < 8 * coef_v; y++)
    for (int x = 0; x < 8 * coef_h; x++)
      data[((data_y + y) * jpeg.frame.width + data_x + x) * jpeg.frame.num_chan + chan] = block2[y * 8 * coef_h + x];
}

void put_block(short block[8][8], int chan, int h, int v)
{
  int data_x, data_y;
  if (jpeg.channels[chan].H < h_max || jpeg.channels[chan].V < v_max)
  	interpolate_block(block, chan, h_max / jpeg.channels[chan].H, v_max / jpeg.channels[chan].V, (data_pos_x * h_max + h) * 8, (data_pos_y * v_max + v) * 8);
  else
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++) {
      data_x = (data_pos_x * h_max + h) * 8 + x;
      data_y = (data_pos_y * v_max + v) * 8 + y;
      data[(data_y * jpeg.frame.width + data_x) * jpeg.frame.num_chan + chan] = block[x][y];
    }
}

int correct_jpeg_size(int size, int factor)
{
  if (size % 8 == 0 && (size / factor) % 8 == 0) return size;
  while ((size / factor) % 8 != 0) size++;
  return size;
}
