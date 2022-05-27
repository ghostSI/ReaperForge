#include "ogg.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// find definition of alloca if it's not in stdlib.h:
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#endif
#if defined(__linux__) || defined(__linux) || defined(__sun__) || defined(__EMSCRIPTEN__) || defined(__NEWLIB__)
#include <alloca.h>
#endif


#define STB_VORBIS_MAX_CHANNELS    16  // enough for anyone?
#define STB_VORBIS_PUSHDATA_CRC_COUNT  4
#define STB_VORBIS_FAST_HUFFMAN_LENGTH   10
#define STB_VORBIS_FAST_HUFFMAN_SHORT





#ifdef __MINGW32__
   // eff you mingw:
   //     "fixed":
   //         http://sourceforge.net/p/mingw-w64/mailman/message/32882927/
   //     "no that broke the build, reverted, who cares about C":
   //         http://sourceforge.net/p/mingw-w64/mailman/message/32890381/
#ifdef __forceinline
#undef __forceinline
#endif
#define __forceinline
#ifndef alloca
#define alloca __builtin_alloca
#endif
#elif !defined(_MSC_VER)
#if __GNUC__
#define __forceinline inline
#else
#define __forceinline
#endif
#endif


#define CHECK(f)   ((void) 0)

#define MAX_BLOCKSIZE_LOG  13   // from specification
#define MAX_BLOCKSIZE      (1 << MAX_BLOCKSIZE_LOG)

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef f32 codetype;

#ifdef _MSC_VER
#define STBV_NOTUSED(v)  (void)(v)
#else
#define STBV_NOTUSED(v)  (void)sizeof(v)
#endif

// @NOTE
//
// Some arrays below are tagged "//varies", which means it's actually
// a variable-sized piece of data, but rather than malloc I assume it's
// small enough it's better to just allocate it all together with the
// main thing
//
// Most of the variables are specified with the smallest size I could pack
// them into. It might give better performance to make them all full-sized
// integers. It should be safe to freely rearrange the structures or change
// the sizes larger--nothing relies on silently truncating etc., nor the
// order of variables.

#define FAST_HUFFMAN_TABLE_SIZE   (1 << STB_VORBIS_FAST_HUFFMAN_LENGTH)
#define FAST_HUFFMAN_TABLE_MASK   (FAST_HUFFMAN_TABLE_SIZE - 1)

enum STBVorbisError
{
  VORBIS__no_error,

  VORBIS_need_more_data = 1,             // not a real error

  VORBIS_invalid_api_mixing,           // can't mix API modes
  VORBIS_outofmem,                     // not enough memory
  VORBIS_feature_not_supported,        // uses floor 0
  VORBIS_too_many_channels,            // STB_VORBIS_MAX_CHANNELS is too small
  VORBIS_file_open_failure,            // fopen() failed
  VORBIS_seek_without_length,          // can't seek in unknown-length file

  VORBIS_unexpected_eof = 10,            // file is truncated?
  VORBIS_seek_invalid,                 // seek past EOF

  // decoding errors (corrupt/invalid stream) -- you probably
  // don't care about the exact details of these

  // vorbis errors:
  VORBIS_invalid_setup = 20,
  VORBIS_invalid_stream,

  // ogg errors:
  VORBIS_missing_capture_pattern = 30,
  VORBIS_invalid_stream_structure_version,
  VORBIS_continued_packet_flag_invalid,
  VORBIS_incorrect_stream_serial_number,
  VORBIS_invalid_first_page,
  VORBIS_bad_packet_type,
  VORBIS_cant_find_last_page,
  VORBIS_seek_failed,
  VORBIS_ogg_skeleton_not_supported
};

struct stb_vorbis_alloc
{
  char* alloc_buffer;
  i32   alloc_buffer_length_in_bytes;
};

struct Codebook
{
  i32 dimensions, entries;
  u8* codeword_lengths;
  f32  minimum_value;
  f32  delta_value;
  u8  value_bits;
  u8  lookup_type;
  u8  sequence_p;
  u8  sparse;
  u32 lookup_values;
  codetype* multiplicands;
  u32* codewords;
  i16  fast_huffman[FAST_HUFFMAN_TABLE_SIZE];
  u32* sorted_codewords;
  i32* sorted_values;
  i32     sorted_entries;
};

struct Floor0
{
  u8 order;
  u16 rate;
  u16 bark_map_size;
  u8 amplitude_bits;
  u8 amplitude_offset;
  u8 number_of_books;
  u8 book_list[16]; // varies
};

struct Floor1
{
  u8 partitions;
  u8 partition_class_list[32]; // varies
  u8 class_dimensions[16]; // varies
  u8 class_subclasses[16]; // varies
  u8 class_masterbooks[16]; // varies
  i16 subclass_books[16][8]; // varies
  u16 Xlist[31 * 8 + 2]; // varies
  u8 sorted_order[31 * 8 + 2];
  u8 neighbors[31 * 8 + 2][2];
  u8 floor1_multiplier;
  u8 rangebits;
  i32 values;
};

union Floor
{
  Floor0 floor0;
  Floor1 floor1;
};

struct Residue
{
  u32 begin, end;
  u32 part_size;
  u8 classifications;
  u8 classbook;
  u8** classdata;
  i16(*residue_books)[8];
};

struct MappingChannel
{
  u8 magnitude;
  u8 angle;
  u8 mux;
};

struct Mapping
{
  u16 coupling_steps;
  MappingChannel* chan;
  u8  submaps;
  u8  submap_floor[15]; // varies
  u8  submap_residue[15]; // varies
};

struct Mode
{
  u8 blockflag;
  u8 mapping;
  u16 windowtype;
  u16 transformtype;
};

struct CRCscan
{
  u32  goal_crc;    // expected crc if match
  i32     bytes_left;  // bytes left in packet
  u32  crc_so_far;  // running crc
  i32     bytes_done;  // bytes processed in _current_ chunk
  u32  sample_loc;  // granule pos encoded in page
};

struct ProbedPage
{
  u32 page_start, page_end;
  u32 last_decoded_sample;
};

namespace Ogg
{
  struct vorbis
  {
    // user-accessible info
    u32 sample_rate;
    i32 channels;

    u32 setup_memory_required;
    u32 temp_memory_required;
    u32 setup_temp_memory_required;

    char* vendor;
    i32 comment_list_length;
    char** comment_list;

    u8* stream;
    u8* stream_start;
    u8* stream_end;

    u32 stream_len;

    u8  push_mode;

    // the page to seek to when seeking to start, may be zero
    u32 first_audio_page_offset;

    // p_first is the page on which the first audio packet ends
    // (but not necessarily the page on which it starts)
    ProbedPage p_first, p_last;

    // memory management
    stb_vorbis_alloc alloc;
    i32 setup_offset;
    i32 temp_offset;

    // run-time results
    i32 eof;
    enum STBVorbisError error;

    // user-useful data

    // header info
    i32 blocksize[2];
    i32 blocksize_0, blocksize_1;
    i32 codebook_count;
    Codebook* codebooks;
    i32 floor_count;
    u16 floor_types[64]; // varies
    Floor* floor_config;
    i32 residue_count;
    u16 residue_types[64]; // varies
    Residue* residue_config;
    i32 mapping_count;
    Mapping* mapping;
    i32 mode_count;
    Mode mode_config[64];  // varies

    u32 total_samples;

    // decode buffer
    f32* channel_buffers[STB_VORBIS_MAX_CHANNELS];
    f32* outputs[STB_VORBIS_MAX_CHANNELS];

    f32* previous_window[STB_VORBIS_MAX_CHANNELS];
    i32 previous_length;

    i16* finalY[STB_VORBIS_MAX_CHANNELS];

    u32 current_loc; // sample location of next frame to decode
    i32    current_loc_valid;

    // per-blocksize precomputed data

     // twiddle factors
    f32* A[2], * B[2], * C[2];
    f32* window[2];
    u16* bit_reverse[2];

    // current page/packet/segment streaming info
    u32 serial; // stream serial number for verification
    i32 last_page;
    i32 segment_count;
    u8 segments[255];
    u8 page_flag;
    u8 bytes_in_seg;
    u8 first_decode;
    i32 next_seg;
    i32 last_seg;  // flag that we're on the last segment
    i32 last_seg_which; // what was the segment number of the last seg?
    u32 acc;
    i32 valid_bits;
    i32 packet_bytes;
    i32 end_seg_with_known_loc;
    u32 known_loc_for_packet;
    i32 discard_samples_deferred;
    u32 samples_output;

    // push mode scanning
    i32 page_crc_tests; // only in push_mode: number of tests active; -1 if not searching

   // sample-access
    i32 channel_buffer_start;
    i32 channel_buffer_end;
  };
}

#define IS_PUSH_MODE(f)   FALSE

i32 stb_vorbis_get_error(Ogg::vorbis* f);
i32 stb_vorbis_get_sample_offset(Ogg::vorbis* f);
u32 stb_vorbis_get_file_offset(Ogg::vorbis* f);
i32 stb_vorbis_seek_frame(Ogg::vorbis* f, u32 sample_number);
i32 stb_vorbis_seek(Ogg::vorbis* f, u32 sample_number);
i32 stb_vorbis_seek_start(Ogg::vorbis* f);
u32 stb_vorbis_stream_length_in_samples(Ogg::vorbis* f);
f32        stb_vorbis_stream_length_in_seconds(Ogg::vorbis* f);
i32 stb_vorbis_get_frame_float(Ogg::vorbis* f, i32* channels, f32*** output);
i32 stb_vorbis_get_samples_float(Ogg::vorbis* f, i32 channels, f32** buffer, i32 num_samples);

static i32 error(Ogg::vorbis* f, enum STBVorbisError e)
{
  f->error = e;
  if (!f->eof && e != VORBIS_need_more_data) {
    f->error = e; // breakpoint for debugging
  }
  return 0;
}


// these functions are used for allocating temporary memory
// while decoding. if you can afford the stack space, use
// alloca(); otherwise, provide a temp buffer and it will
// allocate out of those.

#define array_size_required(count,size)  (count*(sizeof(void *)+(size)))

#define temp_alloc(f,size)              (f->alloc.alloc_buffer ? setup_temp_malloc(f,size) : alloca(size))
#define temp_free(f,p)                  (void)0
#define temp_alloc_save(f)              ((f)->temp_offset)
#define temp_alloc_restore(f,p)         ((f)->temp_offset = (p))

#define temp_block_array(f,count,size)  make_block_array(temp_alloc(f,array_size_required(count,size)), count, size)

// given a sufficiently large block of memory, make an array of pointers to subblocks of it
static void* make_block_array(void* mem, i32 count, i32 size)
{
  i32 i;
  void** p = (void**)mem;
  char* q = (char*)(p + count);
  for (i = 0; i < count; ++i) {
    p[i] = q;
    q += size;
  }
  return p;
}

static void* setup_malloc(Ogg::vorbis* f, i32 sz)
{
  sz = (sz + 7) & ~7; // round up to nearest 8 for alignment of future allocs.
  f->setup_memory_required += sz;
  if (f->alloc.alloc_buffer) {
    void* p = (char*)f->alloc.alloc_buffer + f->setup_offset;
    if (f->setup_offset + sz > f->temp_offset) return nullptr;
    f->setup_offset += sz;
    return p;
  }
  return sz ? malloc(sz) : nullptr;
}

static void setup_free(Ogg::vorbis* f, void* p)
{
  if (f->alloc.alloc_buffer) return; // do nothing; setup mem is a stack
  free(p);
}

static void* setup_temp_malloc(Ogg::vorbis* f, i32 sz)
{
  sz = (sz + 7) & ~7; // round up to nearest 8 for alignment of future allocs.
  if (f->alloc.alloc_buffer) {
    if (f->temp_offset - sz < f->setup_offset) return nullptr;
    f->temp_offset -= sz;
    return (char*)f->alloc.alloc_buffer + f->temp_offset;
  }
  return malloc(sz);
}

static void setup_temp_free(Ogg::vorbis* f, void* p, i32 sz)
{
  if (f->alloc.alloc_buffer) {
    f->temp_offset += (sz + 7) & ~7;
    return;
  }
  free(p);
}

#define CRC32_POLY    0x04c11db7   // from spec

static u32 crc_table[256];
static void crc32_init(void)
{
  i32 i, j;
  u32 s;
  for (i = 0; i < 256; i++) {
    for (s = (u32)i << 24, j = 0; j < 8; ++j)
      s = (s << 1) ^ (s >= (1U << 31) ? CRC32_POLY : 0);
    crc_table[i] = s;
  }
}

static __forceinline u32 crc32_update(u32 crc, u8 byte)
{
  return (crc << 8) ^ crc_table[byte ^ (crc >> 24)];
}


// used in setup, and for huffman that doesn't go fast path
static u32 bit_reverse(u32 n)
{
  n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1);
  n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2);
  n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
  n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
  return (n >> 16) | (n << 16);
}

static f32 square(f32 x)
{
  return x * x;
}

// this is a weird definition of log2() for which log2(1) = 1, log2(2) = 2, log2(4) = 3
// as required by the specification. fast(?) implementation from stb.h
// @OPTIMIZE: called multiple times per-packet with "constants"; move to setup
static i32 ilog(i32 n)
{
  static signed char log2_4[16] = { 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 };

  if (n < 0) return 0; // signed n returns 0

  // 2 compares if n < 16, 3 compares otherwise (4 if signed or n > 1<<29)
  if (n < (1 << 14))
    if (n < (1 << 4))            return  0 + log2_4[n];
    else if (n < (1 << 9))       return  5 + log2_4[n >> 5];
    else                     return 10 + log2_4[n >> 10];
  else if (n < (1 << 24))
    if (n < (1 << 19))       return 15 + log2_4[n >> 15];
    else                     return 20 + log2_4[n >> 20];
  else if (n < (1 << 29))       return 25 + log2_4[n >> 25];
  else                     return 30 + log2_4[n >> 30];
}

#ifndef M_PI
#define M_PI  3.14159265358979323846264f  // from CRC
#endif

// code length assigned to a value with no huffman encoding
#define NO_CODE   255

/////////////////////// LEAF SETUP FUNCTIONS //////////////////////////
//
// these functions are only called at setup, and only a few times
// per file

static f32 float32_unpack(u32 x)
{
  // from the specification
  u32 mantissa = x & 0x1fffff;
  u32 sign = x & 0x80000000;
  u32 exp = (x & 0x7fe00000) >> 21;
  double res = sign ? -(double)mantissa : (double)mantissa;
  return (f32)ldexp((f32)res, (i32)exp - 788);
}


// zlib & jpeg huffman tables assume that the output symbols
// can either be arbitrarily arranged, or have monotonically
// increasing frequencies--they rely on the lengths being sorted;
// this makes for a very simple generation algorithm.
// vorbis allows a huffman table with non-sorted lengths. This
// requires a more sophisticated construction, since symbols in
// order do not map to huffman codes "in order".
static void add_entry(Codebook* c, u32 huff_code, i32 symbol, i32 count, i32 len, u32* values)
{
  if (!c->sparse) {
    c->codewords[symbol] = huff_code;
  }
  else {
    c->codewords[count] = huff_code;
    c->codeword_lengths[count] = len;
    values[count] = symbol;
  }
}

static i32 compute_codewords(Codebook* c, u8* len, i32 n, u32* values)
{
  i32 i, k, m = 0;
  u32 available[32];

  memset(available, 0, sizeof(available));
  // find the first entry
  for (k = 0; k < n; ++k) if (len[k] < NO_CODE) break;
  if (k == n) { assert(c->sorted_entries == 0); return TRUE; }
  assert(len[k] < 32); // no error return required, code reading lens checks this
  // add to the list
  add_entry(c, 0, k, m++, len[k], values);
  // add all available leaves
  for (i = 1; i <= len[k]; ++i)
    available[i] = 1U << (32 - i);
  // note that the above code treats the first case specially,
  // but it's really the same as the following code, so they
  // could probably be combined (except the initial code is 0,
  // and I use 0 in available[] to mean 'empty')
  for (i = k + 1; i < n; ++i) {
    u32 res;
    i32 z = len[i], y;
    if (z == NO_CODE) continue;
    assert(z < 32); // no error return required, code reading lens checks this
    // find lowest available leaf (should always be earliest,
    // which is what the specification calls for)
    // note that this property, and the fact we can never have
    // more than one free leaf at a given level, isn't totally
    // trivial to prove, but it seems true and the assert never
    // fires, so!
    while (z > 0 && !available[z]) --z;
    if (z == 0) { return FALSE; }
    res = available[z];
    available[z] = 0;
    add_entry(c, bit_reverse(res), i, m++, len[i], values);
    // propagate availability up the tree
    if (z != len[i]) {
      for (y = len[i]; y > z; --y) {
        assert(available[y] == 0);
        available[y] = res + (1 << (32 - y));
      }
    }
  }
  return TRUE;
}

// accelerated huffman table allows fast O(1) match of all symbols
// of length <= STB_VORBIS_FAST_HUFFMAN_LENGTH
static void compute_accelerated_huffman(Codebook* c)
{
  i32 i, len;
  for (i = 0; i < FAST_HUFFMAN_TABLE_SIZE; ++i)
    c->fast_huffman[i] = -1;

  len = c->sparse ? c->sorted_entries : c->entries;
#ifdef STB_VORBIS_FAST_HUFFMAN_SHORT
  if (len > 32767) len = 32767; // largest possible value we can encode!
#endif
  for (i = 0; i < len; ++i) {
    if (c->codeword_lengths[i] <= STB_VORBIS_FAST_HUFFMAN_LENGTH) {
      u32 z = c->sparse ? bit_reverse(c->sorted_codewords[i]) : c->codewords[i];
      // set table entries for all bit combinations in the higher bits
      while (z < FAST_HUFFMAN_TABLE_SIZE) {
        c->fast_huffman[z] = i;
        z += 1 << c->codeword_lengths[i];
      }
    }
  }
}

