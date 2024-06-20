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

#include "AacAdtsFrameHeader.h"
#include "AacAdtsFrame.h"
#include "AacAdtsFrameReader.h"
#include "AacDecoder.h"

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "%s: open(): %s\n", argv[0], strerror(errno));
    exit(1);
  }

  off_t size = lseek(fd, 0, SEEK_END);
  if (size < 0)
  {
    fprintf(stderr, "%s: lseek(): %s\n", argv[0], strerror(errno));
    exit(1);
  }

  uint8_t *bytes = (uint8_t *) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (bytes == MAP_FAILED)
  {
    fprintf(stderr, "%s: mmap(): %s\n", argv[0], strerror(errno));
    exit(1);
  }

  auto reader = AacAdtsFrameReader(bytes, size);

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

  // Create decoder
  auto decoder = AacDecoder(header.getSampleRate());

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

    if (!decoder.decodeBlock(frame.getReader()))
    {
      fprintf(stderr, "Failed to decode block\n");
      exit(1);
    }

    size_t frameSize = frame.getSize();
    reader.advance(frameSize);
  }

  return 0;
}
