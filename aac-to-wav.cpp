#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <endian.h>

#include "AacAdtsFrameHeader.h"
#include "AacAdtsFrame.h"
#include "AacAdtsFrameReader.h"
#include "AacDecoder.h"
#include "AacAudioBlock.h"

#include "WavWriter.h"

uint8_t *mmapFile(const char *filename, size_t *sizePtr)
{
  int fd = open(filename, O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "open(): %s\n", strerror(errno));
    return NULL;
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size < 0)
  {
    fprintf(stderr, "lseek(): %s\n", strerror(errno));
    return NULL;
  }

  uint8_t *bytes = (uint8_t *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (bytes == MAP_FAILED)
  {
    fprintf(stderr, "mmap(): %s\n", strerror(errno));
    return NULL;
  }

  close(fd);

  *sizePtr = size;
  return bytes;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(1);
  }

  // Map the input file into memory
  size_t bytesSize;
  uint8_t *bytes = mmapFile(argv[1], &bytesSize);
  if (!bytes)
  {
    fprintf(stderr, "Couldn't open input file.\n");
    exit(1);
  }

  auto reader = AacAdtsFrameReader(bytes, bytesSize);

  // Skip over any initial ID3 tag
  if (size_t id3Size = reader.skipID3())
    printf("Skipped ID3 tag of %zd bytes.\n", id3Size);

  // Position reader at first frame header
  if (!reader.isAtFrameHeader())
    reader.findNextFrame();

  // Read frame header
  AacAdtsFrameHeader header;
  if (!reader.readFrameHeader(&header))
  {
    fprintf(stderr, "Could not find initial frame header.\n");
    exit(1);
  }

  header.dump();

  // Create WAV writer
  WavWriter writer;

  // Create decoder
  auto decoder = AacDecoder(header.getSampleRate());

  AacAudioBlock audio;

  while (!reader.isComplete())
  {
    auto frame = AacAdtsFrame();
    if (!reader.readFrame(&frame))
    {
      size_t skipped = reader.findNextFrame();
      printf("Skipped %zd bytes looking for a frame header.\n", skipped);
      continue;
    }

    frame.getHeader()->dump();

    if (decoder.getSampleRate() != frame.getHeader()->getSampleRate())
    {
      fprintf(stderr, "Detected sample rate change (%u -> %u)! Reinitializing decoder.\n", decoder.getSampleRate(), frame.getHeader()->getSampleRate());
      decoder = AacDecoder(frame.getHeader()->getSampleRate());
    }

    if (!decoder.decodeBlock(frame.getReader(), &audio))
    {
      fprintf(stderr, "Failed to decode block\n");
      exit(1);
    }

    // Open output file, if not yet open
    if (!writer.isOpen())
    {
      if (!writer.open("out.wav", audio.getChannelCount(), 16, audio.getSampleRate()))
      {
        fprintf(stderr, "Could not open output file: %s\n", strerror(errno));
        exit(1);
      }
    }

    // Ensure little-endian samples
    audio.switchEndianness(std::endian::little);

    // Write to output file
    int16_t *buf;
    auto size = audio.getSampleBuffer(&buf);
    if (!writer.write(reinterpret_cast<uint8_t *>(buf), size))
    {
      fprintf(stderr, "Could not write to output file\n");
      exit(1);
    }

    // Advance the reader
    size_t frameSize = frame.getSize();
    reader.advance(frameSize);
  }

  writer.close();

  return 0;
}