#ifdef _MSC_VER
#define STBV_CDECL __cdecl
#else
#define STBV_CDECL
#endif

static i32 STBV_CDECL uint32_compare(const void* p, const void* q)
{
  u32 x = *(u32*)p;
  u32 y = *(u32*)q;
  return x < y ? -1 : x > y;
}

static i32 include_in_sort(Codebook* c, u8 len)
{
  if (c->sparse) { assert(len != NO_CODE); return TRUE; }
  if (len == NO_CODE) return FALSE;
  if (len > STB_VORBIS_FAST_HUFFMAN_LENGTH) return TRUE;
  return FALSE;
}

// if the fast table above doesn't work, we want to binary
// search them... need to reverse the bits
static void compute_sorted_huffman(Codebook* c, u8* lengths, u32* values)
{
  i32 i, len;
  // build a list of all the entries
  // OPTIMIZATION: don't include the short ones, since they'll be caught by FAST_HUFFMAN.
  // this is kind of a frivolous optimization--I don't see any performance improvement,
  // but it's like 4 extra lines of code, so.
  if (!c->sparse) {
    i32 k = 0;
    for (i = 0; i < c->entries; ++i)
      if (include_in_sort(c, lengths[i]))
        c->sorted_codewords[k++] = bit_reverse(c->codewords[i]);
    assert(k == c->sorted_entries);
  }
  else {
    for (i = 0; i < c->sorted_entries; ++i)
      c->sorted_codewords[i] = bit_reverse(c->codewords[i]);
  }

  qsort(c->sorted_codewords, c->sorted_entries, sizeof(c->sorted_codewords[0]), uint32_compare);
  c->sorted_codewords[c->sorted_entries] = 0xffffffff;

  len = c->sparse ? c->sorted_entries : c->entries;
  // now we need to indicate how they correspond; we could either
  //   #1: sort a different data structure that says who they correspond to
  //   #2: for each sorted entry, search the original list to find who corresponds
  //   #3: for each original entry, find the sorted entry
  // #1 requires extra storage, #2 is slow, #3 can use binary search!
  for (i = 0; i < len; ++i) {
    i32 huff_len = c->sparse ? lengths[values[i]] : lengths[i];
    if (include_in_sort(c, huff_len)) {
      u32 code = bit_reverse(c->codewords[i]);
      i32 x = 0, n = c->sorted_entries;
      while (n > 1) {
        // invariant: sc[x] <= code < sc[x+n]
        i32 m = x + (n >> 1);
        if (c->sorted_codewords[m] <= code) {
          x = m;
          n -= (n >> 1);
        }
        else {
          n >>= 1;
        }
      }
      assert(c->sorted_codewords[x] == code);
      if (c->sparse) {
        c->sorted_values[x] = values[i];
        c->codeword_lengths[x] = huff_len;
      }
      else {
        c->sorted_values[x] = i;
      }
    }
  }
}

// only run while parsing the header (3 times)
static i32 vorbis_validate(u8* data)
{
  static u8 vorbis[6] = { 'v', 'o', 'r', 'b', 'i', 's' };
  return memcmp(data, vorbis, 6) == 0;
}

// called from setup only, once per code book
// (formula implied by specification)
static i32 lookup1_values(i32 entries, i32 dim)
{
  i32 r = (i32)floor(exp((f32)log((f32)entries) / dim));
  if ((i32)floor(pow((f32)r + 1, dim)) <= entries)   // (i32) cast for MinGW warning;
    ++r;                                              // floor() to avoid _ftol() when non-CRT
  if (pow((f32)r + 1, dim) <= entries)
    return -1;
  if ((i32)floor(pow((f32)r, dim)) > entries)
    return -1;
  return r;
}

// called twice per file
static void compute_twiddle_factors(i32 n, f32* A, f32* B, f32* C)
{
  i32 n4 = n >> 2, n8 = n >> 3;
  i32 k, k2;

  for (k = k2 = 0; k < n4; ++k, k2 += 2) {
    A[k2] = (f32)cos(4 * k * M_PI / n);
    A[k2 + 1] = (f32)-sin(4 * k * M_PI / n);
    B[k2] = (f32)cos((k2 + 1) * M_PI / n / 2) * 0.5f;
    B[k2 + 1] = (f32)sin((k2 + 1) * M_PI / n / 2) * 0.5f;
  }
  for (k = k2 = 0; k < n8; ++k, k2 += 2) {
    C[k2] = (f32)cos(2 * (k2 + 1) * M_PI / n);
    C[k2 + 1] = (f32)-sin(2 * (k2 + 1) * M_PI / n);
  }
}

static void compute_window(i32 n, f32* window)
{
  i32 n2 = n >> 1, i;
  for (i = 0; i < n2; ++i)
    window[i] = (f32)sin(0.5 * M_PI * square((f32)sin((i - 0 + 0.5) / n2 * 0.5 * M_PI)));
}

static void compute_bitreverse(i32 n, u16* rev)
{
  i32 ld = ilog(n) - 1; // ilog is off-by-one from normal definitions
  i32 i, n8 = n >> 3;
  for (i = 0; i < n8; ++i)
    rev[i] = (bit_reverse(i) >> (32 - ld + 3)) << 2;
}

static i32 init_blocksize(Ogg::vorbis* f, i32 b, i32 n)
{
  i32 n2 = n >> 1, n4 = n >> 2, n8 = n >> 3;
  f->A[b] = (f32*)setup_malloc(f, sizeof(f32) * n2);
  f->B[b] = (f32*)setup_malloc(f, sizeof(f32) * n2);
  f->C[b] = (f32*)setup_malloc(f, sizeof(f32) * n4);
  if (!f->A[b] || !f->B[b] || !f->C[b]) return error(f, VORBIS_outofmem);
  compute_twiddle_factors(n, f->A[b], f->B[b], f->C[b]);
  f->window[b] = (f32*)setup_malloc(f, sizeof(f32) * n2);
  if (!f->window[b]) return error(f, VORBIS_outofmem);
  compute_window(n, f->window[b]);
  f->bit_reverse[b] = (u16*)setup_malloc(f, sizeof(u16) * n8);
  if (!f->bit_reverse[b]) return error(f, VORBIS_outofmem);
  compute_bitreverse(n, f->bit_reverse[b]);
  return TRUE;
}

static void neighbors(u16* x, i32 n, i32* plow, i32* phigh)
{
  i32 low = -1;
  i32 high = 65536;
  i32 i;
  for (i = 0; i < n; ++i) {
    if (x[i] > low && x[i] < x[n]) { *plow = i; low = x[i]; }
    if (x[i] < high && x[i] > x[n]) { *phigh = i; high = x[i]; }
  }
}

// this has been repurposed so y is now the original index instead of y
typedef struct
{
  u16 x, id;
} stbv__floor_ordering;

static i32 STBV_CDECL point_compare(const void* p, const void* q)
{
  stbv__floor_ordering* a = (stbv__floor_ordering*)p;
  stbv__floor_ordering* b = (stbv__floor_ordering*)q;
  return a->x < b->x ? -1 : a->x > b->x;
}

//
/////////////////////// END LEAF SETUP FUNCTIONS //////////////////////////

#define USE_MEMORY(z)    TRUE

static u8 get8(Ogg::vorbis* z)
{
  if (USE_MEMORY(z)) {
    if (z->stream >= z->stream_end) { z->eof = TRUE; return 0; }
    return *z->stream++;
  }
}

static u32 get32(Ogg::vorbis* f)
{
  u32 x;
  x = get8(f);
  x += get8(f) << 8;
  x += get8(f) << 16;
  x += (u32)get8(f) << 24;
  return x;
}

static i32 getn(Ogg::vorbis* z, u8* data, i32 n)
{
  if (USE_MEMORY(z)) {
    if (z->stream + n > z->stream_end) { z->eof = 1; return 0; }
    memcpy(data, z->stream, n);
    z->stream += n;
    return 1;
  }
}

static void skip(Ogg::vorbis* z, i32 n)
{
  if (USE_MEMORY(z)) {
    z->stream += n;
    if (z->stream >= z->stream_end) z->eof = 1;
    return;
  }
}

static i32 set_file_offset(Ogg::vorbis* f, u32 loc)
{
  f->eof = 0;
  if (USE_MEMORY(f)) {
    if (f->stream_start + loc >= f->stream_end || f->stream_start + loc < f->stream_start) {
      f->stream = f->stream_end;
      f->eof = 1;
      return 0;
    }
    else {
      f->stream = f->stream_start + loc;
      return 1;
    }
  }
}


static u8 ogg_page_header[4] = { 0x4f, 0x67, 0x67, 0x53 };

static i32 capture_pattern(Ogg::vorbis* f)
{
  if (0x4f != get8(f)) return FALSE;
  if (0x67 != get8(f)) return FALSE;
  if (0x67 != get8(f)) return FALSE;
  if (0x53 != get8(f)) return FALSE;
  return TRUE;
}

#define PAGEFLAG_continued_packet   1
#define PAGEFLAG_first_page         2
#define PAGEFLAG_last_page          4

static i32 start_page_no_capturepattern(Ogg::vorbis* f)
{
  u32 loc0, loc1, n;
  if (f->first_decode && !IS_PUSH_MODE(f)) {
    f->p_first.page_start = stb_vorbis_get_file_offset(f) - 4;
  }
  // stream structure version
  if (0 != get8(f)) return error(f, VORBIS_invalid_stream_structure_version);
  // header flag
  f->page_flag = get8(f);
  // absolute granule position
  loc0 = get32(f);
  loc1 = get32(f);
  // @TODO: validate loc0,loc1 as valid positions?
  // stream serial number -- vorbis doesn't interleave, so discard
  get32(f);
  //if (f->serial != get32(f)) return error(f, VORBIS_incorrect_stream_serial_number);
  // page sequence number
  n = get32(f);
  f->last_page = n;
  // CRC32
  get32(f);
  // page_segments
  f->segment_count = get8(f);
  if (!getn(f, f->segments, f->segment_count))
    return error(f, VORBIS_unexpected_eof);
  // assume we _don't_ know any the sample position of any segments
  f->end_seg_with_known_loc = -2;
  if (loc0 != ~0U || loc1 != ~0U) {
    i32 i;
    // determine which packet is the last one that will complete
    for (i = f->segment_count - 1; i >= 0; --i)
      if (f->segments[i] < 255)
        break;
    // 'i' is now the index of the _last_ segment of a packet that ends
    if (i >= 0) {
      f->end_seg_with_known_loc = i;
      f->known_loc_for_packet = loc0;
    }
  }
  if (f->first_decode) {
    i32 i, len;
    len = 0;
    for (i = 0; i < f->segment_count; ++i)
      len += f->segments[i];
    len += 27 + f->segment_count;
    f->p_first.page_end = f->p_first.page_start + len;
    f->p_first.last_decoded_sample = loc0;
  }
  f->next_seg = 0;
  return TRUE;
}

static i32 start_page(Ogg::vorbis* f)
{
  if (!capture_pattern(f)) return error(f, VORBIS_missing_capture_pattern);
  return start_page_no_capturepattern(f);
}

static i32 start_packet(Ogg::vorbis* f)
{
  while (f->next_seg == -1) {
    if (!start_page(f)) return FALSE;
    if (f->page_flag & PAGEFLAG_continued_packet)
      return error(f, VORBIS_continued_packet_flag_invalid);
  }
  f->last_seg = FALSE;
  f->valid_bits = 0;
  f->packet_bytes = 0;
  f->bytes_in_seg = 0;
  // f->next_seg is now valid
  return TRUE;
}

