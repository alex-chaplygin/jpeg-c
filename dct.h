void unzip(short *coef, short block[8][8]);
void dequant(short *coef, byte *quant_table);
void idct(short (*block)[8][8]);
void level_shift(short (*block)[8][8]);
//byte clamp(int i);

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#define clamp(i) max(0, min(255, (i)))
