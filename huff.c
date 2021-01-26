#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"
#include "huff.h"

void generate_size(struct huff_table *h, int num_codes)
{
  int k = 0;
  byte i = 1;
  int j = 1;
  int last;
  h->sizes = malloc(num_codes + 1);
  do
    {
      while (!(j > h->bits[i - 1]))
	{
	  h->sizes[k] = i;
	  k++;
	  j++;
	}

      i++;
      j = 1;
    } while (!(i > 16));

  h->sizes[k] = 0;
  last = k;
}

void generate_codes(struct huff_table *h, int num_codes)
{
  int k = 0;
  word code = 0;
  byte si = h->sizes[0];
  h->codes = malloc(num_codes * sizeof(word));
  do
    {
      do
	{
	  h->codes[k] = code;
	  code++;
	  k++;
	} while (h->sizes[k] == si);

      if (h->sizes[k] == 0)
	{
	  break;
	}

      do
	{
	  code = (ushort)(code << 1);
	  si++;
	} while (h->sizes[k] != si);
    } while (h->sizes[k] == si);
}

void generate_minmax(struct huff_table *h, int num_codes)
{
  int i = -1;//0
  byte j = 0;
  int done = 0;
  do
    {
      i++;
      if (i >= 16) { done = 1; break; }//>
      while ((h->bits[i] == 0) && (!done))
	{
	  h->max_code[i] = -1;
	  i++;
	  if (i >= 16)//>
	    {
	      done = 1;
	      break;
	    }
	}
      if (done) break;
      h->valptr[i] = j;
      h->min_code[i] = h->codes[j];
      j = (byte)(j + h->bits[i] - 1);
      h->max_code[i] = h->codes[j];
      j++;
    }
  while ((h->bits[i] != 0));
}

void generate_huffman(struct huff_table *h, int num_codes)
{
  generate_size(h, num_codes);
  generate_codes(h, num_codes);
  generate_minmax(h, num_codes);
}