static i32 maybe_start_packet(Ogg::vorbis* f)
{
  if (f->next_seg == -1) {
    i32 x = get8(f);
    if (f->eof) return FALSE; // EOF at page boundary is not an error!
    if (0x4f != x) return error(f, VORBIS_missing_capture_pattern);
    if (0x67 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
    if (0x67 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
    if (0x53 != get8(f)) return error(f, VORBIS_missing_capture_pattern);
    if (!start_page_no_capturepattern(f)) return FALSE;
    if (f->page_flag & PAGEFLAG_continued_packet) {
      // set up enough state that we can read this packet if we want,
      // e.g. during recovery
      f->last_seg = FALSE;
      f->bytes_in_seg = 0;
      return error(f, VORBIS_continued_packet_flag_invalid);
    }
  }
  return start_packet(f);
}

static i32 next_segment(Ogg::vorbis* f)
{
  i32 len;
  if (f->last_seg) return 0;
  if (f->next_seg == -1) {
    f->last_seg_which = f->segment_count - 1; // in case start_page fails
    if (!start_page(f)) { f->last_seg = 1; return 0; }
    if (!(f->page_flag & PAGEFLAG_continued_packet)) return error(f, VORBIS_continued_packet_flag_invalid);
  }
  len = f->segments[f->next_seg++];
  if (len < 255) {
    f->last_seg = TRUE;
    f->last_seg_which = f->next_seg - 1;
  }
  if (f->next_seg >= f->segment_count)
    f->next_seg = -1;
  assert(f->bytes_in_seg == 0);
  f->bytes_in_seg = len;
  return len;
}

#define EOP    (-1)
#define INVALID_BITS  (-1)

static i32 get8_packet_raw(Ogg::vorbis* f)
{
  if (!f->bytes_in_seg) {  // CLANG!
    if (f->last_seg) return EOP;
    else if (!next_segment(f)) return EOP;
  }
  assert(f->bytes_in_seg > 0);
  --f->bytes_in_seg;
  ++f->packet_bytes;
  return get8(f);
}

static i32 get8_packet(Ogg::vorbis* f)
{
  i32 x = get8_packet_raw(f);
  f->valid_bits = 0;
  return x;
}

static i32 get32_packet(Ogg::vorbis* f)
{
  u32 x;
  x = get8_packet(f);
  x += get8_packet(f) << 8;
  x += get8_packet(f) << 16;
  x += (u32)get8_packet(f) << 24;
  return x;
}

static void flush_packet(Ogg::vorbis* f)
{
  while (get8_packet_raw(f) != EOP);
}

// @OPTIMIZE: this is the secondary bit decoder, so it's probably not as important
// as the huffman decoder?
static u32 get_bits(Ogg::vorbis* f, i32 n)
{
  u32 z;

  if (f->valid_bits < 0) return 0;
  if (f->valid_bits < n) {
    if (n > 24) {
      // the accumulator technique below would not work correctly in this case
      z = get_bits(f, 24);
      z += get_bits(f, n - 24) << 24;
      return z;
    }
    if (f->valid_bits == 0) f->acc = 0;
    while (f->valid_bits < n) {
      i32 z = get8_packet_raw(f);
      if (z == EOP) {
        f->valid_bits = INVALID_BITS;
        return 0;
      }
      f->acc += z << f->valid_bits;
      f->valid_bits += 8;
    }
  }

  assert(f->valid_bits >= n);
  z = f->acc & ((1 << n) - 1);
  f->acc >>= n;
  f->valid_bits -= n;
  return z;
}

// @OPTIMIZE: primary accumulator for huffman
// expand the buffer to as many bits as possible without reading off end of packet
// it might be nice to allow f->valid_bits and f->acc to be stored in registers,
// e.g. cache them locally and decode locally
static __forceinline void prep_huffman(Ogg::vorbis* f)
{
  if (f->valid_bits <= 24) {
    if (f->valid_bits == 0) f->acc = 0;
    do {
      i32 z;
      if (f->last_seg && !f->bytes_in_seg) return;
      z = get8_packet_raw(f);
      if (z == EOP) return;
      f->acc += (u32)z << f->valid_bits;
      f->valid_bits += 8;
    } while (f->valid_bits <= 24);
  }
}

enum
{
  VORBIS_packet_id = 1,
  VORBIS_packet_comment = 3,
  VORBIS_packet_setup = 5
};

static i32 codebook_decode_scalar_raw(Ogg::vorbis* f, Codebook* c)
{
  i32 i;
  prep_huffman(f);

  if (c->codewords == nullptr && c->sorted_codewords == nullptr)
    return -1;

  // cases to use binary search: sorted_codewords && !c->codewords
  //                             sorted_codewords && c->entries > 8
  if (c->entries > 8 ? c->sorted_codewords != nullptr : !c->codewords) {
    // binary search
    u32 code = bit_reverse(f->acc);
    i32 x = 0, n = c->sorted_entries, len;

    while (n > 1) {
      // invariant: sc[x] <= code < sc[x+n]
      i32 m = x + (n >> 1);
      if (c->sorted_codewords[m] <= code) {
        x = m;
        n -= (n >> 1);
      }
      else {
        n >>= 1;
      }
    }
    // x is now the sorted index
    if (!c->sparse) x = c->sorted_values[x];
    // x is now sorted index if sparse, or symbol otherwise
    len = c->codeword_lengths[x];
    if (f->valid_bits >= len) {
      f->acc >>= len;
      f->valid_bits -= len;
      return x;
    }

    f->valid_bits = 0;
    return -1;
  }

  // if small, linear search
  assert(!c->sparse);
  for (i = 0; i < c->entries; ++i) {
    if (c->codeword_lengths[i] == NO_CODE) continue;
    if (c->codewords[i] == (f->acc & ((1 << c->codeword_lengths[i]) - 1))) {
      if (f->valid_bits >= c->codeword_lengths[i]) {
        f->acc >>= c->codeword_lengths[i];
        f->valid_bits -= c->codeword_lengths[i];
        return i;
      }
      f->valid_bits = 0;
      return -1;
    }
  }

  error(f, VORBIS_invalid_stream);
  f->valid_bits = 0;
  return -1;
}

#ifndef STB_VORBIS_NO_INLINE_DECODE

#define DECODE_RAW(var, f,c)                                  \
   if (f->valid_bits < STB_VORBIS_FAST_HUFFMAN_LENGTH)        \
      prep_huffman(f);                                        \
   var = f->acc & FAST_HUFFMAN_TABLE_MASK;                    \
   var = c->fast_huffman[var];                                \
   if (var >= 0) {                                            \
      i32 n = c->codeword_lengths[var];                       \
      f->acc >>= n;                                           \
      f->valid_bits -= n;                                     \
      if (f->valid_bits < 0) { f->valid_bits = 0; var = -1; } \
   } else {                                                   \
      var = codebook_decode_scalar_raw(f,c);                  \
   }

#endif

#define DECODE(var,f,c)                                       \
   DECODE_RAW(var,f,c)                                        \
   if (c->sparse) var = c->sorted_values[var];

#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
#define DECODE_VQ(var,f,c)   DECODE_RAW(var,f,c)
#else
#define DECODE_VQ(var,f,c)   DECODE(var,f,c)
#endif






// CODEBOOK_ELEMENT_FAST is an optimization for the CODEBOOK_FLOATS case
// where we avoid one addition
#define CODEBOOK_ELEMENT(c,off)          (c->multiplicands[off])
#define CODEBOOK_ELEMENT_FAST(c,off)     (c->multiplicands[off])
#define CODEBOOK_ELEMENT_BASE(c)         (0)

static i32 codebook_decode_start(Ogg::vorbis* f, Codebook* c)
{
  i32 z = -1;

  // type 0 is only legal in a scalar context
  if (c->lookup_type == 0)
    error(f, VORBIS_invalid_stream);
  else {
    DECODE_VQ(z, f, c);
    if (c->sparse) assert(z < c->sorted_entries);
    if (z < 0) {  // check for EOP
      if (!f->bytes_in_seg)
        if (f->last_seg)
          return z;
      error(f, VORBIS_invalid_stream);
    }
  }
  return z;
}

static i32 codebook_decode(Ogg::vorbis* f, Codebook* c, f32* output, i32 len)
{
  i32 i, z = codebook_decode_start(f, c);
  if (z < 0) return FALSE;
  if (len > c->dimensions) len = c->dimensions;

#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
  if (c->lookup_type == 1) {
    f32 last = CODEBOOK_ELEMENT_BASE(c);
    i32 div = 1;
    for (i = 0; i < len; ++i) {
      i32 off = (z / div) % c->lookup_values;
      f32 val = CODEBOOK_ELEMENT_FAST(c, off) + last;
      output[i] += val;
      if (c->sequence_p) last = val + c->minimum_value;
      div *= c->lookup_values;
    }
    return TRUE;
  }
#endif

  z *= c->dimensions;
  if (c->sequence_p) {
    f32 last = CODEBOOK_ELEMENT_BASE(c);
    for (i = 0; i < len; ++i) {
      f32 val = CODEBOOK_ELEMENT_FAST(c, z + i) + last;
      output[i] += val;
      last = val + c->minimum_value;
    }
  }
  else {
    f32 last = CODEBOOK_ELEMENT_BASE(c);
    for (i = 0; i < len; ++i) {
      output[i] += CODEBOOK_ELEMENT_FAST(c, z + i) + last;
    }
  }

  return TRUE;
}

static i32 codebook_decode_step(Ogg::vorbis* f, Codebook* c, f32* output, i32 len, i32 step)
{
  i32 i, z = codebook_decode_start(f, c);
  f32 last = CODEBOOK_ELEMENT_BASE(c);
  if (z < 0) return FALSE;
  if (len > c->dimensions) len = c->dimensions;

#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
  if (c->lookup_type == 1) {
    i32 div = 1;
    for (i = 0; i < len; ++i) {
      i32 off = (z / div) % c->lookup_values;
      f32 val = CODEBOOK_ELEMENT_FAST(c, off) + last;
      output[i * step] += val;
      if (c->sequence_p) last = val;
      div *= c->lookup_values;
    }
    return TRUE;
  }
#endif

  z *= c->dimensions;
  for (i = 0; i < len; ++i) {
    f32 val = CODEBOOK_ELEMENT_FAST(c, z + i) + last;
    output[i * step] += val;
    if (c->sequence_p) last = val;
  }

  return TRUE;
}

static i32 codebook_decode_deinterleave_repeat(Ogg::vorbis* f, Codebook* c, f32** outputs, i32 ch, i32* c_inter_p, i32* p_inter_p, i32 len, i32 total_decode)
{
  i32 c_inter = *c_inter_p;
  i32 p_inter = *p_inter_p;
  i32 i, z, effective = c->dimensions;

  // type 0 is only legal in a scalar context
  if (c->lookup_type == 0)   return error(f, VORBIS_invalid_stream);

  while (total_decode > 0) {
    f32 last = CODEBOOK_ELEMENT_BASE(c);
    DECODE_VQ(z, f, c);
#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
    assert(!c->sparse || z < c->sorted_entries);
#endif
    if (z < 0) {
      if (!f->bytes_in_seg)
        if (f->last_seg) return FALSE;
      return error(f, VORBIS_invalid_stream);
    }

    // if this will take us off the end of the buffers, stop short!
    // we check by computing the length of the virtual interleaved
    // buffer (len*ch), our current offset within it (p_inter*ch)+(c_inter),
    // and the length we'll be using (effective)
    if (c_inter + p_inter * ch + effective > len * ch) {
      effective = len * ch - (p_inter * ch - c_inter);
    }

#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
    if (c->lookup_type == 1) {
      i32 div = 1;
      for (i = 0; i < effective; ++i) {
        i32 off = (z / div) % c->lookup_values;
        f32 val = CODEBOOK_ELEMENT_FAST(c, off) + last;
        if (outputs[c_inter])
          outputs[c_inter][p_inter] += val;
        if (++c_inter == ch) { c_inter = 0; ++p_inter; }
        if (c->sequence_p) last = val;
        div *= c->lookup_values;
      }
    }
    else
#endif
    {
      z *= c->dimensions;
      if (c->sequence_p) {
        for (i = 0; i < effective; ++i) {
          f32 val = CODEBOOK_ELEMENT_FAST(c, z + i) + last;
          if (outputs[c_inter])
            outputs[c_inter][p_inter] += val;
          if (++c_inter == ch) { c_inter = 0; ++p_inter; }
          last = val;
        }
      }
      else {
        for (i = 0; i < effective; ++i) {
          f32 val = CODEBOOK_ELEMENT_FAST(c, z + i) + last;
          if (outputs[c_inter])
            outputs[c_inter][p_inter] += val;
          if (++c_inter == ch) { c_inter = 0; ++p_inter; }
        }
      }
    }

    total_decode -= effective;
  }
  *c_inter_p = c_inter;
  *p_inter_p = p_inter;
  return TRUE;
}

static i32 predict_point(i32 x, i32 x0, i32 x1, i32 y0, i32 y1)
{
  i32 dy = y1 - y0;
  i32 adx = x1 - x0;
  // @OPTIMIZE: force i32 division to round in the right direction... is this necessary on x86?
  i32 err = abs(dy) * (x - x0);
  i32 off = err / adx;
  return dy < 0 ? y0 - off : y0 + off;
}

// the following table is block-copied from the specification
static f32 inverse_db_table[256] =
{
  1.0649863e-07f, 1.1341951e-07f, 1.2079015e-07f, 1.2863978e-07f,
  1.3699951e-07f, 1.4590251e-07f, 1.5538408e-07f, 1.6548181e-07f,
  1.7623575e-07f, 1.8768855e-07f, 1.9988561e-07f, 2.1287530e-07f,
  2.2670913e-07f, 2.4144197e-07f, 2.5713223e-07f, 2.7384213e-07f,
  2.9163793e-07f, 3.1059021e-07f, 3.3077411e-07f, 3.5226968e-07f,
  3.7516214e-07f, 3.9954229e-07f, 4.2550680e-07f, 4.5315863e-07f,
  4.8260743e-07f, 5.1396998e-07f, 5.4737065e-07f, 5.8294187e-07f,
  6.2082472e-07f, 6.6116941e-07f, 7.0413592e-07f, 7.4989464e-07f,
  7.9862701e-07f, 8.5052630e-07f, 9.0579828e-07f, 9.6466216e-07f,
  1.0273513e-06f, 1.0941144e-06f, 1.1652161e-06f, 1.2409384e-06f,
  1.3215816e-06f, 1.4074654e-06f, 1.4989305e-06f, 1.5963394e-06f,
  1.7000785e-06f, 1.8105592e-06f, 1.9282195e-06f, 2.0535261e-06f,
  2.1869758e-06f, 2.3290978e-06f, 2.4804557e-06f, 2.6416497e-06f,
  2.8133190e-06f, 2.9961443e-06f, 3.1908506e-06f, 3.3982101e-06f,
  3.6190449e-06f, 3.8542308e-06f, 4.1047004e-06f, 4.3714470e-06f,
  4.6555282e-06f, 4.9580707e-06f, 5.2802740e-06f, 5.6234160e-06f,
  5.9888572e-06f, 6.3780469e-06f, 6.7925283e-06f, 7.2339451e-06f,
  7.7040476e-06f, 8.2047000e-06f, 8.7378876e-06f, 9.3057248e-06f,
  9.9104632e-06f, 1.0554501e-05f, 1.1240392e-05f, 1.1970856e-05f,
  1.2748789e-05f, 1.3577278e-05f, 1.4459606e-05f, 1.5399272e-05f,
  1.6400004e-05f, 1.7465768e-05f, 1.8600792e-05f, 1.9809576e-05f,
  2.1096914e-05f, 2.2467911e-05f, 2.3928002e-05f, 2.5482978e-05f,
  2.7139006e-05f, 2.8902651e-05f, 3.0780908e-05f, 3.2781225e-05f,
  3.4911534e-05f, 3.7180282e-05f, 3.9596466e-05f, 4.2169667e-05f,
  4.4910090e-05f, 4.7828601e-05f, 5.0936773e-05f, 5.4246931e-05f,
  5.7772202e-05f, 6.1526565e-05f, 6.5524908e-05f, 6.9783085e-05f,
  7.4317983e-05f, 7.9147585e-05f, 8.4291040e-05f, 8.9768747e-05f,
  9.5602426e-05f, 0.00010181521f, 0.00010843174f, 0.00011547824f,
  0.00012298267f, 0.00013097477f, 0.00013948625f, 0.00014855085f,
  0.00015820453f, 0.00016848555f, 0.00017943469f, 0.00019109536f,
  0.00020351382f, 0.00021673929f, 0.00023082423f, 0.00024582449f,
  0.00026179955f, 0.00027881276f, 0.00029693158f, 0.00031622787f,
  0.00033677814f, 0.00035866388f, 0.00038197188f, 0.00040679456f,
  0.00043323036f, 0.00046138411f, 0.00049136745f, 0.00052329927f,
  0.00055730621f, 0.00059352311f, 0.00063209358f, 0.00067317058f,
  0.00071691700f, 0.00076350630f, 0.00081312324f, 0.00086596457f,
  0.00092223983f, 0.00098217216f, 0.0010459992f,  0.0011139742f,
  0.0011863665f,  0.0012634633f,  0.0013455702f,  0.0014330129f,
  0.0015261382f,  0.0016253153f,  0.0017309374f,  0.0018434235f,
  0.0019632195f,  0.0020908006f,  0.0022266726f,  0.0023713743f,
  0.0025254795f,  0.0026895994f,  0.0028643847f,  0.0030505286f,
  0.0032487691f,  0.0034598925f,  0.0036847358f,  0.0039241906f,
  0.0041792066f,  0.0044507950f,  0.0047400328f,  0.0050480668f,
  0.0053761186f,  0.0057254891f,  0.0060975636f,  0.0064938176f,
  0.0069158225f,  0.0073652516f,  0.0078438871f,  0.0083536271f,
  0.0088964928f,  0.009474637f,   0.010090352f,   0.010746080f,
  0.011444421f,   0.012188144f,   0.012980198f,   0.013823725f,
  0.014722068f,   0.015678791f,   0.016697687f,   0.017782797f,
  0.018938423f,   0.020169149f,   0.021479854f,   0.022875735f,
  0.024362330f,   0.025945531f,   0.027631618f,   0.029427276f,
  0.031339626f,   0.033376252f,   0.035545228f,   0.037855157f,
  0.040315199f,   0.042935108f,   0.045725273f,   0.048696758f,
  0.051861348f,   0.055231591f,   0.058820850f,   0.062643361f,
  0.066714279f,   0.071049749f,   0.075666962f,   0.080584227f,
  0.085821044f,   0.091398179f,   0.097337747f,   0.10366330f,
  0.11039993f,    0.11757434f,    0.12521498f,    0.13335215f,
  0.14201813f,    0.15124727f,    0.16107617f,    0.17154380f,
  0.18269168f,    0.19456402f,    0.20720788f,    0.22067342f,
  0.23501402f,    0.25028656f,    0.26655159f,    0.28387361f,
  0.30232132f,    0.32196786f,    0.34289114f,    0.36517414f,
  0.38890521f,    0.41417847f,    0.44109412f,    0.46975890f,
  0.50028648f,    0.53279791f,    0.56742212f,    0.60429640f,
  0.64356699f,    0.68538959f,    0.72993007f,    0.77736504f,
  0.82788260f,    0.88168307f,    0.9389798f,     1.0f
};


// @OPTIMIZE: if you want to replace this bresenham line-drawing routine,
// note that you must produce bit-identical output to decode correctly;
// this specific sequence of operations is specified in the spec (it's
// drawing integer-quantized frequency-space lines that the encoder
// expects to be exactly the same)
//     ... also, isn't the whole point of Bresenham's algorithm to NOT
// have to divide in the setup? sigh.
#ifndef STB_VORBIS_NO_DEFER_FLOOR
#define LINE_OP(a,b)   a *= b
#else
#define LINE_OP(a,b)   a = b
#endif

#ifdef STB_VORBIS_DIVIDE_TABLE
#define DIVTAB_NUMER   32
#define DIVTAB_DENOM   64
i8 integer_divide_table[DIVTAB_NUMER][DIVTAB_DENOM]; // 2KB
#endif

static __forceinline void draw_line(f32* output, i32 x0, i32 y0, i32 x1, i32 y1, i32 n)
{
  i32 dy = y1 - y0;
  i32 adx = x1 - x0;
  i32 ady = abs(dy);
  i32 base;
  i32 x = x0, y = y0;
  i32 err = 0;
  i32 sy;

  base = dy / adx;
  if (dy < 0)
    sy = base - 1;
  else
    sy = base + 1;
  ady -= abs(base) * adx;
  if (x1 > n) x1 = n;
  if (x < x1) {
    LINE_OP(output[x], inverse_db_table[y & 255]);
    for (++x; x < x1; ++x) {
      err += ady;
      if (err >= adx) {
        err -= adx;
        y += sy;
      }
      else
        y += base;
      LINE_OP(output[x], inverse_db_table[y & 255]);
    }
  }
}

static i32 residue_decode(Ogg::vorbis* f, Codebook* book, f32* target, i32 offset, i32 n, i32 rtype)
{
  i32 k;
  if (rtype == 0) {
    i32 step = n / book->dimensions;
    for (k = 0; k < step; ++k)
      if (!codebook_decode_step(f, book, target + offset + k, n - offset - k, step))
        return FALSE;
  }
  else {
    for (k = 0; k < n; ) {
      if (!codebook_decode(f, book, target + offset, n - k))
        return FALSE;
      k += book->dimensions;
      offset += book->dimensions;
    }
  }
  return TRUE;
}

// n is 1/2 of the blocksize --
// specification: "Correct per-vector decode length is [n]/2"
static void decode_residue(Ogg::vorbis* f, f32* residue_buffers[], i32 ch, i32 n, i32 rn, u8* do_not_decode)
{
  i32 i, j, pass;
  Residue* r = f->residue_config + rn;
  i32 rtype = f->residue_types[rn];
  i32 c = r->classbook;
  i32 classwords = f->codebooks[c].dimensions;
  u32 actual_size = rtype == 2 ? n * 2 : n;
  u32 limit_r_begin = (r->begin < actual_size ? r->begin : actual_size);
  u32 limit_r_end = (r->end < actual_size ? r->end : actual_size);
  i32 n_read = limit_r_end - limit_r_begin;
  i32 part_read = n_read / r->part_size;
  i32 temp_alloc_point = temp_alloc_save(f);
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
  u8*** part_classdata = (u8***)temp_block_array(f, f->channels, part_read * sizeof(**part_classdata));
#else
  i32** classifications = (i32**)temp_block_array(f, f->channels, part_read * sizeof(**classifications));
#endif

  CHECK(f);

  for (i = 0; i < ch; ++i)
    if (!do_not_decode[i])
      memset(residue_buffers[i], 0, sizeof(f32) * n);

  if (rtype == 2 && ch != 1) {
    for (j = 0; j < ch; ++j)
      if (!do_not_decode[j])
        break;
    if (j == ch)
      goto done;

    for (pass = 0; pass < 8; ++pass) {
      i32 pcount = 0, class_set = 0;
      if (ch == 2) {
        while (pcount < part_read) {
          i32 z = r->begin + pcount * r->part_size;
          i32 c_inter = (z & 1), p_inter = z >> 1;
          if (pass == 0) {
            Codebook* c = f->codebooks + r->classbook;
            i32 q;
            DECODE(q, f, c);
            if (q == EOP) goto done;
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
            part_classdata[0][class_set] = r->classdata[q];
#else
            for (i = classwords - 1; i >= 0; --i) {
              classifications[0][i + pcount] = q % r->classifications;
              q /= r->classifications;
            }
#endif
          }
          for (i = 0; i < classwords && pcount < part_read; ++i, ++pcount) {
            i32 z = r->begin + pcount * r->part_size;
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
            i32 c = part_classdata[0][class_set][i];
#else
            i32 c = classifications[0][pcount];
#endif
            i32 b = r->residue_books[c][pass];
            if (b >= 0) {
              Codebook* book = f->codebooks + b;
#ifdef STB_VORBIS_DIVIDES_IN_CODEBOOK
              if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                goto done;
#else
              // saves 1%
              if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                goto done;
#endif
            }
            else {
              z += r->part_size;
              c_inter = z & 1;
              p_inter = z >> 1;
            }
          }
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
          ++class_set;
#endif
        }
      }
      else if (ch > 2) {
        while (pcount < part_read) {
          i32 z = r->begin + pcount * r->part_size;
          i32 c_inter = z % ch, p_inter = z / ch;
          if (pass == 0) {
            Codebook* c = f->codebooks + r->classbook;
            i32 q;
            DECODE(q, f, c);
            if (q == EOP) goto done;
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
            part_classdata[0][class_set] = r->classdata[q];
#else
            for (i = classwords - 1; i >= 0; --i) {
              classifications[0][i + pcount] = q % r->classifications;
              q /= r->classifications;
            }
#endif
          }
          for (i = 0; i < classwords && pcount < part_read; ++i, ++pcount) {
            i32 z = r->begin + pcount * r->part_size;
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
            i32 c = part_classdata[0][class_set][i];
#else
            i32 c = classifications[0][pcount];
#endif
            i32 b = r->residue_books[c][pass];
            if (b >= 0) {
              Codebook* book = f->codebooks + b;
              if (!codebook_decode_deinterleave_repeat(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size))
                goto done;
            }
            else {
              z += r->part_size;
              c_inter = z % ch;
              p_inter = z / ch;
            }
          }
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
          ++class_set;
#endif
        }
      }
    }
    goto done;
  }
  CHECK(f);

  for (pass = 0; pass < 8; ++pass) {
    i32 pcount = 0, class_set = 0;
    while (pcount < part_read) {
      if (pass == 0) {
        for (j = 0; j < ch; ++j) {
          if (!do_not_decode[j]) {
            Codebook* c = f->codebooks + r->classbook;
            i32 temp;
            DECODE(temp, f, c);
            if (temp == EOP) goto done;
            part_classdata[j][class_set] = r->classdata[temp];
          }
        }
      }
      for (i = 0; i < classwords && pcount < part_read; ++i, ++pcount) {
        for (j = 0; j < ch; ++j) {
          if (!do_not_decode[j]) {
            i32 c = part_classdata[j][class_set][i];
            i32 b = r->residue_books[c][pass];
            if (b >= 0) {
              f32* target = residue_buffers[j];
              i32 offset = r->begin + pcount * r->part_size;
              i32 n = r->part_size;
              Codebook* book = f->codebooks + b;
              if (!residue_decode(f, book, target, offset, n, rtype))
                goto done;
            }
          }
        }
      }
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
      ++class_set;
#endif
    }
  }
done:
  CHECK(f);
#ifndef STB_VORBIS_DIVIDES_IN_RESIDUE
  temp_free(f, part_classdata);
#else
  temp_free(f, classifications);
#endif
  temp_alloc_restore(f, temp_alloc_point);
}

#ifndef LIBVORBIS_MDCT
#define LIBVORBIS_MDCT 0
#endif

// the following were split out into separate functions while optimizing;
// they could be pushed back up but eh. __forceinline showed no change;
// they're probably already being inlined.
static void imdct_step3_iter0_loop(i32 n, f32* e, i32 i_off, i32 k_off, f32* A)
{
  f32* ee0 = e + i_off;
  f32* ee2 = ee0 + k_off;
  i32 i;

  assert((n & 3) == 0);
  for (i = (n >> 2); i > 0; --i) {
    f32 k00_20, k01_21;
    k00_20 = ee0[0] - ee2[0];
    k01_21 = ee0[-1] - ee2[-1];
    ee0[0] += ee2[0];//ee0[ 0] = ee0[ 0] + ee2[ 0];
    ee0[-1] += ee2[-1];//ee0[-1] = ee0[-1] + ee2[-1];
    ee2[0] = k00_20 * A[0] - k01_21 * A[1];
    ee2[-1] = k01_21 * A[0] + k00_20 * A[1];
    A += 8;

    k00_20 = ee0[-2] - ee2[-2];
    k01_21 = ee0[-3] - ee2[-3];
    ee0[-2] += ee2[-2];//ee0[-2] = ee0[-2] + ee2[-2];
    ee0[-3] += ee2[-3];//ee0[-3] = ee0[-3] + ee2[-3];
    ee2[-2] = k00_20 * A[0] - k01_21 * A[1];
    ee2[-3] = k01_21 * A[0] + k00_20 * A[1];
    A += 8;

    k00_20 = ee0[-4] - ee2[-4];
    k01_21 = ee0[-5] - ee2[-5];
    ee0[-4] += ee2[-4];//ee0[-4] = ee0[-4] + ee2[-4];
    ee0[-5] += ee2[-5];//ee0[-5] = ee0[-5] + ee2[-5];
    ee2[-4] = k00_20 * A[0] - k01_21 * A[1];
    ee2[-5] = k01_21 * A[0] + k00_20 * A[1];
    A += 8;

    k00_20 = ee0[-6] - ee2[-6];
    k01_21 = ee0[-7] - ee2[-7];
    ee0[-6] += ee2[-6];//ee0[-6] = ee0[-6] + ee2[-6];
    ee0[-7] += ee2[-7];//ee0[-7] = ee0[-7] + ee2[-7];
    ee2[-6] = k00_20 * A[0] - k01_21 * A[1];
    ee2[-7] = k01_21 * A[0] + k00_20 * A[1];
    A += 8;
    ee0 -= 8;
    ee2 -= 8;
  }
}

static void imdct_step3_inner_r_loop(i32 lim, f32* e, i32 d0, i32 k_off, f32* A, i32 k1)
{
  i32 i;
  f32 k00_20, k01_21;

  f32* e0 = e + d0;
  f32* e2 = e0 + k_off;

  for (i = lim >> 2; i > 0; --i) {
    k00_20 = e0[-0] - e2[-0];
    k01_21 = e0[-1] - e2[-1];
    e0[-0] += e2[-0];//e0[-0] = e0[-0] + e2[-0];
    e0[-1] += e2[-1];//e0[-1] = e0[-1] + e2[-1];
    e2[-0] = (k00_20)*A[0] - (k01_21)*A[1];
    e2[-1] = (k01_21)*A[0] + (k00_20)*A[1];

    A += k1;

    k00_20 = e0[-2] - e2[-2];
    k01_21 = e0[-3] - e2[-3];
    e0[-2] += e2[-2];//e0[-2] = e0[-2] + e2[-2];
    e0[-3] += e2[-3];//e0[-3] = e0[-3] + e2[-3];
    e2[-2] = (k00_20)*A[0] - (k01_21)*A[1];
    e2[-3] = (k01_21)*A[0] + (k00_20)*A[1];

    A += k1;

    k00_20 = e0[-4] - e2[-4];
    k01_21 = e0[-5] - e2[-5];
    e0[-4] += e2[-4];//e0[-4] = e0[-4] + e2[-4];
    e0[-5] += e2[-5];//e0[-5] = e0[-5] + e2[-5];
    e2[-4] = (k00_20)*A[0] - (k01_21)*A[1];
    e2[-5] = (k01_21)*A[0] + (k00_20)*A[1];

    A += k1;

    k00_20 = e0[-6] - e2[-6];
    k01_21 = e0[-7] - e2[-7];
    e0[-6] += e2[-6];//e0[-6] = e0[-6] + e2[-6];
    e0[-7] += e2[-7];//e0[-7] = e0[-7] + e2[-7];
    e2[-6] = (k00_20)*A[0] - (k01_21)*A[1];
    e2[-7] = (k01_21)*A[0] + (k00_20)*A[1];

    e0 -= 8;
    e2 -= 8;

    A += k1;
  }
}

static void imdct_step3_inner_s_loop(i32 n, f32* e, i32 i_off, i32 k_off, f32* A, i32 a_off, i32 k0)
{
  i32 i;
  f32 A0 = A[0];
  f32 A1 = A[0 + 1];
  f32 A2 = A[0 + a_off];
  f32 A3 = A[0 + a_off + 1];
  f32 A4 = A[0 + a_off * 2 + 0];
  f32 A5 = A[0 + a_off * 2 + 1];
  f32 A6 = A[0 + a_off * 3 + 0];
  f32 A7 = A[0 + a_off * 3 + 1];

  f32 k00, k11;

  f32* ee0 = e + i_off;
  f32* ee2 = ee0 + k_off;

  for (i = n; i > 0; --i) {
    k00 = ee0[0] - ee2[0];
    k11 = ee0[-1] - ee2[-1];
    ee0[0] = ee0[0] + ee2[0];
    ee0[-1] = ee0[-1] + ee2[-1];
    ee2[0] = (k00)*A0 - (k11)*A1;
    ee2[-1] = (k11)*A0 + (k00)*A1;

    k00 = ee0[-2] - ee2[-2];
    k11 = ee0[-3] - ee2[-3];
    ee0[-2] = ee0[-2] + ee2[-2];
    ee0[-3] = ee0[-3] + ee2[-3];
    ee2[-2] = (k00)*A2 - (k11)*A3;
    ee2[-3] = (k11)*A2 + (k00)*A3;

    k00 = ee0[-4] - ee2[-4];
    k11 = ee0[-5] - ee2[-5];
    ee0[-4] = ee0[-4] + ee2[-4];
    ee0[-5] = ee0[-5] + ee2[-5];
    ee2[-4] = (k00)*A4 - (k11)*A5;
    ee2[-5] = (k11)*A4 + (k00)*A5;

    k00 = ee0[-6] - ee2[-6];
    k11 = ee0[-7] - ee2[-7];
    ee0[-6] = ee0[-6] + ee2[-6];
    ee0[-7] = ee0[-7] + ee2[-7];
    ee2[-6] = (k00)*A6 - (k11)*A7;
    ee2[-7] = (k11)*A6 + (k00)*A7;

    ee0 -= k0;
    ee2 -= k0;
  }
}

static __forceinline void iter_54(f32* z)
{
  f32 k00, k11, k22, k33;
  f32 y0, y1, y2, y3;

  k00 = z[0] - z[-4];
  y0 = z[0] + z[-4];
  y2 = z[-2] + z[-6];
  k22 = z[-2] - z[-6];

  z[-0] = y0 + y2;      // z0 + z4 + z2 + z6
  z[-2] = y0 - y2;      // z0 + z4 - z2 - z6

  // done with y0,y2

  k33 = z[-3] - z[-7];

  z[-4] = k00 + k33;    // z0 - z4 + z3 - z7
  z[-6] = k00 - k33;    // z0 - z4 - z3 + z7

  // done with k33

  k11 = z[-1] - z[-5];
  y1 = z[-1] + z[-5];
  y3 = z[-3] + z[-7];

  z[-1] = y1 + y3;      // z1 + z5 + z3 + z7
  z[-3] = y1 - y3;      // z1 + z5 - z3 - z7
  z[-5] = k11 - k22;    // z1 - z5 + z2 - z6
  z[-7] = k11 + k22;    // z1 - z5 - z2 + z6
}

static void imdct_step3_inner_s_loop_ld654(i32 n, f32* e, i32 i_off, f32* A, i32 base_n)
{
  i32 a_off = base_n >> 3;
  f32 A2 = A[0 + a_off];
  f32* z = e + i_off;
  f32* base = z - 16 * n;

  while (z > base) {
    f32 k00, k11;
    f32 l00, l11;

    k00 = z[-0] - z[-8];
    k11 = z[-1] - z[-9];
    l00 = z[-2] - z[-10];
    l11 = z[-3] - z[-11];
    z[-0] = z[-0] + z[-8];
    z[-1] = z[-1] + z[-9];
    z[-2] = z[-2] + z[-10];
    z[-3] = z[-3] + z[-11];
    z[-8] = k00;
    z[-9] = k11;
    z[-10] = (l00 + l11) * A2;
    z[-11] = (l11 - l00) * A2;

    k00 = z[-4] - z[-12];
    k11 = z[-5] - z[-13];
    l00 = z[-6] - z[-14];
    l11 = z[-7] - z[-15];
    z[-4] = z[-4] + z[-12];
    z[-5] = z[-5] + z[-13];
    z[-6] = z[-6] + z[-14];
    z[-7] = z[-7] + z[-15];
    z[-12] = k11;
    z[-13] = -k00;
    z[-14] = (l11 - l00) * A2;
    z[-15] = (l00 + l11) * -A2;

    iter_54(z);
    iter_54(z - 8);
    z -= 16;
  }
}

static void inverse_mdct(f32* buffer, i32 n, Ogg::vorbis* f, i32 blocktype)
{
  i32 n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l;
  i32 ld;
  // @OPTIMIZE: reduce register pressure by using fewer variables?
  i32 save_point = temp_alloc_save(f);
  f32* buf2 = (f32*)temp_alloc(f, n2 * sizeof(*buf2));
  f32* u = nullptr, * v = nullptr;
  // twiddle factors
  f32* A = f->A[blocktype];

  // IMDCT algorithm from "The use of multirate filter banks for coding of high quality digital audio"
  // See notes about bugs in that paper in less-optimal implementation 'inverse_mdct_old' after this function.

  // kernel from paper


  // merged:
  //   copy and reflect spectral data
  //   step 0

  // note that it turns out that the items added together during
  // this step are, in fact, being added to themselves (as reflected
  // by step 0). inexplicable inefficiency! this became obvious
  // once I combined the passes.

  // so there's a missing 'times 2' here (for adding X to itself).
  // this propagates through linearly to the end, where the numbers
  // are 1/2 too small, and need to be compensated for.

  {
    f32* d, * e, * AA, * e_stop;
    d = &buf2[n2 - 2];
    AA = A;
    e = &buffer[0];
    e_stop = &buffer[n2];
    while (e != e_stop) {
      d[1] = (e[0] * AA[0] - e[2] * AA[1]);
      d[0] = (e[0] * AA[1] + e[2] * AA[0]);
      d -= 2;
      AA += 2;
      e += 4;
    }

    e = &buffer[n2 - 3];
    while (d >= buf2) {
      d[1] = (-e[2] * AA[0] - -e[0] * AA[1]);
      d[0] = (-e[2] * AA[1] + -e[0] * AA[0]);
      d -= 2;
      AA += 2;
      e -= 4;
    }
  }

  // now we use symbolic names for these, so that we can
  // possibly swap their meaning as we change which operations
  // are in place

  u = buffer;
  v = buf2;

  // step 2    (paper output is w, now u)
  // this could be in place, but the data ends up in the wrong
  // place... _somebody_'s got to swap it, so this is nominated
  {
    f32* AA = &A[n2 - 8];
    f32* d0, * d1, * e0, * e1;

    e0 = &v[n4];
    e1 = &v[0];

    d0 = &u[n4];
    d1 = &u[0];

    while (AA >= A) {
      f32 v40_20, v41_21;

      v41_21 = e0[1] - e1[1];
      v40_20 = e0[0] - e1[0];
      d0[1] = e0[1] + e1[1];
      d0[0] = e0[0] + e1[0];
      d1[1] = v41_21 * AA[4] - v40_20 * AA[5];
      d1[0] = v40_20 * AA[4] + v41_21 * AA[5];

      v41_21 = e0[3] - e1[3];
      v40_20 = e0[2] - e1[2];
      d0[3] = e0[3] + e1[3];
      d0[2] = e0[2] + e1[2];
      d1[3] = v41_21 * AA[0] - v40_20 * AA[1];
      d1[2] = v40_20 * AA[0] + v41_21 * AA[1];

      AA -= 8;

      d0 += 4;
      d1 += 4;
      e0 += 4;
      e1 += 4;
    }
  }

  // step 3
  ld = ilog(n) - 1; // ilog is off-by-one from normal definitions

  // optimized step 3:

  // the original step3 loop can be nested r inside s or s inside r;
  // it's written originally as s inside r, but this is dumb when r
  // iterates many times, and s few. So I have two copies of it and
  // switch between them halfway.

  // this is iteration 0 of step 3
  imdct_step3_iter0_loop(n >> 4, u, n2 - 1 - n4 * 0, -(n >> 3), A);
  imdct_step3_iter0_loop(n >> 4, u, n2 - 1 - n4 * 1, -(n >> 3), A);

  // this is iteration 1 of step 3
  imdct_step3_inner_r_loop(n >> 5, u, n2 - 1 - n8 * 0, -(n >> 4), A, 16);
  imdct_step3_inner_r_loop(n >> 5, u, n2 - 1 - n8 * 1, -(n >> 4), A, 16);
  imdct_step3_inner_r_loop(n >> 5, u, n2 - 1 - n8 * 2, -(n >> 4), A, 16);
  imdct_step3_inner_r_loop(n >> 5, u, n2 - 1 - n8 * 3, -(n >> 4), A, 16);

  l = 2;
  for (; l < (ld - 3) >> 1; ++l) {
    i32 k0 = n >> (l + 2), k0_2 = k0 >> 1;
    i32 lim = 1 << (l + 1);
    i32 i;
    for (i = 0; i < lim; ++i)
      imdct_step3_inner_r_loop(n >> (l + 4), u, n2 - 1 - k0 * i, -k0_2, A, 1 << (l + 3));
  }

  for (; l < ld - 6; ++l) {
    i32 k0 = n >> (l + 2), k1 = 1 << (l + 3), k0_2 = k0 >> 1;
    i32 rlim = n >> (l + 6), r;
    i32 lim = 1 << (l + 1);
    i32 i_off;
    f32* A0 = A;
    i_off = n2 - 1;
    for (r = rlim; r > 0; --r) {
      imdct_step3_inner_s_loop(lim, u, i_off, -k0_2, A0, k1, k0);
      A0 += k1 * 4;
      i_off -= 8;
    }
  }

  // iterations with count:
  //   ld-6,-5,-4 all interleaved together
  //       the big win comes from getting rid of needless flops
  //         due to the constants on pass 5 & 4 being all 1 and 0;
  //       combining them to be simultaneous to improve cache made little difference
  imdct_step3_inner_s_loop_ld654(n >> 5, u, n2 - 1, A, n);

  // output is u

  // step 4, 5, and 6
  // cannot be in-place because of step 5
  {
    u16* bitrev = f->bit_reverse[blocktype];
    // weirdly, I'd have thought reading sequentially and writing
    // erratically would have been better than vice-versa, but in
    // fact that's not what my testing showed. (That is, with
    // j = bitreverse(i), do you read i and write j, or read j and write i.)

    f32* d0 = &v[n4 - 4];
    f32* d1 = &v[n2 - 4];
    while (d0 >= v) {
      i32 k4;

      k4 = bitrev[0];
      d1[3] = u[k4 + 0];
      d1[2] = u[k4 + 1];
      d0[3] = u[k4 + 2];
      d0[2] = u[k4 + 3];

      k4 = bitrev[1];
      d1[1] = u[k4 + 0];
      d1[0] = u[k4 + 1];
      d0[1] = u[k4 + 2];
      d0[0] = u[k4 + 3];

      d0 -= 4;
      d1 -= 4;
      bitrev += 2;
    }
  }
  // (paper output is u, now v)


  // data must be in buf2
  assert(v == buf2);

  // step 7   (paper output is v, now v)
  // this is now in place
  {
    f32* C = f->C[blocktype];
    f32* d, * e;

    d = v;
    e = v + n2 - 4;

    while (d < e) {
      f32 a02, a11, b0, b1, b2, b3;

      a02 = d[0] - e[2];
      a11 = d[1] + e[3];

      b0 = C[1] * a02 + C[0] * a11;
      b1 = C[1] * a11 - C[0] * a02;

      b2 = d[0] + e[2];
      b3 = d[1] - e[3];

      d[0] = b2 + b0;
      d[1] = b3 + b1;
      e[2] = b2 - b0;
      e[3] = b1 - b3;

      a02 = d[2] - e[0];
      a11 = d[3] + e[1];

      b0 = C[3] * a02 + C[2] * a11;
      b1 = C[3] * a11 - C[2] * a02;

      b2 = d[2] + e[0];
      b3 = d[3] - e[1];

      d[2] = b2 + b0;
      d[3] = b3 + b1;
      e[0] = b2 - b0;
      e[1] = b1 - b3;

      C += 4;
      d += 4;
      e -= 4;
    }
  }

  // data must be in buf2


  // step 8+decode   (paper output is X, now buffer)
  // this generates pairs of data a la 8 and pushes them directly through
  // the decode kernel (pushing rather than pulling) to avoid having
  // to make another pass later

  // this cannot POSSIBLY be in place, so we refer to the buffers directly

  {
    f32* d0, * d1, * d2, * d3;

    f32* B = f->B[blocktype] + n2 - 8;
    f32* e = buf2 + n2 - 8;
    d0 = &buffer[0];
    d1 = &buffer[n2 - 4];
    d2 = &buffer[n2];
    d3 = &buffer[n - 4];
    while (e >= v) {
      f32 p0, p1, p2, p3;

      p3 = e[6] * B[7] - e[7] * B[6];
      p2 = -e[6] * B[6] - e[7] * B[7];

      d0[0] = p3;
      d1[3] = -p3;
      d2[0] = p2;
      d3[3] = p2;

      p1 = e[4] * B[5] - e[5] * B[4];
      p0 = -e[4] * B[4] - e[5] * B[5];

      d0[1] = p1;
      d1[2] = -p1;
      d2[1] = p0;
      d3[2] = p0;

      p3 = e[2] * B[3] - e[3] * B[2];
      p2 = -e[2] * B[2] - e[3] * B[3];

      d0[2] = p3;
      d1[1] = -p3;
      d2[2] = p2;
      d3[1] = p2;

      p1 = e[0] * B[1] - e[1] * B[0];
      p0 = -e[0] * B[0] - e[1] * B[1];

      d0[3] = p1;
      d1[0] = -p1;
      d2[3] = p0;
      d3[0] = p0;

      B -= 8;
      e -= 8;
      d0 += 4;
      d2 += 4;
      d1 -= 4;
      d3 -= 4;
    }
  }

  temp_free(f, buf2);
  temp_alloc_restore(f, save_point);
}

static f32* get_window(Ogg::vorbis* f, i32 len)
{
  len <<= 1;
  if (len == f->blocksize_0) return f->window[0];
  if (len == f->blocksize_1) return f->window[1];
  return nullptr;
}

typedef i16 YTYPE;
static i32 do_floor(Ogg::vorbis* f, Mapping* map, i32 i, i32 n, f32* target, YTYPE* finalY, u8* step2_flag)
{
  i32 n2 = n >> 1;
  i32 s = map->chan[i].mux, floor;
  floor = map->submap_floor[s];
  if (f->floor_types[floor] == 0) {
    return error(f, VORBIS_invalid_stream);
  }
  else {
    Floor1* g = &f->floor_config[floor].floor1;
    i32 j, q;
    i32 lx = 0, ly = finalY[0] * g->floor1_multiplier;
    for (q = 1; q < g->values; ++q) {
      j = g->sorted_order[q];
#ifndef STB_VORBIS_NO_DEFER_FLOOR
      STBV_NOTUSED(step2_flag);
      if (finalY[j] >= 0)
#else
      if (step2_flag[j])
#endif
      {
        i32 hy = finalY[j] * g->floor1_multiplier;
        i32 hx = g->Xlist[j];
        if (lx != hx)
          draw_line(target, lx, ly, hx, hy, n2);
        CHECK(f);
        lx = hx, ly = hy;
      }
    }
    if (lx < n2) {
      // optimization of: draw_line(target, lx,ly, n,ly, n2);
      for (j = lx; j < n2; ++j)
        LINE_OP(target[j], inverse_db_table[ly]);
      CHECK(f);
    }
  }
  return TRUE;
}

// The meaning of "left" and "right"
//
// For a given frame:
//     we compute samples from 0..n
//     window_center is n/2
//     we'll window and mix the samples from left_start to left_end with data from the previous frame
//     all of the samples from left_end to right_start can be output without mixing; however,
//        this interval is 0-length except when transitioning between short and long frames
//     all of the samples from right_start to right_end need to be mixed with the next frame,
//        which we don't have, so those get saved in a buffer
//     frame N's right_end-right_start, the number of samples to mix with the next frame,
//        has to be the same as frame N+1's left_end-left_start (which they are by
//        construction)

static i32 vorbis_decode_initial(Ogg::vorbis* f, i32* p_left_start, i32* p_left_end, i32* p_right_start, i32* p_right_end, i32* mode)
{
  Mode* m;
  i32 i, n, prev, next, window_center;
  f->channel_buffer_start = f->channel_buffer_end = 0;

retry:
  if (f->eof) return FALSE;
  if (!maybe_start_packet(f))
    return FALSE;
  // check packet type
  if (get_bits(f, 1) != 0) {
    if (IS_PUSH_MODE(f))
      return error(f, VORBIS_bad_packet_type);
    while (EOP != get8_packet(f));
    goto retry;
  }

  if (f->alloc.alloc_buffer)
    assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);

  i = get_bits(f, ilog(f->mode_count - 1));
  if (i == EOP) return FALSE;
  if (i >= f->mode_count) return FALSE;
  *mode = i;
  m = f->mode_config + i;
  if (m->blockflag) {
    n = f->blocksize_1;
    prev = get_bits(f, 1);
    next = get_bits(f, 1);
  }
  else {
    prev = next = 0;
    n = f->blocksize_0;
  }

  // WINDOWING

  window_center = n >> 1;
  if (m->blockflag && !prev) {
    *p_left_start = (n - f->blocksize_0) >> 2;
    *p_left_end = (n + f->blocksize_0) >> 2;
  }
  else {
    *p_left_start = 0;
    *p_left_end = window_center;
  }
  if (m->blockflag && !next) {
    *p_right_start = (n * 3 - f->blocksize_0) >> 2;
    *p_right_end = (n * 3 + f->blocksize_0) >> 2;
  }
  else {
    *p_right_start = window_center;
    *p_right_end = n;
  }

  return TRUE;
}

static i32 vorbis_decode_packet_rest(Ogg::vorbis* f, i32* len, Mode* m, i32 left_start, i32 left_end, i32 right_start, i32 right_end, i32* p_left)
{
  Mapping* map;
  i32 i, j, k, n, n2;
  i32 zero_channel[256];
  i32 really_zero_channel[256];

  // WINDOWING

  STBV_NOTUSED(left_end);
  n = f->blocksize[m->blockflag];
  map = &f->mapping[m->mapping];

  // FLOORS
  n2 = n >> 1;

  CHECK(f);

  for (i = 0; i < f->channels; ++i) {
    i32 s = map->chan[i].mux, floor;
    zero_channel[i] = FALSE;
    floor = map->submap_floor[s];
    if (f->floor_types[floor] == 0) {
      return error(f, VORBIS_invalid_stream);
    }
    else {
      Floor1* g = &f->floor_config[floor].floor1;
      if (get_bits(f, 1)) {
        short* finalY;
        u8 step2_flag[256];
        static i32 range_list[4] = { 256, 128, 86, 64 };
        i32 range = range_list[g->floor1_multiplier - 1];
        i32 offset = 2;
        finalY = f->finalY[i];
        finalY[0] = get_bits(f, ilog(range) - 1);
        finalY[1] = get_bits(f, ilog(range) - 1);
        for (j = 0; j < g->partitions; ++j) {
          i32 pclass = g->partition_class_list[j];
          i32 cdim = g->class_dimensions[pclass];
          i32 cbits = g->class_subclasses[pclass];
          i32 csub = (1 << cbits) - 1;
          i32 cval = 0;
          if (cbits) {
            Codebook* c = f->codebooks + g->class_masterbooks[pclass];
            DECODE(cval, f, c);
          }
          for (k = 0; k < cdim; ++k) {
            i32 book = g->subclass_books[pclass][cval & csub];
            cval = cval >> cbits;
            if (book >= 0) {
              i32 temp;
              Codebook* c = f->codebooks + book;
              DECODE(temp, f, c);
              finalY[offset++] = temp;
            }
            else
              finalY[offset++] = 0;
          }
        }
        if (f->valid_bits == INVALID_BITS) goto error; // behavior according to spec
        step2_flag[0] = step2_flag[1] = 1;
        for (j = 2; j < g->values; ++j) {
          i32 low, high, pred, highroom, lowroom, room, val;
          low = g->neighbors[j][0];
          high = g->neighbors[j][1];
          //neighbors(g->Xlist, j, &low, &high);
          pred = predict_point(g->Xlist[j], g->Xlist[low], g->Xlist[high], finalY[low], finalY[high]);
          val = finalY[j];
          highroom = range - pred;
          lowroom = pred;
          if (highroom < lowroom)
            room = highroom * 2;
          else
            room = lowroom * 2;
          if (val) {
            step2_flag[low] = step2_flag[high] = 1;
            step2_flag[j] = 1;
            if (val >= room)
              if (highroom > lowroom)
                finalY[j] = val - lowroom + pred;
              else
                finalY[j] = pred - val + highroom - 1;
            else
              if (val & 1)
                finalY[j] = pred - ((val + 1) >> 1);
              else
                finalY[j] = pred + (val >> 1);
          }
          else {
            step2_flag[j] = 0;
            finalY[j] = pred;
          }
        }

        // defer final floor computation until _after_ residue
        for (j = 0; j < g->values; ++j) {
          if (!step2_flag[j])
            finalY[j] = -1;
        }

      }
      else {
      error:
        zero_channel[i] = TRUE;
      }
      // So we just defer everything else to later

      // at this point we've decoded the floor into buffer
    }
  }
  CHECK(f);
  // at this point we've decoded all floors

  if (f->alloc.alloc_buffer)
    assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);

  // re-enable coupled channels if necessary
  memcpy(really_zero_channel, zero_channel, sizeof(really_zero_channel[0]) * f->channels);
  for (i = 0; i < map->coupling_steps; ++i)
    if (!zero_channel[map->chan[i].magnitude] || !zero_channel[map->chan[i].angle]) {
      zero_channel[map->chan[i].magnitude] = zero_channel[map->chan[i].angle] = FALSE;
    }

  CHECK(f);
  // RESIDUE DECODE
  for (i = 0; i < map->submaps; ++i) {
    f32* residue_buffers[STB_VORBIS_MAX_CHANNELS];
    i32 r;
    u8 do_not_decode[256];
    i32 ch = 0;
    for (j = 0; j < f->channels; ++j) {
      if (map->chan[j].mux == i) {
        if (zero_channel[j]) {
          do_not_decode[ch] = TRUE;
          residue_buffers[ch] = nullptr;
        }
        else {
          do_not_decode[ch] = FALSE;
          residue_buffers[ch] = f->channel_buffers[j];
        }
        ++ch;
      }
    }
    r = map->submap_residue[i];
    decode_residue(f, residue_buffers, ch, n2, r, do_not_decode);
  }

  if (f->alloc.alloc_buffer)
    assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);
  CHECK(f);

  // INVERSE COUPLING
  for (i = map->coupling_steps - 1; i >= 0; --i) {
    i32 n2 = n >> 1;
    f32* m = f->channel_buffers[map->chan[i].magnitude];
    f32* a = f->channel_buffers[map->chan[i].angle];
    for (j = 0; j < n2; ++j) {
      f32 a2, m2;
      if (m[j] > 0)
        if (a[j] > 0)
          m2 = m[j], a2 = m[j] - a[j];
        else
          a2 = m[j], m2 = m[j] + a[j];
      else
        if (a[j] > 0)
          m2 = m[j], a2 = m[j] + a[j];
        else
          a2 = m[j], m2 = m[j] - a[j];
      m[j] = m2;
      a[j] = a2;
    }
  }
  CHECK(f);

  // finish decoding the floors
  for (i = 0; i < f->channels; ++i) {
    if (really_zero_channel[i]) {
      memset(f->channel_buffers[i], 0, sizeof(*f->channel_buffers[i]) * n2);
    }
    else {
      do_floor(f, map, i, n, f->channel_buffers[i], f->finalY[i], nullptr);
    }
  }

  // INVERSE MDCT
  CHECK(f);
  for (i = 0; i < f->channels; ++i)
    inverse_mdct(f->channel_buffers[i], n, f, m->blockflag);
  CHECK(f);

  // this shouldn't be necessary, unless we exited on an error
  // and want to flush to get to the next packet
  flush_packet(f);

  if (f->first_decode) {
    // assume we start so first non-discarded sample is sample 0
    // this isn't to spec, but spec would require us to read ahead
    // and decode the size of all current frames--could be done,
    // but presumably it's not a commonly used feature
    f->current_loc = 0u - n2; // start of first frame is positioned for discard (NB this is an intentional u32 overflow/wrap-around)
    // we might have to discard samples "from" the next frame too,
    // if we're lapping a large block then a small at the start?
    f->discard_samples_deferred = n - right_end;
    f->current_loc_valid = TRUE;
    f->first_decode = FALSE;
  }
  else if (f->discard_samples_deferred) {
    if (f->discard_samples_deferred >= right_start - left_start) {
      f->discard_samples_deferred -= (right_start - left_start);
      left_start = right_start;
      *p_left = left_start;
    }
    else {
      left_start += f->discard_samples_deferred;
      *p_left = left_start;
      f->discard_samples_deferred = 0;
    }
  }
  else if (f->previous_length == 0 && f->current_loc_valid) {
    // we're recovering from a seek... that means we're going to discard
    // the samples from this packet even though we know our position from
    // the last page header, so we need to update the position based on
    // the discarded samples here
    // but wait, the code below is going to add this in itself even
    // on a discard, so we don't need to do it here...
  }

  // check if we have ogg information about the sample # for this packet
  if (f->last_seg_which == f->end_seg_with_known_loc) {
    // if we have a valid current loc, and this is final:
    if (f->current_loc_valid && (f->page_flag & PAGEFLAG_last_page)) {
      u32 current_end = f->known_loc_for_packet;
      // then let's infer the size of the (probably) short final frame
      if (current_end < f->current_loc + (right_end - left_start)) {
        if (current_end < f->current_loc) {
          // negative truncation, that's impossible!
          *len = 0;
        }
        else {
          *len = current_end - f->current_loc;
        }
        *len += left_start; // this doesn't seem right, but has no ill effect on my test files
        if (*len > right_end) *len = right_end; // this should never happen
        f->current_loc += *len;
        return TRUE;
      }
    }
    // otherwise, just set our sample loc
    // guess that the ogg granule pos refers to the _middle_ of the
    // last frame?
    // set f->current_loc to the position of left_start
    f->current_loc = f->known_loc_for_packet - (n2 - left_start);
    f->current_loc_valid = TRUE;
  }
  if (f->current_loc_valid)
    f->current_loc += (right_start - left_start);

  if (f->alloc.alloc_buffer)
    assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset);
  *len = right_end;  // ignore samples after the window goes to 0
  CHECK(f);

  return TRUE;
}

