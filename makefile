OBJS=main.o jpeg.o huff.o decode.o dct.o color.o block.o
CFLAGS= -g -O3

all: $(OBJS)
	gcc -o /tmp/jpeg $(OBJS) -lm
