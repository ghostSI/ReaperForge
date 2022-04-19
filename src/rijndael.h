#ifndef RIJNDAEL_H
#define RIJNDAEL_H

#include "typedefs.h"

namespace Rijndael {
    void decrypt(const u8 *key, const u8 *in, u8 *result, size_t n, const u8* iv = nullptr);
};

#endif // RIJNDAEL_H