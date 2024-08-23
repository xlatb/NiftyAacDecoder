#include <stdio.h>

#include "AacBitReader.h"

#include "AacScalefactorDecoder.h"

struct AacScalefactorHuffman
{
  unsigned int count;  // Number of entries
  unsigned int maxBits;  // Bit length of longest codeword
  struct { unsigned int len; unsigned int codeword; int8_t index; } entries[];
};

static const AacScalefactorHuffman huffmanTable =
#include "tables/huffman-table-scalefactor.c"

bool AacScalefactorDecoder::decode(int *scalefactorIndex)
{
  unsigned int codeword = m_reader->readBit();
  unsigned int len = 1;
  unsigned int i = 0;

  while (true)
  {
    //printf("AacScalefactorDecoder::decode(): i %d  len %d  codeword 0x%X  index %d\n", i, len, codeword, huffmanTable.entries[i].index);
    // If we've hit an entry with more bits than we have, read more bits
    if (len < huffmanTable.entries[i].len)
    {
      unsigned int readCount = huffmanTable.entries[i].len - len;
      unsigned int bits = m_reader->readUInt(readCount);
      codeword = (codeword << readCount) | bits;
      len += readCount;
    }

    // Check each entry of the current length
    while (huffmanTable.entries[i].len == len)
    {
      if (huffmanTable.entries[i].codeword == codeword)
      {
        *scalefactorIndex = huffmanTable.entries[i].index;
        return true;
      }

      i++;
      if (i >= huffmanTable.count)
        return false;  // Not found
    }
  }

  // Not reached
  abort();
}
