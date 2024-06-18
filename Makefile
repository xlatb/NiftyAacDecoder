BINS=read
OBJS=AacBitReader.o AacDecoder.o AacScalefactorDecoder.o AacSpectrumDecoder.o \
	AacAdtsFrameHeader.o AacAdtsFrameReader.o AacAdtsFrame.o
CXXFLAGS=-std=c++14 -Wall -Wshadow -g `pkg-config --cflags sdl2`

.PHONY: bins
bins: $(BINS)

read: $(OBJS) read.o
	g++ $(CXXFLAGS) -o read $(OBJS) read.o `pkg-config --libs sdl2`

%.o: %.cpp *.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINS)
