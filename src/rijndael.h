#ifndef RIJNDAEL_H
#define RIJNDAEL_H

#include "typedefs.h"

class Rijndael
{
public:
    void MakeKey(u8 const* key);

public:
    void EncryptBlock(u8 const* in, u8* result);

    void Decrypt(u8 const* in, u8* result, size_t n);

private:
    //Encryption (m_Ke) round key
    int m_Ke[15][8];
    //Decryption (m_Kd) round key
    int m_Kd[15][8];
    //Key Length
    int m_keylength;
    //Block Size
    //Number of Rounds
    //Chain Block
    u8 m_chain0[32];
    u8 m_chain[32];
    //Auxiliary private use buffers
    int tk[8];
    int a[8];
    int t[8];
};

#endif // RIJNDAEL_H