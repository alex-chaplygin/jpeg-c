#include <math.h>
#include "jpeg.h"
#include "dct.h"

extern byte *data;
extern struct jpeg jpeg;

void convert_colors()
{
  double temp;
  byte y, cb, cr;
  for (int i = 0; i < jpeg.frame.width * jpeg.frame.height; i++) {
    y = data[i * 3];
    cb = data[i * 3 + 1];
    cr = data[i * 3 + 2];
    // 255 188 147 -> 255 220 255
    int r = (int)round(y + 1.402 * (cr - 128));
    int g = (int)round(y - 0.3441 * (cb - 128) - 0.7141 * (cr - 128));
    int b = (int)round(y + 1.772 * (cb - 128));
    data[i * 3 + 0] = clamp(r);
    data[i * 3 + 1] = clamp(g);
    data[i * 3 + 2] = clamp(b);
  }
}