static i32 vorbis_decode_packet(Ogg::vorbis* f, i32* len, i32* p_left, i32* p_right)
{
  i32 mode, left_end, right_end;
  if (!vorbis_decode_initial(f, p_left, &left_end, p_right, &right_end, &mode)) return 0;
  return vorbis_decode_packet_rest(f, len, f->mode_config + mode, *p_left, left_end, *p_right, right_end, p_left);
}

static i32 vorbis_finish_frame(Ogg::vorbis* f, i32 len, i32 left, i32 right)
{
  i32 prev, i, j;
  // we use right&left (the start of the right- and left-window sin()-regions)
  // to determine how much to return, rather than inferring from the rules
  // (same result, clearer code); 'left' indicates where our sin() window
  // starts, therefore where the previous window's right edge starts, and
  // therefore where to start mixing from the previous buffer. 'right'
  // indicates where our sin() ending-window starts, therefore that's where
  // we start saving, and where our returned-data ends.

  // mixin from previous window
  if (f->previous_length) {
    i32 i, j, n = f->previous_length;
    f32* w = get_window(f, n);
    if (w == nullptr) return 0;
    for (i = 0; i < f->channels; ++i) {
      for (j = 0; j < n; ++j)
        f->channel_buffers[i][left + j] =
        f->channel_buffers[i][left + j] * w[j] +
        f->previous_window[i][j] * w[n - 1 - j];
    }
  }

  prev = f->previous_length;

  // last half of this data becomes previous window
  f->previous_length = len - right;

  // @OPTIMIZE: could avoid this copy by double-buffering the
  // output (flipping previous_window with channel_buffers), but
  // then previous_window would have to be 2x as large, and
  // channel_buffers couldn't be temp mem (although they're NOT
  // currently temp mem, they could be (unless we want to level
  // performance by spreading out the computation))
  for (i = 0; i < f->channels; ++i)
    for (j = 0; right + j < len; ++j)
      f->previous_window[i][j] = f->channel_buffers[i][right + j];

  if (!prev)
    // there was no previous packet, so this data isn't valid...
    // this isn't entirely true, only the would-have-overlapped data
    // isn't valid, but this seems to be what the spec requires
    return 0;

  // truncate a short frame
  if (len < right) right = len;

  f->samples_output += right - left;

  return right - left;
}

