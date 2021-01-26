#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jpeg.h"
#include "markers.h"
#include "huff.h"
#include "decode.h"
#include "color.h"

int width = 100;
int height = 100;
int num_channels = 3;

struct jpeg jpeg;
FILE *jpeg_file;
extern byte *data;

void swap(word *data)
{
  word w = *data;
  *data = (w >> 8) + ((w & 0xff) << 8);
}

word read16()
{
  word w;
  fread(&w, sizeof(w), 1, jpeg_file);
  swap(&w);
  return w;
}

void read_frame()
{
    fread(&jpeg.frame, sizeof(jpeg.frame), 1, jpeg_file);
    swap(&jpeg.frame.width);
    swap(&jpeg.frame.height);
    width = jpeg.frame.width;
    height = jpeg.frame.height;
    num_channels = jpeg.frame.num_chan;
    jpeg.channels = malloc(sizeof(struct channel) * num_channels);
    fread(jpeg.channels, sizeof(struct channel) * num_channels, 1, jpeg_file);
#ifdef DEBUG
    fprintf(stderr, "width = %d, height = %d, num_chan = %d bits = %d\n", width, height, num_channels, jpeg.frame.numbits);
    for (int i = 0; i < num_channels; i++)
      fprintf(stderr, "channel %d H = %d V = %d quant = %d\n", jpeg.channels[i].id, jpeg.channels[i].H, jpeg.channels[i].V, jpeg.channels[i].quant_num);
#endif
}

void read_quant_table()
{
  struct quant_table q;
  fread(&q, sizeof(q), 1, jpeg_file);
  jpeg.quant_tables[q.id] = q;
#ifdef DEBUG
  fprintf(stderr, "Quant table num = %d %d | ", q.id, q.p);
  for (int i = 0; i < sizeof(q.table); i++) fprintf(stderr, "%d ", q.table[i]);
  fprintf(stderr, "\n");
#endif
}

void read_huff_table()
{
  struct huff_table h;
  int num_codes = 0;
  fread(&h, 17, 1, jpeg_file);
  for (int i = 0; i < sizeof(h.bits); i++) num_codes += h.bits[i];
  h.values = malloc(num_codes);
  fread(h.values, num_codes, 1, jpeg_file);
  generate_huffman(&h, num_codes);
  jpeg.huff_tables[h.id + h.class * 2] = h;
#ifdef DEBUG
  fprintf(stderr, "HUFF table id = %d class = %d\nbits ", h.id, h.class);
  for (int i = 0; i < sizeof(h.bits); i++) fprintf(stderr, "%d ", h.bits[i]);
  fprintf(stderr, "\nvalues ");
  for (int i = 0; i < num_codes; i++) fprintf(stderr, "%x ", h.values[i]);
  fprintf(stderr, "\nsizes ");
  for (int i = 0; i < num_codes; i++) fprintf(stderr, "%d ", h.sizes[i]);
  fprintf(stderr, "\ncodes ");
  for (int i = 0; i < num_codes; i++) fprintf(stderr, "%x ", h.codes[i]);
  fprintf(stderr, "\nmincode ");  
  for (int i = 0; i < 16; i++) fprintf(stderr, "%x ", h.min_code[i]);
  fprintf(stderr, "\nmaxcode ");  
  for (int i = 0; i < 16; i++) fprintf(stderr, "%x ", h.max_code[i]);
  fprintf(stderr, "\nvalptr ");  
  for (int i = 0; i < 16; i++) fprintf(stderr, "%x ", h.valptr[i]);
  fprintf(stderr, "\n");  
#endif  
}

void read_restart()
{
  jpeg.restart_interval = read16();
#ifdef DEBUG
  fprintf(stderr, "restart = %d\n", jpeg.restart_interval);
#endif
}

void read_scan()
{
  fread(&jpeg.scan, sizeof(jpeg.scan), 1, jpeg_file);
  if (jpeg.scan.num_comp != 3) {
    fprintf(stderr, "Invalid scan num_comp = %d\n", jpeg.scan.num_comp);
    exit(1);
  }
#ifdef DEBUG
  for (int i = 0; i < jpeg.scan.num_comp; i++)
    fprintf(stderr, "scan num = %d dc = %d ac = %d\n", jpeg.scan.comps[i].id, jpeg.scan.comps[i].dc, jpeg.scan.comps[i].ac);
#endif
}

void read_jpeg_data(word marker)
{
  int pos = ftell(jpeg_file);
  word len = read16();
#ifdef DEBUG
  fprintf(stderr, "len = %d\n", len);
#endif
  switch (marker) {
  case FRM: read_frame(); break;
  case FRM_PROGR: fprintf(stderr, "Progressive mode is not supported\n"); exit(1); break;
  case QNT: read_quant_table(); break;
  case HUF: read_huff_table(); break;
  case RST: read_restart(); break;
  case SOS: read_scan(); break;
  }
  fseek(jpeg_file, pos + len, SEEK_SET);
}

void jpeg_decode(char *filename)
{
  jpeg.restart_interval = 0;
  jpeg_file = fopen(filename, "rb");
  if (!jpeg_file) {
    fprintf(stderr, "No file %s\n", filename);
    exit(1);
  }
  word marker = read16();
  if (marker != SOI) {
    fprintf(stderr, "No SOI %04x\n", marker);
    exit(1);
  }
  while (marker != SOS) {
    marker = read16();
#ifdef DEBUG
    fprintf(stderr, "marker = %04x\n", marker);
#endif
    read_jpeg_data(marker);
  }
  decode_frame();
  fclose(jpeg_file);
}

int jpeg_get_width()
{
  return width;
}

int jpeg_get_height()
{
  return height;
}

int jpeg_get_num_channels()
{
  return num_channels;
}

byte *jpeg_get_data()
{
  byte *data2 = malloc(width * height * num_channels);
  byte *dst = data2;
  int pos;
  convert_colors();
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      for (int c = 0; c < num_channels; c++) {
	pos = (y * jpeg.frame.width + x) * num_channels;
	*dst++ = data[pos + c];
      }
  return data2;
}
