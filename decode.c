#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeg.h"
#include "dct.h"
#include "markers.h"
#include "block.h"

extern FILE *jpeg_file;
extern struct jpeg jpeg;

byte *data;
struct huff_table *cur_huff_table;
byte count;
byte cur_byte;
int num_blocks_mcu;
short pred[4];
int h_max;
int v_max;
int data_pos_x = 0;
int data_pos_y = 0;

byte next_bit()
{
  byte b2;
  if (count == 0)
    {
      fread(&cur_byte, 1, 1, jpeg_file);
      count = 8;
      if (cur_byte == 0xFF)
	{
	  fread(&b2, 1, 1, jpeg_file);
	  if (b2 != 0)
	    {
	      if (b2 == 0xDC) {
		fprintf(stderr, "DNL marker\n");
		exit(1);
	      } else {
		fprintf(stderr, "Decode error pos = %x\n", ftell(jpeg_file));
		exit(1);
	      }
	      
	    }
	}
    }
  byte bit = cur_byte >> 7;
#ifdef DEBUG
  //  fprintf(stderr, "%x %x\n", cur_byte, bit);
#endif
  count--;
  cur_byte = cur_byte << 1;
  return bit;
}

byte decode()
{
  int i = 0;
  word code = next_bit();
  while (i < 16 && code > cur_huff_table->max_code[i])
    {
      i++;
      code = (word)((code << 1) + next_bit());
    }
  if (i >= 16) return 0;
  int j = cur_huff_table->valptr[i];
  j = j + code - cur_huff_table->min_code[i];
  return cur_huff_table->values[j];
}

word receive(byte ssss)
{
  byte i = 0;
  word v = 0;
  do
    {
      i++;
      v = (word)((v << 1) + next_bit());
    }
  while (i != ssss);
  return v;
}

short extend(ushort diff, int num_bits)
{
            short diffT = (short)(1 << (num_bits - 1));
            short result = (short)diff;
            if (diff < diffT)
            {
                diffT = (short)((-1 << num_bits) + 1);
                result = (short)(diff + diffT);
            }
            return result;
}/*
short extend(word diff, int num_bits)
{
  word vt = (word)(1 << (num_bits - 1));
  while (diff < vt)
    {
      vt = (word)((-1 << num_bits) + 1);
      diff += (word)vt;
    }

  return (short)diff;
  }*/

short decode_dc()
{
  byte t = decode();
  if (!t) return 0;
  word w = receive(t);
  return extend(w, t);
}

void decode_block(short *block, int num_dc, int num_ac)
{
  byte k, rs, rrrr, r, ssss;
  int flag = 1;
  k = 1;
  memset(block, 0, 64 * sizeof(short));
  cur_huff_table = &jpeg.huff_tables[num_dc];
  block[0] = decode_dc();
  cur_huff_table = &jpeg.huff_tables[num_ac + 2];
  while (flag)
    {
      rs = decode();
      ssss = (byte)(rs & 0xf);
      rrrr = (byte)(rs >> 4);
      r = rrrr;
      if (ssss == 0)
	if (r == 15) k += 16;
	else flag = 0;
      else
	{
	  k += r;
	  block[k] = (word)receive(ssss);
	  block[k] = extend((word)block[k], ssss);
	  if (k == 63) flag = 0;
	  else k++;
	}
    }
}

int decode_mcu()
{
  short coef[64];
  short block[8][8];

  for (int chan = 0; chan < jpeg.frame.num_chan; chan++)
    for (int v = 0; v < jpeg.channels[chan].V; v++)
      for (int h = 0; h < jpeg.channels[chan].H; h++) {
	decode_block(coef, jpeg.scan.comps[chan].dc, jpeg.scan.comps[chan].ac);
#ifdef DEBUG
	printf("Block\n");
	for (int i = 0; i < 64; i++) printf("%d ", coef[i]);
	printf("\n\n");
#endif
	coef[0] += pred[chan];
	pred[chan] = coef[0];
	dequant(coef, jpeg.quant_tables[jpeg.channels[chan].quant_num].table);
	unzip(coef, block);
	idct(&block);
	level_shift(&block);
#ifdef DEBUG
	for (int y = 0; y < 8; y++) {
	  for (int x = 0; x < 8; x++)
	    fprintf(stderr, "%d ", block[x][y]);
	  fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
#endif
	put_block(block, chan, h, v);
      }
      data_pos_x++;
      if (data_pos_x >= jpeg.frame.width / 8 / h_max) {
	data_pos_x = 0;
	data_pos_y++;
      }
      if (data_pos_y >= jpeg.frame.height / 8 / v_max) return 1;
      else return 0;
}

int decode_restart_interval()
{
    for (int r = 0; r < jpeg.restart_interval; r++) {
#ifdef DEBUG
      fprintf(stderr, "data_x = %d data_y = %d %d \n", data_pos_x, data_pos_y, jpeg.frame.width / 8 / h_max);
#endif
      if (decode_mcu()) return 1;
    }
#ifdef DEBUG
    fprintf(stderr, "Restart %x\n", ftell(jpeg_file));
#endif
    count = 0;
    pred[0] = pred[1] = pred[2] = 0;
    word w = read16();
    if (w == EOI) return 1;
    else if (w < 0xffd0 || w > 0xffd7) {
      fprintf(stderr, "Restart error\n");
      exit(1);
    }
    return 0;
}

void decode_frame()
{
  count = 0;
  num_blocks_mcu = 0;
  pred[0] = pred[1] = pred[2] = 0;
  h_max = 0;
  v_max = 0;
  for (int i = 0; i < jpeg.frame.num_chan; i++) {
    num_blocks_mcu += jpeg.channels[i].H * jpeg.channels[i].V;
    if (jpeg.channels[i].H > h_max) h_max = jpeg.channels[i].H;
    if (jpeg.channels[i].V > v_max) v_max = jpeg.channels[i].V;
#ifdef DEBUG
    fprintf(stderr, "Num blocks mcu = %d\n", num_blocks_mcu);
    fprintf(stderr, "Channel = %d H = %d V = %d \n", i, jpeg.channels[i].H, jpeg.channels[i].V);
#endif
  }
  jpeg.frame.width =  correct_jpeg_size(jpeg.frame.width, h_max);
  jpeg.frame.height = correct_jpeg_size(jpeg.frame.height, v_max);
#ifdef DEBUG
  fprintf(stderr, "Total bytes %d\n", jpeg.frame.width * jpeg.frame.height * jpeg.frame.num_chan);
  fprintf(stderr, "h_max = %d v_max = %d\n", h_max, v_max);
#endif
  data = malloc(jpeg.frame.width * jpeg.frame.height * jpeg.frame.num_chan);
  while(1) {
    if (jpeg.restart_interval == 0) {
      decode_mcu();
      if (data_pos_y >= jpeg.frame.height / 8 / v_max) break;
    } else if (decode_restart_interval()) break;
  }
}
