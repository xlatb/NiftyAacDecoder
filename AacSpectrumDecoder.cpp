#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <array>

#include "AacConstants.h"
#include "AacBitReader.h"

#include "AacSpectrumDecoder.h"

#define AAC_SPECTRUM_ESC_VALUE 16

// Huffman table with 2 values per entry
struct AacSpectrumHuffman2
{
  unsigned int count;  // Number of entries
  unsigned int maxBits;  // Bit length of longest codeword
  struct { unsigned int len; unsigned int codeword; int8_t v1; int8_t v2; } entries[];
};

// Huffman tbale with 4 values per entry
struct AacSpectrumHuffman4
{
  unsigned int count;  // Number of entries
  unsigned int maxBits;  // Bit length of longest codeword
  struct { unsigned int len; unsigned int codeword; int8_t v1; int8_t v2; int8_t v3; int8_t v4; } entries[];
};

static const AacSpectrumHuffman4 codebook1 =
#include "tables/huffman-table-spectrum-1.c"

static const AacSpectrumHuffman4 codebook2 =
#include "tables/huffman-table-spectrum-2.c"

static const AacSpectrumHuffman4 codebook3 =
#include "tables/huffman-table-spectrum-3.c"

static const AacSpectrumHuffman4 codebook4 =
#include "tables/huffman-table-spectrum-4.c"

static const AacSpectrumHuffman2 codebook5 =
#include "tables/huffman-table-spectrum-5.c"

static const AacSpectrumHuffman2 codebook6 =
#include "tables/huffman-table-spectrum-6.c"

static const AacSpectrumHuffman2 codebook7 =
#include "tables/huffman-table-spectrum-7.c"

static const AacSpectrumHuffman2 codebook8 =
#include "tables/huffman-table-spectrum-8.c"

static const AacSpectrumHuffman2 codebook9 =
#include "tables/huffman-table-spectrum-9.c"

static const AacSpectrumHuffman2 codebook10 =
#include "tables/huffman-table-spectrum-10.c"

static const AacSpectrumHuffman2 codebook11 =
#include "tables/huffman-table-spectrum-11.c"

static const struct
{
  bool        isSigned;
  int         dimension;
  const void *codebook;
} codebooks[] =
{
  {false, 0, NULL},

  {true,  4, &codebook1},
  {true,  4, &codebook2},
  {false, 4, &codebook3},
  {false, 4, &codebook4},

  {true,  2, &codebook5},
  {true,  2, &codebook6},
  {false, 2, &codebook7},
  {false, 2, &codebook8},
  {false, 2, &codebook9},
  {false, 2, &codebook10},

  {false, 2, &codebook11},
};

constexpr unsigned int codebookCount = std::size(codebooks);

bool AacSpectrumDecoder::decode2(unsigned int tableNum, int *y, int *z)
{
  assert(tableNum < codebookCount);
  assert(codebooks[tableNum].dimension == 2);

  const AacSpectrumHuffman2 *huffmanTable = reinterpret_cast<const AacSpectrumHuffman2 *>(codebooks[tableNum].codebook);

  unsigned int codeword = m_reader->readUInt(1);
  unsigned int len = 1;
  unsigned int i = 0;

  while (true)
  {
    printf("AacSpectrumDecoder::decode2(): i %d  len %d  codeword 0x%X  v1 %d  v2 %d\n", i, len, codeword, huffmanTable->entries[i].v1, huffmanTable->entries[i].v2);
    // If we've hit an entry with more bits than we have, read more bits
    if (len < huffmanTable->entries[i].len)
    {
      unsigned int readCount = huffmanTable->entries[i].len - len;
      unsigned int bits = m_reader->readUInt(readCount);
      codeword = (codeword << readCount) | bits;
      len += readCount;
    }

    // Check each entry of the current length
    while (huffmanTable->entries[i].len == len)
    {
      if (huffmanTable->entries[i].codeword == codeword)
      {
        int8_t v1 = huffmanTable->entries[i].v1;
        int8_t v2 = huffmanTable->entries[i].v2;

        // Read sign bits if needed, but don't act on them yet
        bool sign1 = false;
        bool sign2 = false;
        if (!codebooks[tableNum].isSigned)
        {
          // Read extra sign bits for non-zero coefficients
          if (v1 != 0) sign1 = m_reader->readUInt(1);
          if (v2 != 0) sign2 = m_reader->readUInt(1);
        }

        // Read escapes if needed
        if (tableNum == AAC_HCB_ESC)
        {
          if (v1 == AAC_SPECTRUM_ESC_VALUE) v1 = decodeEscape();
          if (v2 == AAC_SPECTRUM_ESC_VALUE) v2 = decodeEscape();
        }

        // Apply sign bits
        if (!codebooks[tableNum].isSigned)
        {
          if (sign1) v1 = -v1;
          if (sign2) v2 = -v2;
        }

        *y = v1;
        *z = v2;
        return true;
      }

      i++;
      if (i >= huffmanTable->count)
        return false;  // Not found
    }
  }

  // Not reached
  abort();
}

bool AacSpectrumDecoder::decode4(unsigned int tableNum, int *w, int *x, int *y, int *z)
{
  assert(tableNum < codebookCount);
  assert(codebooks[tableNum].dimension == 2);

  const AacSpectrumHuffman4 *huffmanTable = reinterpret_cast<const AacSpectrumHuffman4 *>(codebooks[tableNum].codebook);

  unsigned int codeword = m_reader->readUInt(1);
  unsigned int len = 1;
  unsigned int i = 0;

  while (true)
  {
    printf("AacSpectrumDecoder::decode4(): i %d  len %d  codeword 0x%X  v1 %d  v2 %d  v3 %d  v4 %d\n", i, len, codeword, huffmanTable->entries[i].v1, huffmanTable->entries[i].v2, huffmanTable->entries[i].v3, huffmanTable->entries[i].v4);
    // If we've hit an entry with more bits than we have, read more bits
    if (len < huffmanTable->entries[i].len)
    {
      unsigned int readCount = huffmanTable->entries[i].len - len;
      unsigned int bits = m_reader->readUInt(readCount);
      codeword = (codeword << readCount) | bits;
      len += readCount;
    }

    // Check each entry of the current length
    while (huffmanTable->entries[i].len == len)
    {
      if (huffmanTable->entries[i].codeword == codeword)
      {
        int8_t v1 = huffmanTable->entries[i].v1;
        int8_t v2 = huffmanTable->entries[i].v2;
        int8_t v3 = huffmanTable->entries[i].v3;
        int8_t v4 = huffmanTable->entries[i].v4;

        if (!codebooks[tableNum].isSigned)
        {
          // Read extra sign bits for non-zero coefficients
          if ((v1 != 0) && m_reader->readUInt(1)) v1 = -v1;
          if ((v2 != 0) && m_reader->readUInt(1)) v2 = -v2;
          if ((v3 != 0) && m_reader->readUInt(1)) v3 = -v3;
          if ((v4 != 0) && m_reader->readUInt(1)) v4 = -v4;
        }

        *w = v1;
        *x = v2;
        *y = v3;
        *z = v4;
        return true;
      }

      i++;
      if (i >= huffmanTable->count)
        return false;  // Not found
    }
  }

  // Not reached
  abort();
}

// Escape decoding for table 11.
unsigned int AacSpectrumDecoder::decodeEscape(void)
{
  // Count the number of consecutive 1 bits
  unsigned int len = 0;
  while (m_reader->readUInt(1))
  {
    len++;
  }

  // Read the escape word bits
  unsigned int word = m_reader->readUInt(len);

  return (2 << (len + 3)) | word;
}