static i32 vorbis_pump_first_frame(Ogg::vorbis* f)
{
  i32 len, right, left, res;
  res = vorbis_decode_packet(f, &len, &left, &right);
  if (res)
    vorbis_finish_frame(f, len, left, right);
  return res;
}

static i32 start_decoder(Ogg::vorbis* f)
{
  u8 header[6], x, y;
  i32 len, i, j, k, max_submaps = 0;
  i32 longest_floorlist = 0;

  // first page, first packet
  f->first_decode = TRUE;

  if (!start_page(f))                              return FALSE;
  // validate page flag
  if (!(f->page_flag & PAGEFLAG_first_page))       return error(f, VORBIS_invalid_first_page);
  if (f->page_flag & PAGEFLAG_last_page)           return error(f, VORBIS_invalid_first_page);
  if (f->page_flag & PAGEFLAG_continued_packet)    return error(f, VORBIS_invalid_first_page);
  // check for expected packet length
  if (f->segment_count != 1)                       return error(f, VORBIS_invalid_first_page);
  if (f->segments[0] != 30) {
    // check for the Ogg skeleton fishead identifying header to refine our error
    if (f->segments[0] == 64 &&
      getn(f, header, 6) &&
      header[0] == 'f' &&
      header[1] == 'i' &&
      header[2] == 's' &&
      header[3] == 'h' &&
      header[4] == 'e' &&
      header[5] == 'a' &&
      get8(f) == 'd' &&
      get8(f) == '\0')                        return error(f, VORBIS_ogg_skeleton_not_supported);
    else
      return error(f, VORBIS_invalid_first_page);
  }

  // read packet
  // check packet header
  if (get8(f) != VORBIS_packet_id)                 return error(f, VORBIS_invalid_first_page);
  if (!getn(f, header, 6))                         return error(f, VORBIS_unexpected_eof);
  if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_first_page);
  // vorbis_version
  if (get32(f) != 0)                               return error(f, VORBIS_invalid_first_page);
  f->channels = get8(f); if (!f->channels)         return error(f, VORBIS_invalid_first_page);
  if (f->channels > STB_VORBIS_MAX_CHANNELS)       return error(f, VORBIS_too_many_channels);
  f->sample_rate = get32(f); if (!f->sample_rate)  return error(f, VORBIS_invalid_first_page);
  get32(f); // bitrate_maximum
  get32(f); // bitrate_nominal
  get32(f); // bitrate_minimum
  x = get8(f);
  {
    i32 log0, log1;
    log0 = x & 15;
    log1 = x >> 4;
    f->blocksize_0 = 1 << log0;
    f->blocksize_1 = 1 << log1;
    if (log0 < 6 || log0 > 13)                       return error(f, VORBIS_invalid_setup);
    if (log1 < 6 || log1 > 13)                       return error(f, VORBIS_invalid_setup);
    if (log0 > log1)                                 return error(f, VORBIS_invalid_setup);
  }

  // framing_flag
  x = get8(f);
  if (!(x & 1))                                    return error(f, VORBIS_invalid_first_page);

  // second packet!
  if (!start_page(f))                              return FALSE;

  if (!start_packet(f))                            return FALSE;

  if (!next_segment(f))                            return FALSE;

  if (get8_packet(f) != VORBIS_packet_comment)            return error(f, VORBIS_invalid_setup);
  for (i = 0; i < 6; ++i) header[i] = get8_packet(f);
  if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_setup);
  //file vendor
  len = get32_packet(f);
  f->vendor = (char*)setup_malloc(f, sizeof(char) * (len + 1));
  if (f->vendor == nullptr)                           return error(f, VORBIS_outofmem);
  for (i = 0; i < len; ++i) {
    f->vendor[i] = get8_packet(f);
  }
  f->vendor[len] = (char)'\0';
  //user comments
  f->comment_list_length = get32_packet(f);
  f->comment_list = nullptr;
  if (f->comment_list_length > 0)
  {
    f->comment_list = (char**)setup_malloc(f, sizeof(char*) * (f->comment_list_length));
    if (f->comment_list == nullptr)                  return error(f, VORBIS_outofmem);
  }

  for (i = 0; i < f->comment_list_length; ++i) {
    len = get32_packet(f);
    f->comment_list[i] = (char*)setup_malloc(f, sizeof(char) * (len + 1));
    if (f->comment_list[i] == nullptr)               return error(f, VORBIS_outofmem);

    for (j = 0; j < len; ++j) {
      f->comment_list[i][j] = get8_packet(f);
    }
    f->comment_list[i][len] = (char)'\0';
  }

  // framing_flag
  x = get8_packet(f);
  if (!(x & 1))                                    return error(f, VORBIS_invalid_setup);


  skip(f, f->bytes_in_seg);
  f->bytes_in_seg = 0;

  do {
    len = next_segment(f);
    skip(f, len);
    f->bytes_in_seg = 0;
  } while (len);

  // third packet!
  if (!start_packet(f))                            return FALSE;

  crc32_init(); // always init it, to avoid multithread race conditions

  if (get8_packet(f) != VORBIS_packet_setup)       return error(f, VORBIS_invalid_setup);
  for (i = 0; i < 6; ++i) header[i] = get8_packet(f);
  if (!vorbis_validate(header))                    return error(f, VORBIS_invalid_setup);

  // codebooks

  f->codebook_count = get_bits(f, 8) + 1;
  f->codebooks = (Codebook*)setup_malloc(f, sizeof(*f->codebooks) * f->codebook_count);
  if (f->codebooks == nullptr)                        return error(f, VORBIS_outofmem);
  memset(f->codebooks, 0, sizeof(*f->codebooks) * f->codebook_count);
  for (i = 0; i < f->codebook_count; ++i) {
    u32* values;
    i32 ordered, sorted_count;
    i32 total = 0;
    u8* lengths;
    Codebook* c = f->codebooks + i;
    CHECK(f);
    x = get_bits(f, 8); if (x != 0x42)            return error(f, VORBIS_invalid_setup);
    x = get_bits(f, 8); if (x != 0x43)            return error(f, VORBIS_invalid_setup);
    x = get_bits(f, 8); if (x != 0x56)            return error(f, VORBIS_invalid_setup);
    x = get_bits(f, 8);
    c->dimensions = (get_bits(f, 8) << 8) + x;
    x = get_bits(f, 8);
    y = get_bits(f, 8);
    c->entries = (get_bits(f, 8) << 16) + (y << 8) + x;
    ordered = get_bits(f, 1);
    c->sparse = ordered ? 0 : get_bits(f, 1);

    if (c->dimensions == 0 && c->entries != 0)    return error(f, VORBIS_invalid_setup);

    if (c->sparse)
      lengths = (u8*)setup_temp_malloc(f, c->entries);
    else
      lengths = c->codeword_lengths = (u8*)setup_malloc(f, c->entries);

    if (!lengths) return error(f, VORBIS_outofmem);

    if (ordered) {
      i32 current_entry = 0;
      i32 current_length = get_bits(f, 5) + 1;
      while (current_entry < c->entries) {
        i32 limit = c->entries - current_entry;
        i32 n = get_bits(f, ilog(limit));
        if (current_length >= 32) return error(f, VORBIS_invalid_setup);
        if (current_entry + n > (i32) c->entries) { return error(f, VORBIS_invalid_setup); }
        memset(lengths + current_entry, current_length, n);
        current_entry += n;
        ++current_length;
      }
    }
    else {
      for (j = 0; j < c->entries; ++j) {
        i32 present = c->sparse ? get_bits(f, 1) : 1;
        if (present) {
          lengths[j] = get_bits(f, 5) + 1;
          ++total;
          if (lengths[j] == 32)
            return error(f, VORBIS_invalid_setup);
        }
        else {
          lengths[j] = NO_CODE;
        }
      }
    }

    if (c->sparse && total >= c->entries >> 2) {
      // convert sparse items to non-sparse!
      if (c->entries > (i32)f->setup_temp_memory_required)
        f->setup_temp_memory_required = c->entries;

      c->codeword_lengths = (u8*)setup_malloc(f, c->entries);
      if (c->codeword_lengths == nullptr) return error(f, VORBIS_outofmem);
      memcpy(c->codeword_lengths, lengths, c->entries);
      setup_temp_free(f, lengths, c->entries); // note this is only safe if there have been no intervening temp mallocs!
      lengths = c->codeword_lengths;
      c->sparse = 0;
    }

    // compute the size of the sorted tables
    if (c->sparse) {
      sorted_count = total;
    }
    else {
      sorted_count = 0;
#ifndef STB_VORBIS_NO_HUFFMAN_BINARY_SEARCH
      for (j = 0; j < c->entries; ++j)
        if (lengths[j] > STB_VORBIS_FAST_HUFFMAN_LENGTH && lengths[j] != NO_CODE)
          ++sorted_count;
#endif
    }

    c->sorted_entries = sorted_count;
    values = nullptr;

    CHECK(f);
    if (!c->sparse) {
      c->codewords = (u32*)setup_malloc(f, sizeof(c->codewords[0]) * c->entries);
      if (!c->codewords)                  return error(f, VORBIS_outofmem);
    }
    else {
      u32 size;
      if (c->sorted_entries) {
        c->codeword_lengths = (u8*)setup_malloc(f, c->sorted_entries);
        if (!c->codeword_lengths)           return error(f, VORBIS_outofmem);
        c->codewords = (u32*)setup_temp_malloc(f, sizeof(*c->codewords) * c->sorted_entries);
        if (!c->codewords)                  return error(f, VORBIS_outofmem);
        values = (u32*)setup_temp_malloc(f, sizeof(*values) * c->sorted_entries);
        if (!values)                        return error(f, VORBIS_outofmem);
      }
      size = c->entries + (sizeof(*c->codewords) + sizeof(*values)) * c->sorted_entries;
      if (size > f->setup_temp_memory_required)
        f->setup_temp_memory_required = size;
    }

    if (!compute_codewords(c, lengths, c->entries, values)) {
      if (c->sparse) setup_temp_free(f, values, 0);
      return error(f, VORBIS_invalid_setup);
    }

    if (c->sorted_entries) {
      // allocate an extra slot for sentinels
      c->sorted_codewords = (u32*)setup_malloc(f, sizeof(*c->sorted_codewords) * (c->sorted_entries + 1));
      if (c->sorted_codewords == nullptr) return error(f, VORBIS_outofmem);
      // allocate an extra slot at the front so that c->sorted_values[-1] is defined
      // so that we can catch that case without an extra if
      c->sorted_values = (i32*)setup_malloc(f, sizeof(*c->sorted_values) * (c->sorted_entries + 1));
      if (c->sorted_values == nullptr) return error(f, VORBIS_outofmem);
      ++c->sorted_values;
      c->sorted_values[-1] = -1;
      compute_sorted_huffman(c, lengths, values);
    }

    if (c->sparse) {
      setup_temp_free(f, values, sizeof(*values) * c->sorted_entries);
      setup_temp_free(f, c->codewords, sizeof(*c->codewords) * c->sorted_entries);
      setup_temp_free(f, lengths, c->entries);
      c->codewords = nullptr;
    }

    compute_accelerated_huffman(c);

    CHECK(f);
    c->lookup_type = get_bits(f, 4);
    if (c->lookup_type > 2) return error(f, VORBIS_invalid_setup);
    if (c->lookup_type > 0) {
      u16* mults;
      c->minimum_value = float32_unpack(get_bits(f, 32));
      c->delta_value = float32_unpack(get_bits(f, 32));
      c->value_bits = get_bits(f, 4) + 1;
      c->sequence_p = get_bits(f, 1);
      if (c->lookup_type == 1) {
        i32 values = lookup1_values(c->entries, c->dimensions);
        if (values < 0) return error(f, VORBIS_invalid_setup);
        c->lookup_values = (u32)values;
      }
      else {
        c->lookup_values = c->entries * c->dimensions;
      }
      if (c->lookup_values == 0) return error(f, VORBIS_invalid_setup);
      mults = (u16*)setup_temp_malloc(f, sizeof(mults[0]) * c->lookup_values);
      if (mults == nullptr) return error(f, VORBIS_outofmem);
      for (j = 0; j < (i32)c->lookup_values; ++j) {
        i32 q = get_bits(f, c->value_bits);
        if (q == EOP) { setup_temp_free(f, mults, sizeof(mults[0]) * c->lookup_values); return error(f, VORBIS_invalid_setup); }
        mults[j] = q;
      }

      if (c->lookup_type == 1) {
        i32 len, sparse = c->sparse;
        f32 last = 0;
        // pre-expand the lookup1-style multiplicands, to avoid a divide in the inner loop
        if (sparse) {
          if (c->sorted_entries == 0) goto skip;
          c->multiplicands = (codetype*)setup_malloc(f, sizeof(c->multiplicands[0]) * c->sorted_entries * c->dimensions);
        }
        else
          c->multiplicands = (codetype*)setup_malloc(f, sizeof(c->multiplicands[0]) * c->entries * c->dimensions);
        if (c->multiplicands == nullptr) { setup_temp_free(f, mults, sizeof(mults[0]) * c->lookup_values); return error(f, VORBIS_outofmem); }
        len = sparse ? c->sorted_entries : c->entries;
        for (j = 0; j < len; ++j) {
          u32 z = sparse ? c->sorted_values[j] : j;
          u32 div = 1;
          for (k = 0; k < c->dimensions; ++k) {
            i32 off = (z / div) % c->lookup_values;
            f32 val = mults[off] * c->delta_value + c->minimum_value + last;
            c->multiplicands[j * c->dimensions + k] = val;
            if (c->sequence_p)
              last = val;
            if (k + 1 < c->dimensions) {
              if (div > UINT_MAX / (u32)c->lookup_values) {
                setup_temp_free(f, mults, sizeof(mults[0]) * c->lookup_values);
                return error(f, VORBIS_invalid_setup);
              }
              div *= c->lookup_values;
            }
          }
        }
        c->lookup_type = 2;
      }
      else
      {
        f32 last = 0;
        CHECK(f);
        c->multiplicands = (codetype*)setup_malloc(f, sizeof(c->multiplicands[0]) * c->lookup_values);
        if (c->multiplicands == nullptr) { setup_temp_free(f, mults, sizeof(mults[0]) * c->lookup_values); return error(f, VORBIS_outofmem); }
        for (j = 0; j < (i32)c->lookup_values; ++j) {
          f32 val = mults[j] * c->delta_value + c->minimum_value + last;
          c->multiplicands[j] = val;
          if (c->sequence_p)
            last = val;
        }
      }
#ifndef STB_VORBIS_DIVIDES_IN_CODEBOOK
      skip : ;
#endif
      setup_temp_free(f, mults, sizeof(mults[0]) * c->lookup_values);

      CHECK(f);
    }
    CHECK(f);
  }

  // time domain transfers (notused)

  x = get_bits(f, 6) + 1;
  for (i = 0; i < x; ++i) {
    u32 z = get_bits(f, 16);
    if (z != 0) return error(f, VORBIS_invalid_setup);
  }

  // Floors
  f->floor_count = get_bits(f, 6) + 1;
  f->floor_config = (Floor*)setup_malloc(f, f->floor_count * sizeof(*f->floor_config));
  if (f->floor_config == nullptr) return error(f, VORBIS_outofmem);
  for (i = 0; i < f->floor_count; ++i) {
    f->floor_types[i] = get_bits(f, 16);
    if (f->floor_types[i] > 1) return error(f, VORBIS_invalid_setup);
    if (f->floor_types[i] == 0) {
      Floor0* g = &f->floor_config[i].floor0;
      g->order = get_bits(f, 8);
      g->rate = get_bits(f, 16);
      g->bark_map_size = get_bits(f, 16);
      g->amplitude_bits = get_bits(f, 6);
      g->amplitude_offset = get_bits(f, 8);
      g->number_of_books = get_bits(f, 4) + 1;
      for (j = 0; j < g->number_of_books; ++j)
        g->book_list[j] = get_bits(f, 8);
      return error(f, VORBIS_feature_not_supported);
    }
    else {
      stbv__floor_ordering p[31 * 8 + 2];
      Floor1* g = &f->floor_config[i].floor1;
      i32 max_class = -1;
      g->partitions = get_bits(f, 5);
      for (j = 0; j < g->partitions; ++j) {
        g->partition_class_list[j] = get_bits(f, 4);
        if (g->partition_class_list[j] > max_class)
          max_class = g->partition_class_list[j];
      }
      for (j = 0; j <= max_class; ++j) {
        g->class_dimensions[j] = get_bits(f, 3) + 1;
        g->class_subclasses[j] = get_bits(f, 2);
        if (g->class_subclasses[j]) {
          g->class_masterbooks[j] = get_bits(f, 8);
          if (g->class_masterbooks[j] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
        }
        for (k = 0; k < 1 << g->class_subclasses[j]; ++k) {
          g->subclass_books[j][k] = (i16)get_bits(f, 8) - 1;
          if (g->subclass_books[j][k] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
        }
      }
      g->floor1_multiplier = get_bits(f, 2) + 1;
      g->rangebits = get_bits(f, 4);
      g->Xlist[0] = 0;
      g->Xlist[1] = 1 << g->rangebits;
      g->values = 2;
      for (j = 0; j < g->partitions; ++j) {
        i32 c = g->partition_class_list[j];
        for (k = 0; k < g->class_dimensions[c]; ++k) {
          g->Xlist[g->values] = get_bits(f, g->rangebits);
          ++g->values;
        }
      }
      // precompute the sorting
      for (j = 0; j < g->values; ++j) {
        p[j].x = g->Xlist[j];
        p[j].id = j;
      }
      qsort(p, g->values, sizeof(p[0]), point_compare);
      for (j = 0; j < g->values - 1; ++j)
        if (p[j].x == p[j + 1].x)
          return error(f, VORBIS_invalid_setup);
      for (j = 0; j < g->values; ++j)
        g->sorted_order[j] = (u8)p[j].id;
      // precompute the neighbors
      for (j = 2; j < g->values; ++j) {
        i32 low = 0, hi = 0;
        neighbors(g->Xlist, j, &low, &hi);
        g->neighbors[j][0] = low;
        g->neighbors[j][1] = hi;
      }

      if (g->values > longest_floorlist)
        longest_floorlist = g->values;
    }
  }

  // Residue
  f->residue_count = get_bits(f, 6) + 1;
  f->residue_config = (Residue*)setup_malloc(f, f->residue_count * sizeof(f->residue_config[0]));
  if (f->residue_config == nullptr) return error(f, VORBIS_outofmem);
  memset(f->residue_config, 0, f->residue_count * sizeof(f->residue_config[0]));
  for (i = 0; i < f->residue_count; ++i) {
    u8 residue_cascade[64];
    Residue* r = f->residue_config + i;
    f->residue_types[i] = get_bits(f, 16);
    if (f->residue_types[i] > 2) return error(f, VORBIS_invalid_setup);
    r->begin = get_bits(f, 24);
    r->end = get_bits(f, 24);
    if (r->end < r->begin) return error(f, VORBIS_invalid_setup);
    r->part_size = get_bits(f, 24) + 1;
    r->classifications = get_bits(f, 6) + 1;
    r->classbook = get_bits(f, 8);
    if (r->classbook >= f->codebook_count) return error(f, VORBIS_invalid_setup);
    for (j = 0; j < r->classifications; ++j) {
      u8 high_bits = 0;
      u8 low_bits = get_bits(f, 3);
      if (get_bits(f, 1))
        high_bits = get_bits(f, 5);
      residue_cascade[j] = high_bits * 8 + low_bits;
    }
    r->residue_books = (short(*)[8]) setup_malloc(f, sizeof(r->residue_books[0]) * r->classifications);
    if (r->residue_books == nullptr) return error(f, VORBIS_outofmem);
    for (j = 0; j < r->classifications; ++j) {
      for (k = 0; k < 8; ++k) {
        if (residue_cascade[j] & (1 << k)) {
          r->residue_books[j][k] = get_bits(f, 8);
          if (r->residue_books[j][k] >= f->codebook_count) return error(f, VORBIS_invalid_setup);
        }
        else {
          r->residue_books[j][k] = -1;
        }
      }
    }
    // precompute the classifications[] array to avoid inner-loop mod/divide
    // call it 'classdata' since we already have r->classifications
    r->classdata = (u8**)setup_malloc(f, sizeof(*r->classdata) * f->codebooks[r->classbook].entries);
    if (!r->classdata) return error(f, VORBIS_outofmem);
    memset(r->classdata, 0, sizeof(*r->classdata) * f->codebooks[r->classbook].entries);
    for (j = 0; j < f->codebooks[r->classbook].entries; ++j) {
      i32 classwords = f->codebooks[r->classbook].dimensions;
      i32 temp = j;
      r->classdata[j] = (u8*)setup_malloc(f, sizeof(r->classdata[j][0]) * classwords);
      if (r->classdata[j] == nullptr) return error(f, VORBIS_outofmem);
      for (k = classwords - 1; k >= 0; --k) {
        r->classdata[j][k] = temp % r->classifications;
        temp /= r->classifications;
      }
    }
  }

  f->mapping_count = get_bits(f, 6) + 1;
  f->mapping = (Mapping*)setup_malloc(f, f->mapping_count * sizeof(*f->mapping));
  if (f->mapping == nullptr) return error(f, VORBIS_outofmem);
  memset(f->mapping, 0, f->mapping_count * sizeof(*f->mapping));
  for (i = 0; i < f->mapping_count; ++i) {
    Mapping* m = f->mapping + i;
    i32 mapping_type = get_bits(f, 16);
    if (mapping_type != 0) return error(f, VORBIS_invalid_setup);
    m->chan = (MappingChannel*)setup_malloc(f, f->channels * sizeof(*m->chan));
    if (m->chan == nullptr) return error(f, VORBIS_outofmem);
    if (get_bits(f, 1))
      m->submaps = get_bits(f, 4) + 1;
    else
      m->submaps = 1;
    if (m->submaps > max_submaps)
      max_submaps = m->submaps;
    if (get_bits(f, 1)) {
      m->coupling_steps = get_bits(f, 8) + 1;
      if (m->coupling_steps > f->channels) return error(f, VORBIS_invalid_setup);
      for (k = 0; k < m->coupling_steps; ++k) {
        m->chan[k].magnitude = get_bits(f, ilog(f->channels - 1));
        m->chan[k].angle = get_bits(f, ilog(f->channels - 1));
        if (m->chan[k].magnitude >= f->channels)        return error(f, VORBIS_invalid_setup);
        if (m->chan[k].angle >= f->channels)        return error(f, VORBIS_invalid_setup);
        if (m->chan[k].magnitude == m->chan[k].angle)   return error(f, VORBIS_invalid_setup);
      }
    }
    else
      m->coupling_steps = 0;

    // reserved field
    if (get_bits(f, 2)) return error(f, VORBIS_invalid_setup);
    if (m->submaps > 1) {
      for (j = 0; j < f->channels; ++j) {
        m->chan[j].mux = get_bits(f, 4);
        if (m->chan[j].mux >= m->submaps)                return error(f, VORBIS_invalid_setup);
      }
    }
    else
      // @SPECIFICATION: this case is missing from the spec
      for (j = 0; j < f->channels; ++j)
        m->chan[j].mux = 0;

    for (j = 0; j < m->submaps; ++j) {
      get_bits(f, 8); // discard
      m->submap_floor[j] = get_bits(f, 8);
      m->submap_residue[j] = get_bits(f, 8);
      if (m->submap_floor[j] >= f->floor_count)      return error(f, VORBIS_invalid_setup);
      if (m->submap_residue[j] >= f->residue_count)  return error(f, VORBIS_invalid_setup);
    }
  }

  // Modes
  f->mode_count = get_bits(f, 6) + 1;
  for (i = 0; i < f->mode_count; ++i) {
    Mode* m = f->mode_config + i;
    m->blockflag = get_bits(f, 1);
    m->windowtype = get_bits(f, 16);
    m->transformtype = get_bits(f, 16);
    m->mapping = get_bits(f, 8);
    if (m->windowtype != 0)                 return error(f, VORBIS_invalid_setup);
    if (m->transformtype != 0)              return error(f, VORBIS_invalid_setup);
    if (m->mapping >= f->mapping_count)     return error(f, VORBIS_invalid_setup);
  }

  flush_packet(f);

  f->previous_length = 0;

  for (i = 0; i < f->channels; ++i) {
    f->channel_buffers[i] = (f32*)setup_malloc(f, sizeof(f32) * f->blocksize_1);
    f->previous_window[i] = (f32*)setup_malloc(f, sizeof(f32) * f->blocksize_1 / 2);
    f->finalY[i] = (i16*)setup_malloc(f, sizeof(i16) * longest_floorlist);
    if (f->channel_buffers[i] == nullptr || f->previous_window[i] == nullptr || f->finalY[i] == nullptr) return error(f, VORBIS_outofmem);
    memset(f->channel_buffers[i], 0, sizeof(f32) * f->blocksize_1);
#ifdef STB_VORBIS_NO_DEFER_FLOOR
    f->floor_buffers[i] = (f32*)setup_malloc(f, sizeof(f32) * f->blocksize_1 / 2);
    if (f->floor_buffers[i] == nullptr) return error(f, VORBIS_outofmem);
#endif
  }

  if (!init_blocksize(f, 0, f->blocksize_0)) return FALSE;
  if (!init_blocksize(f, 1, f->blocksize_1)) return FALSE;
  f->blocksize[0] = f->blocksize_0;
  f->blocksize[1] = f->blocksize_1;

  // compute how much temporary memory is needed

  // 1.
  {
    u32 imdct_mem = (f->blocksize_1 * sizeof(f32) >> 1);
    u32 classify_mem;
    i32 i, max_part_read = 0;
    for (i = 0; i < f->residue_count; ++i) {
      Residue* r = f->residue_config + i;
      u32 actual_size = f->blocksize_1 / 2;
      u32 limit_r_begin = r->begin < actual_size ? r->begin : actual_size;
      u32 limit_r_end = r->end < actual_size ? r->end : actual_size;
      i32 n_read = limit_r_end - limit_r_begin;
      i32 part_read = n_read / r->part_size;
      if (part_read > max_part_read)
        max_part_read = part_read;
    }
    classify_mem = f->channels * (sizeof(void*) + max_part_read * sizeof(u8*));

    // maximum reasonable partition size is f->blocksize_1

    f->temp_memory_required = classify_mem;
    if (imdct_mem > f->temp_memory_required)
      f->temp_memory_required = imdct_mem;
  }


  if (f->alloc.alloc_buffer) {
    assert(f->temp_offset == f->alloc.alloc_buffer_length_in_bytes);
    // check if there's enough temp memory so we don't error later
    if (f->setup_offset + sizeof(*f) + f->temp_memory_required > (u32)f->temp_offset)
      return error(f, VORBIS_outofmem);
  }

  // @TODO: stb_vorbis_seek_start expects first_audio_page_offset to point to a page
  // without PAGEFLAG_continued_packet, so this either points to the first page, or
  // the page after the end of the headers. It might be cleaner to point to a page
  // in the middle of the headers, when that's the page where the first audio packet
  // starts, but we'd have to also correctly skip the end of any continued packet in
  // stb_vorbis_seek_start.
  if (f->next_seg == -1) {
    f->first_audio_page_offset = stb_vorbis_get_file_offset(f);
  }
  else {
    f->first_audio_page_offset = 0;
  }

  return TRUE;
}

static void vorbis_deinit(Ogg::vorbis* p)
{
  i32 i, j;

  setup_free(p, p->vendor);
  for (i = 0; i < p->comment_list_length; ++i) {
    setup_free(p, p->comment_list[i]);
  }
  setup_free(p, p->comment_list);

  if (p->residue_config) {
    for (i = 0; i < p->residue_count; ++i) {
      Residue* r = p->residue_config + i;
      if (r->classdata) {
        for (j = 0; j < p->codebooks[r->classbook].entries; ++j)
          setup_free(p, r->classdata[j]);
        setup_free(p, r->classdata);
      }
      setup_free(p, r->residue_books);
    }
  }

  if (p->codebooks) {
    CHECK(p);
    for (i = 0; i < p->codebook_count; ++i) {
      Codebook* c = p->codebooks + i;
      setup_free(p, c->codeword_lengths);
      setup_free(p, c->multiplicands);
      setup_free(p, c->codewords);
      setup_free(p, c->sorted_codewords);
      // c->sorted_values[-1] is the first entry in the array
      setup_free(p, c->sorted_values ? c->sorted_values - 1 : nullptr);
    }
    setup_free(p, p->codebooks);
  }
  setup_free(p, p->floor_config);
  setup_free(p, p->residue_config);
  if (p->mapping) {
    for (i = 0; i < p->mapping_count; ++i)
      setup_free(p, p->mapping[i].chan);
    setup_free(p, p->mapping);
  }
  CHECK(p);
  for (i = 0; i < p->channels && i < STB_VORBIS_MAX_CHANNELS; ++i) {
    setup_free(p, p->channel_buffers[i]);
    setup_free(p, p->previous_window[i]);
    setup_free(p, p->finalY[i]);
  }
  for (i = 0; i < 2; ++i) {
    setup_free(p, p->A[i]);
    setup_free(p, p->B[i]);
    setup_free(p, p->C[i]);
    setup_free(p, p->window[i]);
    setup_free(p, p->bit_reverse[i]);
  }
}

void Ogg::close(Ogg::vorbis* p)
{
  if (p == nullptr) return;
  vorbis_deinit(p);
  setup_free(p, p);
}

static void vorbis_init(Ogg::vorbis* p, const stb_vorbis_alloc* z)
{
  memset(p, 0, sizeof(*p)); // nullptr out all malloc'd pointers to start
  if (z) {
    p->alloc = *z;
    p->alloc.alloc_buffer_length_in_bytes &= ~7;
    p->temp_offset = p->alloc.alloc_buffer_length_in_bytes;
  }
  p->eof = 0;
  p->error = VORBIS__no_error;
  p->stream = nullptr;
  p->codebooks = nullptr;
  p->page_crc_tests = -1;
}

i32 stb_vorbis_get_sample_offset(Ogg::vorbis* f)
{
  if (f->current_loc_valid)
    return f->current_loc;
  else
    return -1;
}

Ogg::Info Ogg::getInfo(Ogg::vorbis* f)
{
  Ogg::Info d;
  d.channels = f->channels;
  d.sample_rate = f->sample_rate;
  d.setup_memory_required = f->setup_memory_required;
  d.setup_temp_memory_required = f->setup_temp_memory_required;
  d.temp_memory_required = f->temp_memory_required;
  d.max_frame_size = f->blocksize_1 >> 1;
  return d;
}

i32 stb_vorbis_get_error(Ogg::vorbis* f)
{
  i32 e = f->error;
  f->error = VORBIS__no_error;
  return e;
}

static Ogg::vorbis* vorbis_alloc(Ogg::vorbis* f)
{
  Ogg::vorbis* p = (Ogg::vorbis*)setup_malloc(f, sizeof(*p));
  return p;
}

u32 stb_vorbis_get_file_offset(Ogg::vorbis* f)
{
  if (USE_MEMORY(f)) return (u32)(f->stream - f->stream_start);
}

static u32 vorbis_find_page(Ogg::vorbis* f, u32* end, u32* last)
{
  for (;;) {
    i32 n;
    if (f->eof) return 0;
    n = get8(f);
    if (n == 0x4f) { // page header candidate
      u32 retry_loc = stb_vorbis_get_file_offset(f);
      i32 i;
      // check if we're off the end of a file_section stream
      if (retry_loc - 25 > f->stream_len)
        return 0;
      // check the rest of the header
      for (i = 1; i < 4; ++i)
        if (get8(f) != ogg_page_header[i])
          break;
      if (f->eof) return 0;
      if (i == 4) {
        u8 header[27];
        u32 i, crc, goal, len;
        for (i = 0; i < 4; ++i)
          header[i] = ogg_page_header[i];
        for (; i < 27; ++i)
          header[i] = get8(f);
        if (f->eof) return 0;
        if (header[4] != 0) goto invalid;
        goal = header[22] + (header[23] << 8) + (header[24] << 16) + ((u32)header[25] << 24);
        for (i = 22; i < 26; ++i)
          header[i] = 0;
        crc = 0;
        for (i = 0; i < 27; ++i)
          crc = crc32_update(crc, header[i]);
        len = 0;
        for (i = 0; i < header[26]; ++i) {
          i32 s = get8(f);
          crc = crc32_update(crc, s);
          len += s;
        }
        if (len && f->eof) return 0;
        for (i = 0; i < len; ++i)
          crc = crc32_update(crc, get8(f));
        // finished parsing probable page
        if (crc == goal) {
          // we could now check that it's either got the last
          // page flag set, OR it's followed by the capture
          // pattern, but I guess TECHNICALLY you could have
          // a file with garbage between each ogg page and recover
          // from it automatically? So even though that paranoia
          // might decrease the chance of an invalid decode by
          // another 2^32, not worth it since it would hose those
          // invalid-but-useful files?
          if (end)
            *end = stb_vorbis_get_file_offset(f);
          if (last) {
            if (header[5] & 0x04)
              *last = 1;
            else
              *last = 0;
          }
          set_file_offset(f, retry_loc - 1);
          return 1;
        }
      }
    invalid:
      // not a valid page, so rewind and look for next one
      set_file_offset(f, retry_loc);
    }
  }
}


#define SAMPLE_unknown  0xffffffff

// seeking is implemented with a binary search, which narrows down the range to
// 64K, before using a linear search (because finding the synchronization
// pattern can be expensive, and the chance we'd find the end page again is
// relatively high for small ranges)
//
// two initial interpolation-style probes are used at the start of the search
// to try to bound either side of the binary search sensibly, while still
// working in O(log n) time if they fail.

static i32 get_seek_page_info(Ogg::vorbis* f, ProbedPage* z)
{
  u8 header[27], lacing[255];
  i32 i, len;

  // record where the page starts
  z->page_start = stb_vorbis_get_file_offset(f);

  // parse the header
  getn(f, header, 27);
  if (header[0] != 'O' || header[1] != 'g' || header[2] != 'g' || header[3] != 'S')
    return 0;
  getn(f, lacing, header[26]);

  // determine the length of the payload
  len = 0;
  for (i = 0; i < header[26]; ++i)
    len += lacing[i];

  // this implies where the page ends
  z->page_end = z->page_start + 27 + header[26] + len;

  // read the last-decoded sample out of the data
  z->last_decoded_sample = header[6] + (header[7] << 8) + (header[8] << 16) + (header[9] << 24);

  // restore file state to where we were
  set_file_offset(f, z->page_start);
  return 1;
}

// rarely used function to seek back to the preceding page while finding the
// start of a packet
static i32 go_to_page_before(Ogg::vorbis* f, u32 limit_offset)
{
  u32 previous_safe, end;

  // now we want to seek back 64K from the limit
  if (limit_offset >= 65536 && limit_offset - 65536 >= f->first_audio_page_offset)
    previous_safe = limit_offset - 65536;
  else
    previous_safe = f->first_audio_page_offset;

  set_file_offset(f, previous_safe);

  while (vorbis_find_page(f, &end, nullptr)) {
    if (end >= limit_offset && stb_vorbis_get_file_offset(f) < limit_offset)
      return 1;
    set_file_offset(f, end);
  }

  return 0;
}

// implements the search logic for finding a page and starting decoding. if
// the function succeeds, current_loc_valid will be true and current_loc will
// be less than or equal to the provided sample number (the closer the
// better).
static i32 seek_to_sample_coarse(Ogg::vorbis* f, u32 sample_number)
{
  ProbedPage left, right, mid;
  i32 i, start_seg_with_known_loc, end_pos, page_start;
  u32 delta, stream_length, padding, last_sample_limit;
  double offset = 0.0, bytes_per_sample = 0.0;
  i32 probe = 0;

  // find the last page and validate the target sample
  stream_length = stb_vorbis_stream_length_in_samples(f);
  if (stream_length == 0)            return error(f, VORBIS_seek_without_length);
  if (sample_number > stream_length) return error(f, VORBIS_seek_invalid);

  // this is the maximum difference between the window-center (which is the
  // actual granule position value), and the right-start (which the spec
  // indicates should be the granule position (give or take one)).
  padding = ((f->blocksize_1 - f->blocksize_0) >> 2);
  if (sample_number < padding)
    last_sample_limit = 0;
  else
    last_sample_limit = sample_number - padding;

  left = f->p_first;
  while (left.last_decoded_sample == ~0U) {
    // (untested) the first page does not have a 'last_decoded_sample'
    set_file_offset(f, left.page_end);
    if (!get_seek_page_info(f, &left)) goto error;
  }

  right = f->p_last;
  assert(right.last_decoded_sample != ~0U);

  // starting from the start is handled differently
  if (last_sample_limit <= left.last_decoded_sample) {
    if (stb_vorbis_seek_start(f)) {
      if (f->current_loc > sample_number)
        return error(f, VORBIS_seek_failed);
      return 1;
    }
    return 0;
  }

  while (left.page_end != right.page_start) {
    assert(left.page_end < right.page_start);
    // search range in bytes
    delta = right.page_start - left.page_end;
    if (delta <= 65536) {
      // there's only 64K left to search - handle it linearly
      set_file_offset(f, left.page_end);
    }
    else {
      if (probe < 2) {
        if (probe == 0) {
          // first probe (interpolate)
          double data_bytes = right.page_end - left.page_start;
          bytes_per_sample = data_bytes / right.last_decoded_sample;
          offset = left.page_start + bytes_per_sample * (last_sample_limit - left.last_decoded_sample);
        }
        else {
          // second probe (try to bound the other side)
          double error = ((double)last_sample_limit - mid.last_decoded_sample) * bytes_per_sample;
          if (error >= 0 && error < 8000) error = 8000;
          if (error <  0 && error > -8000) error = -8000;
          offset += error * 2;
        }

        // ensure the offset is valid
        if (offset < left.page_end)
          offset = left.page_end;
        if (offset > right.page_start - 65536)
          offset = right.page_start - 65536;

        set_file_offset(f, (u32)offset);
      }
      else {
        // binary search for large ranges (offset by 32K to ensure
        // we don't hit the right page)
        set_file_offset(f, left.page_end + (delta / 2) - 32768);
      }

      if (!vorbis_find_page(f, nullptr, nullptr)) goto error;
    }

    for (;;) {
      if (!get_seek_page_info(f, &mid)) goto error;
      if (mid.last_decoded_sample != ~0U) break;
      // (untested) no frames end on this page
      set_file_offset(f, mid.page_end);
      assert(mid.page_start < right.page_start);
    }

    // if we've just found the last page again then we're in a tricky file,
    // and we're close enough (if it wasn't an interpolation probe).
    if (mid.page_start == right.page_start) {
      if (probe >= 2 || delta <= 65536)
        break;
    }
    else {
      if (last_sample_limit < mid.last_decoded_sample)
        right = mid;
      else
        left = mid;
    }

    ++probe;
  }

  // seek back to start of the last packet
  page_start = left.page_start;
  set_file_offset(f, page_start);
  if (!start_page(f)) return error(f, VORBIS_seek_failed);
  end_pos = f->end_seg_with_known_loc;
  assert(end_pos >= 0);

  for (;;) {
    for (i = end_pos; i > 0; --i)
      if (f->segments[i - 1] != 255)
        break;

    start_seg_with_known_loc = i;

    if (start_seg_with_known_loc > 0 || !(f->page_flag & PAGEFLAG_continued_packet))
      break;

    // (untested) the final packet begins on an earlier page
    if (!go_to_page_before(f, page_start))
      goto error;

    page_start = stb_vorbis_get_file_offset(f);
    if (!start_page(f)) goto error;
    end_pos = f->segment_count - 1;
  }

  // prepare to start decoding
  f->current_loc_valid = FALSE;
  f->last_seg = FALSE;
  f->valid_bits = 0;
  f->packet_bytes = 0;
  f->bytes_in_seg = 0;
  f->previous_length = 0;
  f->next_seg = start_seg_with_known_loc;

  for (i = 0; i < start_seg_with_known_loc; i++)
    skip(f, f->segments[i]);

  // start decoding (optimizable - this frame is generally discarded)
  if (!vorbis_pump_first_frame(f))
    return 0;
  if (f->current_loc > sample_number)
    return error(f, VORBIS_seek_failed);
  return 1;

error:
  // try to restore the file to a valid state
  stb_vorbis_seek_start(f);
  return error(f, VORBIS_seek_failed);
}

// the same as vorbis_decode_initial, but without advancing
static i32 peek_decode_initial(Ogg::vorbis* f, i32* p_left_start, i32* p_left_end, i32* p_right_start, i32* p_right_end, i32* mode)
{
  i32 bits_read, bytes_read;

  if (!vorbis_decode_initial(f, p_left_start, p_left_end, p_right_start, p_right_end, mode))
    return 0;

  // either 1 or 2 bytes were read, figure out which so we can rewind
  bits_read = 1 + ilog(f->mode_count - 1);
  if (f->mode_config[*mode].blockflag)
    bits_read += 2;
  bytes_read = (bits_read + 7) / 8;

  f->bytes_in_seg += bytes_read;
  f->packet_bytes -= bytes_read;
  skip(f, -bytes_read);
  if (f->next_seg == -1)
    f->next_seg = f->segment_count - 1;
  else
    f->next_seg--;
  f->valid_bits = 0;

  return 1;
}

i32 stb_vorbis_seek_frame(Ogg::vorbis* f, u32 sample_number)
{
  u32 max_frame_samples;

  if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);

  // fast page-level search
  if (!seek_to_sample_coarse(f, sample_number))
    return 0;

  assert(f->current_loc_valid);
  assert(f->current_loc <= sample_number);

  // linear search for the relevant packet
  max_frame_samples = (f->blocksize_1 * 3 - f->blocksize_0) >> 2;
  while (f->current_loc < sample_number) {
    i32 left_start, left_end, right_start, right_end, mode, frame_samples;
    if (!peek_decode_initial(f, &left_start, &left_end, &right_start, &right_end, &mode))
      return error(f, VORBIS_seek_failed);
    // calculate the number of samples returned by the next frame
    frame_samples = right_start - left_start;
    if (f->current_loc + frame_samples > sample_number) {
      return 1; // the next frame will contain the sample
    }
    else if (f->current_loc + frame_samples + max_frame_samples > sample_number) {
      // there's a chance the frame after this could contain the sample
      vorbis_pump_first_frame(f);
    }
    else {
      // this frame is too early to be relevant
      f->current_loc += frame_samples;
      f->previous_length = 0;
      maybe_start_packet(f);
      flush_packet(f);
    }
  }
  // the next frame should start with the sample
  if (f->current_loc != sample_number) return error(f, VORBIS_seek_failed);
  return 1;
}

