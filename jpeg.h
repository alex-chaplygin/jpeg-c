typedef unsigned char byte;
typedef unsigned short word;

#pragma pack(1)

struct frame {
  byte numbits;
  word height;
  word width;
  byte num_chan;
};

struct channel {
  byte id;
  byte V:4;
  byte H:4;
  byte quant_num;
};

struct quant_table {
  byte id:4;
  byte p:4;
  byte table[64];
};

struct comp {
  byte id;
  byte ac:4;
  byte dc:4;
};

struct scan {
  byte num_comp;
  struct comp comps[3];
  byte start;
  byte end;
  byte apr;
};

struct huff_table {
  byte id:4;
  byte class:4;
  byte bits[16];
  byte *values;
  byte *sizes;
  word *codes;
  int min_code[16];
  int max_code[16];
  byte valptr[16];
};

struct jpeg {
  struct frame frame;
  struct channel *channels;
  struct quant_table quant_tables[4];
  struct huff_table huff_tables[4];
  word restart_interval;
  struct scan scan;
};


void jpeg_decode(char *filename);
int jpeg_get_width();
int jpeg_get_height();
int jpeg_get_num_channels();
byte *jpeg_get_data();
