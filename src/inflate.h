#ifndef INFLATE_H
#define INFLATE_H

#include "typedefs.h"

namespace Inflate
{
    i32 inflate(const void *compressed_data, i32 compressed_size,
                 void *output_buffer, i32 output_size,
                 u32 *crc_ret = nullptr);
}


#endif // INFLATE_H