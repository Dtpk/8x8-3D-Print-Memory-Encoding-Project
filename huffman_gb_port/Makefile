# GBDK path
GBDK_HOME = /home/daniel_sensenig/gbdk_toolchain/

CC = $(GBDK_HOME)bin/lcc

# GBDK_PATH = /opt/gbdk/bin/

CFLAGS = -Wa-l -Wl-m -Wl-j

all: huffman_gb.gb

huffman_gb.gb: main.c huffman_gb_data.h
	$(CC) $(CFLAGS) -o huffman_gb.gb main.c

clean:
	rm -f *.o *.lst *.map *.gb *.ihx *.sym