i32 stb_vorbis_seek(Ogg::vorbis* f, u32 sample_number)
{
  if (!stb_vorbis_seek_frame(f, sample_number))
    return 0;

  if (sample_number != f->current_loc) {
    i32 n;
    u32 frame_start = f->current_loc;
    stb_vorbis_get_frame_float(f, &n, nullptr);
    assert(sample_number > frame_start);
    assert(f->channel_buffer_start + (i32)(sample_number - frame_start) <= f->channel_buffer_end);
    f->channel_buffer_start += (sample_number - frame_start);
  }

  return 1;
}

i32 stb_vorbis_seek_start(Ogg::vorbis* f)
{
  if (IS_PUSH_MODE(f)) { return error(f, VORBIS_invalid_api_mixing); }
  set_file_offset(f, f->first_audio_page_offset);
  f->previous_length = 0;
  f->first_decode = TRUE;
  f->next_seg = -1;
  return vorbis_pump_first_frame(f);
}

u32 stb_vorbis_stream_length_in_samples(Ogg::vorbis* f)
{
  u32 restore_offset, previous_safe;
  u32 end, last_page_loc;

  if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);
  if (!f->total_samples) {
    u32 last;
    u32 lo, hi;
    char header[6];

    // first, store the current decode position so we can restore it
    restore_offset = stb_vorbis_get_file_offset(f);

    // now we want to seek back 64K from the end (the last page must
    // be at most a little less than 64K, but let's allow a little slop)
    if (f->stream_len >= 65536 && f->stream_len - 65536 >= f->first_audio_page_offset)
      previous_safe = f->stream_len - 65536;
    else
      previous_safe = f->first_audio_page_offset;

    set_file_offset(f, previous_safe);
    // previous_safe is now our candidate 'earliest known place that seeking
    // to will lead to the final page'

    if (!vorbis_find_page(f, &end, &last)) {
      // if we can't find a page, we're hosed!
      f->error = VORBIS_cant_find_last_page;
      f->total_samples = 0xffffffff;
      goto done;
    }

    // check if there are more pages
    last_page_loc = stb_vorbis_get_file_offset(f);

    // stop when the last_page flag is set, not when we reach eof;
    // this allows us to stop short of a 'file_section' end without
    // explicitly checking the length of the section
    while (!last) {
      set_file_offset(f, end);
      if (!vorbis_find_page(f, &end, &last)) {
        // the last page we found didn't have the 'last page' flag
        // set. whoops!
        break;
      }
      //previous_safe = last_page_loc+1; // NOTE: not used after this point, but note for debugging
      last_page_loc = stb_vorbis_get_file_offset(f);
    }

    set_file_offset(f, last_page_loc);

    // parse the header
    getn(f, (u8*)header, 6);
    // extract the absolute granule position
    lo = get32(f);
    hi = get32(f);
    if (lo == 0xffffffff && hi == 0xffffffff) {
      f->error = VORBIS_cant_find_last_page;
      f->total_samples = SAMPLE_unknown;
      goto done;
    }
    if (hi)
      lo = 0xfffffffe; // saturate
    f->total_samples = lo;

    f->p_last.page_start = last_page_loc;
    f->p_last.page_end = end;
    f->p_last.last_decoded_sample = lo;

  done:
    set_file_offset(f, restore_offset);
  }
  return f->total_samples == SAMPLE_unknown ? 0 : f->total_samples;
}

