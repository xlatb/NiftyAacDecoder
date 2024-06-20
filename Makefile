BINS=read

OBJS=AacConstants.o AacBitReader.o \
	AacDecoder.o AacScalefactorDecoder.o AacSpectrumDecoder.o \
	AacAdtsFrameHeader.o AacAdtsFrameReader.o AacAdtsFrame.o

BINOBJS=read.o

CXXFLAGS=-std=c++17 -Wall -Wshadow -g `pkg-config --cflags sdl2`

HUFFTABLES=tables/huffman-table-scalefactor.c \
	tables/huffman-table-spectrum-1.c \
	tables/huffman-table-spectrum-2.c \
	tables/huffman-table-spectrum-3.c \
	tables/huffman-table-spectrum-4.c \
	tables/huffman-table-spectrum-5.c \
	tables/huffman-table-spectrum-6.c \
	tables/huffman-table-spectrum-7.c \
	tables/huffman-table-spectrum-8.c \
	tables/huffman-table-spectrum-9.c \
	tables/huffman-table-spectrum-10.c \
	tables/huffman-table-spectrum-11.c

.PHONY: bins
bins: $(HUFFTABLES) $(BINS)

tables/huffman-table-scalefactor.c: tables/huffman-table-scalefactor.txt
	./format-huffman-table.pl $< signed 1 60 > $@

tables/huffman-table-spectrum-1.c: tables/huffman-table-spectrum-1.txt
	./format-huffman-table.pl $< signed 4 1 > $@

tables/huffman-table-spectrum-2.c: tables/huffman-table-spectrum-2.txt
	./format-huffman-table.pl $< signed 4 1 > $@

tables/huffman-table-spectrum-3.c: tables/huffman-table-spectrum-3.txt
	./format-huffman-table.pl $< unsigned 4 2 > $@

tables/huffman-table-spectrum-4.c: tables/huffman-table-spectrum-4.txt
	./format-huffman-table.pl $< unsigned 4 2 > $@

tables/huffman-table-spectrum-5.c: tables/huffman-table-spectrum-5.txt
	./format-huffman-table.pl $< signed 2 4 > $@

tables/huffman-table-spectrum-6.c: tables/huffman-table-spectrum-6.txt
	./format-huffman-table.pl $< signed 2 4 > $@

tables/huffman-table-spectrum-7.c: tables/huffman-table-spectrum-7.txt
	./format-huffman-table.pl $< unsigned 2 7 > $@

tables/huffman-table-spectrum-8.c: tables/huffman-table-spectrum-8.txt
	./format-huffman-table.pl $< unsigned 2 7 > $@

tables/huffman-table-spectrum-9.c: tables/huffman-table-spectrum-9.txt
	./format-huffman-table.pl $< unsigned 2 12 > $@

tables/huffman-table-spectrum-10.c: tables/huffman-table-spectrum-10.txt
	./format-huffman-table.pl $< unsigned 2 12 > $@

tables/huffman-table-spectrum-11.c: tables/huffman-table-spectrum-11.txt
	./format-huffman-table.pl $< unsigned 2 16 > $@

read: $(OBJS) read.o
	g++ $(CXXFLAGS) -o read $(OBJS) read.o `pkg-config --libs sdl2`

%.o: %.cpp *.h $(HUFFTABLES)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINOBJS) $(BINS) $(HUFFTABLES)
