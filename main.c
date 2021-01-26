#include <stdio.h>
#include "jpeg.h"

void write_ppm()
{
  int width = jpeg_get_width();
  int height = jpeg_get_height();
  int num_chan = jpeg_get_num_channels();
  byte *data = jpeg_get_data();
  printf("P3\n");
  printf("%i %i\n", width, height);
  printf("255\n");
  for (int i = 0; i < width * height * num_chan; i++) printf("%i ", data[i]);
  printf("\n");
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    printf("jpeg <file>\n");
  else {
    jpeg_decode(argv[1]);
    write_ppm();
  }
  return 0;
}