f32 stb_vorbis_stream_length_in_seconds(Ogg::vorbis* f)
{
  return stb_vorbis_stream_length_in_samples(f) / (f32)f->sample_rate;
}



i32 stb_vorbis_get_frame_float(Ogg::vorbis* f, i32* channels, f32*** output)
{
  i32 len, right, left, i;
  if (IS_PUSH_MODE(f)) return error(f, VORBIS_invalid_api_mixing);

  if (!vorbis_decode_packet(f, &len, &left, &right)) {
    f->channel_buffer_start = f->channel_buffer_end = 0;
    return 0;
  }

  len = vorbis_finish_frame(f, len, left, right);
  for (i = 0; i < f->channels; ++i)
    f->outputs[i] = f->channel_buffers[i] + left;

  f->channel_buffer_start = left;
  f->channel_buffer_end = left + len;

  if (channels) *channels = f->channels;
  if (output)   *output = f->outputs;
  return len;
}

Ogg::vorbis* Ogg::open(const u8* data, i32 len)
{
  vorbis* f, p;
  if (!data) {
    return nullptr;
  }
  vorbis_init(&p, nullptr);
  p.stream = (u8*)data;
  p.stream_end = (u8*)data + len;
  p.stream_start = (u8*)p.stream;
  p.stream_len = len;
  p.push_mode = FALSE;
  if (start_decoder(&p)) {
    f = vorbis_alloc(&p);
    if (f) {
      *f = p;
      vorbis_pump_first_frame(f);
      return f;
    }
  }
  vorbis_deinit(&p);
  return nullptr;
}

