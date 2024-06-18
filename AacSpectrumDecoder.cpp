#include <stdint.h>

#include "AacSpectrumDecoder.h"

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