i32 Ogg::getSamplesInterleaved(Ogg::vorbis* f, i32 channels, f32* buffer, i32 num_floats)
{
  f32** outputs;
  i32 len = num_floats / channels;
  i32 n = 0;
  i32 z = f->channels;
  if (z > channels) z = channels;
  while (n < len) {
    i32 i, j;
    i32 k = f->channel_buffer_end - f->channel_buffer_start;
    if (n + k >= len) k = len - n;
    for (j = 0; j < k; ++j) {
      for (i = 0; i < z; ++i)
        *buffer++ = f->channel_buffers[i][f->channel_buffer_start + j];
      for (; i < channels; ++i)
        *buffer++ = 0;
    }
    n += k;
    f->channel_buffer_start += k;
    if (n == len)
      break;
    if (!stb_vorbis_get_frame_float(f, nullptr, &outputs))
      break;
  }
  return n;
}

i32 stb_vorbis_get_samples_float(Ogg::vorbis* f, i32 channels, f32** buffer, i32 num_samples)
{
  f32** outputs;
  i32 n = 0;
  i32 z = f->channels;
  if (z > channels) z = channels;
  while (n < num_samples) {
    i32 i;
    i32 k = f->channel_buffer_end - f->channel_buffer_start;
    if (n + k >= num_samples) k = num_samples - n;
    if (k) {
      for (i = 0; i < z; ++i)
        memcpy(buffer[i] + n, f->channel_buffers[i] + f->channel_buffer_start, sizeof(f32) * k);
      for (; i < channels; ++i)
        memset(buffer[i] + n, 0, sizeof(f32) * k);
    }
    n += k;
    f->channel_buffer_start += k;
    if (n == num_samples)
      break;
    if (!stb_vorbis_get_frame_float(f, nullptr, &outputs))
      break;
  }
  return n;
}
