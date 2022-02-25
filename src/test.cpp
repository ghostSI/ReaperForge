#include "test.h"
#include "rijndael.h"

void rijndaelTest()
{
    static const u8 psarcKey[] =
            {
                    0xC5, 0x3D, 0xB2, 0x38, 0x70, 0xA1, 0xA2, 0xF7,
                    0x1C, 0xAE, 0x64, 0x06, 0x1F, 0xDD, 0x0E, 0x11,
                    0x57, 0x30, 0x9D, 0xC8, 0x52, 0x04, 0xD4, 0xC5,
                    0xBF, 0xDF, 0x25, 0x09, 0x0D, 0xF2, 0x57, 0x2C
            };

    static const u8 encrypted_data[] = {
            0x50, 0x53, 0x41, 0x52, 0x00, 0x01, 0x00, 0x04, 0x7a, 0x6c, 0x69, 0x62,
            0x00, 0x00, 0x04, 0x26, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x19,
            0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x9b, 0x35, 0x0f, 0xf5,
            0x18, 0xb6, 0x64, 0x91, 0x66, 0xfd, 0xea, 0xa7, 0xdc, 0xbc, 0x69, 0xed,
            0x68, 0x11, 0x9a, 0x75, 0x08, 0xdb, 0x0c, 0x5c, 0x3e, 0xfe, 0x7d, 0x09,
            0xd2, 0x89, 0xcc, 0xdb, 0xd4, 0x06, 0xd2, 0x11, 0x81, 0x94, 0x65, 0xf2,
            0x09, 0x63, 0x37, 0xca, 0x7b, 0xb2, 0x9c, 0xbf, 0xe9, 0x0a, 0x3a, 0x0a,
            0x05, 0xc8, 0xf6, 0xcd, 0xab, 0xa3, 0xfe, 0xc9, 0x15, 0x02, 0x4b, 0x70,
            0x69, 0x4e, 0xee, 0x56, 0x45, 0x83, 0xc2, 0x5b, 0x2e, 0xdf, 0xed, 0x76,
            0x72, 0x7a, 0x30, 0xfd, 0x15, 0x1d, 0x39, 0x81, 0x9b, 0x16, 0x8a, 0x81,
            0x08, 0xb9, 0xff, 0xb5, 0xbb, 0xcc, 0x5d, 0x35, 0x6f, 0x18, 0x1e, 0xf0,
            0xf1, 0x57, 0x28, 0x4d, 0x1b, 0x82, 0x74, 0x0a, 0xd5, 0xf1, 0x92, 0x97,
            0xb1, 0xfb, 0xfe, 0x00, 0x25, 0xd4, 0x31, 0x02, 0x82, 0xc2, 0x59, 0xd7,
            0x5f, 0x70, 0x63, 0x67, 0x20, 0x0c, 0xd2, 0xab, 0x41, 0x4d, 0xd4, 0xad,
            0xb9, 0x0c, 0x9b, 0xe4, 0xd7, 0xba, 0x31, 0x5f, 0xb4, 0xae, 0xe6, 0x83,
            0xf4, 0x26, 0x0e, 0x81, 0xcb, 0xa8, 0x26, 0xfc, 0xd7, 0xf7, 0x33, 0x71,
            0x53, 0x03, 0x4b, 0xeb, 0x6e, 0x21, 0x52, 0x42, 0xe2, 0x4e, 0x41, 0xd0,
            0xbd, 0xe7, 0x88, 0x3d, 0xa4, 0x70, 0xd8, 0x90, 0x07, 0x0e, 0x24, 0xe9,
            0xc2, 0xad, 0x81, 0xce, 0x1c, 0x49, 0xf4, 0x86, 0xc4, 0xf6, 0x2a, 0x55,
            0xe6, 0x75, 0x84, 0x01, 0x89, 0x7b, 0xa6, 0xcb, 0x62, 0x82, 0xb5, 0xf1,
            0x46, 0xb8, 0x37, 0x6f, 0x60, 0xcb, 0x5b, 0x7b, 0x6f, 0x76, 0x90, 0x7c,
            0x30, 0xff, 0xa0, 0x26, 0xdd, 0xcc, 0x02, 0xd5, 0xc8, 0xf1, 0xc9, 0x5f,
            0x62, 0xdf, 0x35, 0xcf, 0x3a, 0x90, 0xd6, 0x9a, 0x92, 0xba, 0xbf, 0xae,
            0x90, 0xbd, 0xb7, 0x1c, 0xf1, 0x4f, 0x43, 0x83, 0x4b, 0x2b, 0xb7, 0x6a,
            0x5c, 0xc7, 0x6e, 0x1c, 0xac, 0x8a, 0x22, 0xf8, 0xf5, 0xa3, 0x84, 0x53,
            0xd0, 0xb3, 0xa9, 0x65, 0x63, 0x77, 0x94, 0xb3, 0x27, 0xf9, 0x9e, 0xa5,
            0xd9, 0x07, 0xa7, 0x57, 0xe2, 0x4e, 0x3c, 0x70, 0x81, 0xf2, 0x1a, 0x77,
            0x6d, 0x17, 0xdf, 0xe5, 0x72, 0x8f, 0x1f, 0xd2, 0x41, 0xab, 0xcd, 0xf7,
            0x2a, 0x9b, 0x0b, 0x25, 0x7b, 0x4f, 0x95, 0x52, 0xcc, 0x8f, 0x82, 0xa9,
            0xf3, 0x8d, 0xb1, 0x2b, 0x32, 0x2a, 0x90, 0x0c, 0x87, 0xa6, 0xcf, 0x16,
            0x72, 0xe7, 0x48, 0x30, 0x9d, 0xe7, 0x46, 0x89, 0x42, 0x6a, 0x64, 0xf5,
            0x41, 0x1e, 0x45, 0x0f, 0x19, 0x73, 0x65, 0xb6, 0x16, 0x30, 0x0f, 0x71,
            0x8b, 0xf5, 0xf2, 0x78, 0x4e, 0x35, 0x6d, 0x88, 0x76, 0x49, 0xcb, 0x5e,
            0xc6, 0xd2, 0xa7, 0x9e, 0x70, 0x4a, 0x7b, 0xef, 0x40, 0xf9, 0x7b, 0xe2,
            0x24, 0xe4, 0x5c, 0xa4, 0xa6, 0x11, 0x0f, 0xac, 0xd2, 0x57, 0x37, 0x9c,
            0xab, 0x00, 0x00, 0xf1, 0xff, 0x0d, 0x6a, 0x78, 0x41, 0x06, 0x11, 0x22,
            0x53, 0x48, 0xc1, 0x92, 0x29, 0x2b, 0xab, 0x5e, 0xfc, 0x0a, 0x85, 0xba,
            0x35, 0x8f, 0xde, 0xbb, 0xeb, 0xbc, 0xa9, 0xc2, 0xf0, 0xe8, 0xf9, 0x59,
            0xbf, 0xc3, 0x23, 0x9d, 0x1a, 0x58, 0xa5, 0x32, 0xc3, 0xc5, 0xd3, 0x24,
            0x05, 0xe5, 0x0a, 0xaa, 0xa8, 0x7c, 0x83, 0x6b, 0x97, 0x7d, 0xea, 0x82,
            0x5c, 0x37, 0x2f, 0x63, 0xf2, 0xa4, 0x81, 0x0b, 0x2a, 0xed, 0x58, 0xe6,
            0xe6, 0x51, 0x30, 0x45, 0x53, 0x15, 0xa6, 0xd4, 0x1c, 0xf9, 0x78, 0xc6,
            0x08, 0xa5, 0x04, 0x71, 0x28, 0x6d, 0xc2, 0x60, 0x59, 0xed, 0x59, 0xea,
            0xb3, 0xf4, 0xcb, 0x0b, 0x29, 0x71, 0xb4, 0x94, 0x19, 0x7a, 0x19, 0x87,
            0xd7, 0x09, 0xbc, 0x8a, 0x0e, 0xd9, 0x92, 0x04, 0x41, 0xd8, 0x16, 0x44,
            0xd0, 0xe4, 0x6f, 0xdd, 0x5d, 0x95, 0x90, 0x08, 0xa6, 0x0e, 0x46, 0x49,
            0xac, 0xf7, 0xa8, 0x7d, 0x0c, 0x2a, 0xbb, 0x54, 0x2e, 0xcc, 0xee, 0x1e,
            0x3c, 0xb1, 0xe0, 0x06, 0x74, 0x4f, 0x9f, 0xb3, 0x67, 0x55, 0x2a, 0x6f,
            0xa9, 0x54, 0xfc, 0xd1, 0x3d, 0x46, 0xfd, 0x5f, 0x7e, 0x99, 0x5f, 0x1e,
            0x01, 0x93, 0xb1, 0xc5, 0x48, 0xc8, 0x2b, 0xef, 0x8d, 0x8c, 0x15, 0xe2,
            0x24, 0xe8, 0x3c, 0x81, 0xb3, 0x36, 0x14, 0x15, 0xf8, 0xd9, 0xb5, 0x30,
            0xf3, 0x0d, 0x03, 0x70, 0xa2, 0x91, 0x57, 0x5a, 0x53, 0x4a, 0x6a, 0x72,
            0xf9, 0x8e, 0x03, 0x4b, 0xb5, 0xf6, 0x5b, 0xf9, 0xfe, 0xc5, 0x63, 0x20,
            0x1e, 0x8b, 0xe0, 0xbc, 0x31, 0xf5, 0x5c, 0x68, 0xb2, 0xd1, 0x6c, 0x48,
            0xfb, 0xa6, 0xd9, 0x61, 0x5d, 0xde, 0x27, 0xa3, 0x34, 0x5d, 0x8d, 0x38,
            0x3c, 0x32, 0xb4, 0x7e, 0x9b, 0x00, 0xe8, 0x00, 0x85, 0x37, 0xf2, 0xca,
            0xfd, 0x9e, 0xce, 0x0c, 0xef, 0xf5, 0x69, 0x98, 0xe7, 0x13, 0x22, 0xfc,
            0xee, 0xc7, 0x4c, 0xf2, 0xb5, 0x3e, 0x7c, 0x47, 0x62, 0x42, 0xf9, 0x5e,
            0x91, 0xbc, 0x6c, 0x92, 0xe8, 0xb0, 0x9c, 0x86, 0xe9, 0x25, 0x21, 0x63,
            0x98, 0xea, 0x5b, 0x8b, 0x68, 0x5f, 0x13, 0x1c, 0xcc, 0x2d, 0xd1, 0xbc,
            0x15, 0xb2, 0x71, 0x15, 0x76, 0x60, 0x55, 0x5e, 0xd8, 0x14, 0x8b, 0x6b,
            0x07, 0x80, 0x64, 0xf4, 0xf6, 0x5d, 0x4c, 0x60, 0x3e, 0x2d, 0x18, 0x1d,
            0x57, 0x87, 0x13, 0xb3, 0x7c, 0xc6, 0x06, 0x53, 0x9a, 0xf6, 0xac, 0xb5,
            0x00, 0x4c, 0x49, 0x95, 0xe5, 0x85, 0xa5, 0x2d, 0xe1, 0x4e, 0x4a, 0xe1,
            0x59, 0x9c, 0x72, 0xe6, 0x15, 0x07, 0x15, 0x15, 0x6e, 0x26, 0xb1, 0xa9,
            0x90, 0x07, 0x57, 0x06, 0xc1, 0xf0, 0x14, 0x66, 0xce, 0x34, 0xbf, 0x44,
            0x84, 0xfe, 0x82, 0x5a, 0x01, 0x32, 0x9e, 0xe6, 0x4d, 0x9c, 0x34, 0xfd,
            0xc9, 0xfd, 0x90, 0x2e, 0x38, 0x32, 0x7e, 0x0f, 0xfa, 0x6c, 0xbf, 0xdc,
            0xdc, 0xc3, 0x7a, 0xba, 0xac, 0x8a, 0xed, 0xd8, 0x43, 0x1f, 0xa7, 0xc8,
            0x38, 0x2d, 0xd4, 0xa0, 0xd2, 0xe0, 0x78, 0x7a, 0xe9, 0x52, 0x31, 0x41,
            0x90, 0xfd, 0x8e, 0x77, 0x01, 0x50, 0x14, 0xab, 0xda, 0x58, 0xd3, 0xb6,
            0x73, 0xc8, 0x96, 0x51, 0xbc, 0xa5, 0x23, 0x4d, 0xab, 0x4b, 0xb2, 0xc7,
            0x11, 0xfa, 0x92, 0x4e, 0x11, 0x89, 0xf7, 0x11, 0x86, 0xdb, 0x7e, 0xbc,
            0x2b, 0x7c, 0x2e, 0xe0, 0x1b, 0x76, 0x57, 0x28, 0xa0, 0x91, 0xac, 0x86,
            0x68, 0xc6, 0x0b, 0x10, 0x0a, 0x13, 0x62, 0xf9, 0x89, 0xdf, 0xd5, 0x4f,
            0xc9, 0xfa, 0xb1, 0x92, 0x60, 0x3c, 0x56, 0xfb, 0x25, 0x96, 0x44, 0x5b,
            0xa8, 0x3d, 0x29, 0xcc, 0xc2, 0x79, 0xa2, 0x27, 0x86, 0x23, 0x12, 0x1f,
            0x9a, 0xb7, 0x62, 0x53, 0xd7, 0x3e, 0x8b, 0x30, 0x15, 0xbb, 0x39, 0x89,
            0x07, 0x75, 0x2b, 0xd1, 0x33, 0x36, 0xee, 0x46, 0xb5, 0x29, 0x44, 0x48,
            0xb6, 0x04, 0x95, 0xcf, 0xa6, 0x8d, 0x85, 0x04, 0xb5, 0x6e, 0xe5, 0x34,
            0x42, 0x70, 0xff, 0xea, 0xe8, 0xde, 0xa8, 0xf7, 0x76, 0x77, 0x4b, 0x4b,
            0xb4, 0xcd, 0xd8, 0xeb, 0x8f, 0xec, 0x70, 0xf3, 0xc5, 0x64, 0xd8, 0xe0,
            0xd7, 0x8b, 0x7e, 0x0f, 0x41, 0x00, 0xb2, 0x5a, 0xf6, 0x85, 0xde, 0xd2,
            0xba, 0xa0, 0x7f, 0xb3, 0x6f, 0xfb, 0x42, 0x7b, 0xbe, 0x4a, 0x42, 0x26,
            0xb6, 0x88, 0xda, 0x9f, 0x8e, 0xc5, 0x23, 0x30, 0xb6, 0x58, 0x8e, 0x57,
            0xb3, 0xfd, 0x8e, 0x92, 0x3e, 0x32, 0x51, 0x99, 0xfd, 0x76, 0xc5, 0x93,
            0x9f, 0xf3, 0x48, 0x83, 0x18, 0x2a, 0xc8, 0xaf, 0xee, 0x72, 0xdc, 0x3b,
            0x7a, 0xe4, 0x49, 0x85, 0xbe, 0x51, 0x81, 0x60, 0xac, 0x7c, 0x1b, 0x4f,
            0x19, 0x3b, 0xe0, 0x90, 0x17, 0x59, 0x78, 0xda, 0xad, 0x92, 0xdd, 0x8e,
            0x83, 0x20, 0x10, 0x85, 0xef, 0x7d, 0x18, 0x58, 0x6d, 0xb5, 0x7d, 0x1b,
            0x33, 0xca, 0x14, 0x67, 0x8b, 0x83, 0x61, 0x50, 0xfb, 0xf8, 0xf5, 0xef,
            0x6a, 0xb3, 0xb1, 0xb5, 0x69, 0x42, 0x08, 0x81, 0x73, 0xbe, 0x39, 0x27,
            0xc1, 0xde, 0x1e, 0x20, 0x82, 0x51, 0x34, 0xb8, 0xaa, 0x6f, 0x4b, 0x08,
            0x71, 0x3b, 0x19, 0x08, 0xd4, 0x08, 0x34, 0xdc, 0x73, 0x24, 0x47, 0xd1,
            0x7b, 0x43, 0x22, 0xde, 0x0d, 0x58, 0x16, 0x67, 0x65, 0x8c, 0x24, 0xf6,
            0x13, 0x6f, 0x9a, 0x5d, 0x3f, 0x37, 0x67, 0x79, 0xb1, 0x98, 0x6f, 0x0e,
            0x62, 0xeb, 0x0d, 0x3a, 0xd1, 0x61, 0x5e, 0xc8, 0x7d, 0x8b, 0x01, 0x2a,
            0x87, 0x65, 0xf0, 0x3e, 0xaa, 0xf9, 0x7d, 0x4f, 0x24, 0x9e, 0xed, 0x2a,
            0x9a, 0xe8, 0xee, 0x4e, 0x51, 0x0d, 0x18, 0x84, 0x3c, 0x27, 0xd0, 0x75,
            0x64, 0xd4, 0xb2, 0x27, 0xd0, 0x1b, 0xf2, 0x7a, 0x24, 0x36, 0x7e, 0x14,
            0x3d, 0x7b, 0x76, 0xb2, 0xa9, 0x8a, 0xef, 0x7f, 0x1c, 0x69, 0x9e, 0xe6,
            0x97, 0xd3, 0x4f, 0x9e, 0x9e, 0xd5, 0x88, 0xed, 0x31, 0x5c, 0xd9, 0x05,
            0x1c, 0x08, 0xc7, 0x7f, 0xb0, 0x59, 0x7e, 0xba, 0x5c, 0x8b, 0x22, 0x4b,
            0x17, 0xea, 0x0e, 0x02, 0xac, 0x0d, 0x68, 0x21, 0xa2, 0x0d, 0xd0, 0x35,
            0x8a, 0x63, 0x32, 0x0f, 0x15, 0x5d, 0x11, 0x6b, 0x8b, 0x8c, 0x81, 0x6a,
            0xbd, 0x63, 0x77, 0x08, 0x46, 0x09, 0xdb, 0xcd, 0x05, 0x21, 0xbc, 0x54,
            0x3f, 0x5a, 0x97, 0xb4, 0xc0, 0x74, 0x43, 0x89, 0x6b, 0x45, 0x29, 0x8d,
            0xab, 0x77, 0x7a, 0xbe, 0x44, 0xfe, 0x4e, 0x94, 0x63, 0xb9, 0xab, 0xe9,
            0x5f, 0xbd, 0x9f, 0x7b, 0x51, 0x7f, 0x37, 0xf7, 0x82, 0x3c, 0x9e, 0x7b,
            0xf0, 0x35, 0xb8, 0x03, 0xc9, 0x37, 0xfd, 0x77, 0xb3, 0x6f, 0xd0, 0x25,
            0xfd, 0x41, 0xea, 0x3b, 0x22, 0x35, 0x5d, 0xf3, 0x7b, 0xf5, 0xa4, 0xf1,
            0xa3, 0x23, 0xdb, 0xc4, 0xb5, 0xa2, 0x85, 0x16, 0x1f, 0x95, 0xf3, 0xf5,
            0x5d, 0x34, 0xaf, 0x80, 0x9d, 0x31, 0xab, 0xf2, 0x09, 0x0a, 0x61, 0xec,
            0xf7, 0x78, 0xda, 0x9d, 0x55, 0x41, 0x68, 0x1b, 0x47, 0x14, 0x1d, 0x2d,
            0x51, 0x2b, 0x6d, 0x0f, 0x35, 0x71, 0xad, 0x40, 0x4e, 0xa6, 0xb0, 0xc2,
            0x9a, 0x1c, 0x8a, 0xd1, 0x21, 0x94, 0xe6, 0x60, 0x07, 0xe7, 0x90, 0xd0,
            0x83, 0x1a, 0x69, 0x36, 0xab, 0x25, 0x07, 0xc5, 0xa0, 0x62, 0x6d, 0x2e,
            0x85, 0xe8, 0x50, 0x68, 0x03, 0x2b, 0x95, 0x80, 0x23, 0x7a, 0x50, 0x40,
            0x6b, 0x6c, 0xe1, 0xc2, 0x46, 0x68, 0xc4, 0xee, 0x10, 0x1d, 0x0c, 0x9d,
            0x43, 0x0f, 0x9b, 0xdb, 0xe6, 0x66, 0x41, 0x0a, 0x12, 0x34, 0x07, 0xe5,
            0x96, 0x40, 0x7b, 0x30, 0x35, 0x54, 0x86, 0x5e, 0xdc, 0xff, 0x67, 0x7d,
            0x6b, 0x4f, 0x5d, 0x58, 0x1e, 0x7f, 0x76, 0xe6, 0xbf, 0xf7, 0xff, 0xfc,
            0xff, 0x77, 0x6b, 0xab, 0xbc, 0xfa, 0x3d, 0x21, 0xe4, 0xc3, 0xa5, 0x0c,
            0xd9, 0x00, 0xc4, 0x97, 0x64, 0xc8, 0xff, 0x7a, 0x56, 0xe1, 0xbd, 0x04,
            0xef, 0x96, 0x55, 0x59, 0xff, 0xcf, 0x0d, 0x4b, 0xff, 0x5e, 0x9a, 0xc6,
            0x8b, 0x98, 0xb3, 0x07, 0x0f, 0x10, 0x85, 0xe0, 0x5c, 0x46, 0xd3, 0x38,
            0xe4, 0xf5, 0x3a, 0xa2, 0xa6, 0x53, 0x2a, 0x23, 0x19, 0x77, 0x84, 0x10,
            0x88, 0xed, 0x01, 0xe2, 0x34, 0xa6, 0x22, 0xec, 0xa3, 0x1d, 0xc0, 0x7a,
            0x0f, 0x6c, 0x5f, 0x88, 0x3e, 0xae, 0x0b, 0x91, 0x9c, 0x13, 0x62, 0xaf,
            0xee, 0x21, 0xd2, 0x11, 0xeb, 0xc1, 0x3e, 0x31, 0xf2, 0x12, 0x0c, 0x47,
            0xa2, 0x09, 0xa8, 0xd3, 0x3c, 0x2d, 0xcb, 0x5e, 0xd4, 0x11, 0x0d, 0xb6,
            0x88, 0x73, 0x13, 0xa2, 0x0b, 0x81, 0xfc, 0xc6, 0xda, 0x88, 0x23, 0x12,
            0x92, 0xf0, 0x84, 0x1e, 0x63, 0x88, 0xbb, 0x22, 0x54, 0xd8, 0x17, 0x89,
            0x3d, 0x12, 0xac, 0x91, 0xf0, 0xd8, 0xca, 0x76, 0xec, 0x2a, 0x53, 0xe7,
            0xb4, 0x81, 0xf2, 0xe3, 0xfa, 0x62, 0x80, 0xa8, 0xc1, 0x73, 0x11, 0x8f,
            0x87, 0xfc, 0x61, 0xc3, 0x69, 0xa0, 0x5e, 0xa0, 0x1b, 0xa1, 0x4d, 0x05,
            0xa5, 0xc8, 0x0f, 0x61, 0x08, 0x44, 0x5d, 0xd7, 0xb4, 0x24, 0x0f, 0xa1,
            0xf2, 0x43, 0x61, 0xa3, 0x54, 0x76, 0xa2, 0x2f, 0xe3, 0xfa, 0x1d, 0x19,
            0x2d, 0x8e, 0x13, 0xfb, 0xdb, 0xd9, 0x30, 0x20, 0x64, 0x06, 0x58, 0xa0,
            0x49, 0x1e, 0xa9, 0xbe, 0xdb, 0x4f, 0xf4, 0xeb, 0xba, 0xe2, 0x0d, 0x1d,
            0x81, 0x79, 0x60, 0x2a, 0x4f, 0x32, 0x76, 0xc4, 0x80, 0x20, 0xe6, 0xdb,
            0xfe, 0x46, 0x4f, 0xe5, 0x47, 0x28, 0x7e, 0x1a, 0x86, 0x22, 0xc9, 0xc3,
            0x05, 0xbf, 0x6d, 0x57, 0xd5, 0xbd, 0x88, 0x42, 0x78, 0xa1, 0xa3, 0x83,
            0x18, 0x04, 0x17, 0x3a, 0x34, 0xaa, 0x30, 0xd8, 0xd5, 0xf4, 0x64, 0x3d,
            0xc0, 0xef, 0xc7, 0x3b, 0x17, 0x3a, 0x0b, 0x61, 0x58, 0x48, 0x74, 0xb8,
            0x9d, 0xe4, 0x1e, 0xb8, 0x07, 0x79, 0x8f, 0x44, 0x40, 0x79, 0x33, 0xea,
            0x45, 0xb6, 0xb5, 0xe3, 0xa3, 0x0e, 0x1e, 0x64, 0xc8, 0x19, 0xf0, 0xb2,
            0x06, 0x57, 0xfc, 0x59, 0x5d, 0x57, 0xfe, 0x6c, 0xd6, 0x38, 0x50, 0x28,
            0x0a, 0xca, 0xd6, 0x33, 0x49, 0x7c, 0xc1, 0x4e, 0x10, 0x28, 0x7d, 0x21,
            0xe6, 0xeb, 0xde, 0x8c, 0x64, 0xf2, 0xd6, 0x34, 0xbe, 0x32, 0x01, 0x9e
    };

    unsigned char plainText_data[] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
            0xdd, 0x00, 0x00, 0x00, 0x04, 0x26, 0x29, 0xa8, 0xc0, 0x34, 0x0c, 0xdc,
            0x9a, 0x33, 0x37, 0x77, 0xca, 0x15, 0xd6, 0xf1, 0xc0, 0x7d, 0x00, 0x00,
            0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x80, 0x00, 0x00, 0x00, 0x05, 0x4d,
            0x83, 0xb1, 0xa2, 0xda, 0x29, 0x53, 0xf3, 0xe0, 0xf7, 0x9f, 0xb7, 0x68,
            0x90, 0x7c, 0x91, 0x1d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20,
            0x80, 0x00, 0x00, 0x00, 0x0a, 0xed, 0xed, 0x30, 0x0c, 0x52, 0xc0, 0x57,
            0x0b, 0x83, 0x19, 0xb2, 0x56, 0xa0, 0x69, 0x97, 0xfd, 0x3f, 0x00, 0x00,
            0x00, 0x03, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x1e, 0xab,
            0x16, 0x9b, 0x7d, 0x23, 0xd5, 0xde, 0xd1, 0x7b, 0x9c, 0xd7, 0x38, 0x34,
            0xe6, 0x71, 0xb8, 0x5b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01,
            0x66, 0x00, 0x00, 0x00, 0x66, 0x6e, 0xf3, 0x66, 0x5d, 0x1b, 0xf6, 0x56,
            0xe6, 0x8f, 0x34, 0xe4, 0xc9, 0xac, 0xe8, 0x23, 0x7e, 0x2f, 0x00, 0x00,
            0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0xcf, 0x00, 0x00, 0x00, 0x67, 0x3e,
            0x99, 0x07, 0x9b, 0x19, 0x21, 0xc1, 0xa9, 0xc2, 0xb2, 0xae, 0x1e, 0xdc,
            0x88, 0xaa, 0xb2, 0x69, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
            0xbd, 0x00, 0x00, 0x00, 0x68, 0xbb, 0x8f, 0x63, 0xcc, 0x58, 0x19, 0xe2,
            0xd7, 0x76, 0xda, 0x20, 0xbb, 0xa4, 0x66, 0x18, 0x88, 0x58, 0x00, 0x00,
            0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x69, 0x43,
            0x54, 0xaf, 0xef, 0x35, 0x7e, 0x97, 0x9d, 0x51, 0x73, 0xa3, 0x80, 0xd9,
            0xca, 0x1a, 0x3c, 0xd4, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xc9,
            0x3b, 0x00, 0x00, 0x00, 0x69, 0x49, 0xc8, 0x9c, 0x45, 0x43, 0x5f, 0x44,
            0x81, 0xf7, 0x5f, 0xc2, 0xc0, 0xe2, 0x33, 0xcf, 0xbb, 0x8b, 0x00, 0x00,
            0x00, 0x09, 0x00, 0x00, 0x57, 0x5c, 0x8c, 0x00, 0x00, 0x01, 0x2a, 0xa6,
            0xc5, 0xcf, 0x99, 0x7f, 0x79, 0x66, 0x9f, 0x95, 0xf0, 0xed, 0x28, 0xb3,
            0x62, 0x74, 0xc1, 0x3d, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0xc9,
            0x43, 0x00, 0x00, 0x58, 0x5f, 0xbe, 0x91, 0x78, 0x89, 0x9c, 0x7a, 0x8c,
            0xe3, 0xbb, 0xda, 0xb2, 0xea, 0x70, 0x61, 0x8e, 0xd7, 0xa8, 0x00, 0x00,
            0x00, 0x62, 0x00, 0x00, 0x05, 0xf9, 0x8e, 0x00, 0x00, 0x59, 0x27, 0x5e,
            0x18, 0xf0, 0xd2, 0xac, 0x2a, 0x52, 0x5e, 0x1e, 0x4d, 0x1e, 0x98, 0xda,
            0xf1, 0x01, 0x28, 0xe4, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x34,
            0x29, 0x00, 0x00, 0x5f, 0x19, 0x57, 0x2b, 0xd6, 0x45, 0xb5, 0x4d, 0x71,
            0xbe, 0x89, 0x27, 0xf0, 0x69, 0x29, 0xc0, 0x9f, 0x7f, 0xd1, 0x00, 0x00,
            0x00, 0x69, 0x00, 0x00, 0x04, 0x00, 0x40, 0x00, 0x00, 0x5f, 0x1e, 0x9a,
            0xd4, 0x59, 0xb7, 0x8b, 0x9e, 0xe0, 0x32, 0x3b, 0x5e, 0x8b, 0xfa, 0x40,
            0xa3, 0xcb, 0x12, 0x40, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x0d, 0xdb,
            0x2d, 0x00, 0x00, 0x61, 0x1f, 0xe7, 0xd7, 0xe8, 0x7d, 0x3c, 0x81, 0x76,
            0xb3, 0xb0, 0xa1, 0x1f, 0xbc, 0xd6, 0xbc, 0x90, 0xfe, 0x1f, 0x00, 0x00,
            0x00, 0x7c, 0x00, 0x00, 0x00, 0xb7, 0xb2, 0x00, 0x00, 0x62, 0x5c, 0x6b,
            0x4e, 0x2b, 0xfb, 0x1f, 0xe9, 0xa6, 0x7b, 0x55, 0x1d, 0x33, 0xb7, 0xb7,
            0xed, 0xaf, 0x66, 0xb0, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x02, 0x00,
            0x40, 0x00, 0x00, 0x62, 0x6b, 0x4f, 0x4d, 0x6c, 0x47, 0x2e, 0xad, 0x3b,
            0xd1, 0x2a, 0x2e, 0x48, 0x6c, 0x91, 0x0a, 0xc1, 0xaf, 0xa9, 0x00, 0x00,
            0x00, 0x80, 0x00, 0x00, 0x04, 0x50, 0x39, 0x00, 0x00, 0x63, 0x6c, 0x1a,
            0xe2, 0x4a, 0x46, 0x8e, 0x1f, 0xf8, 0xaa, 0x4b, 0x80, 0x99, 0x03, 0x21,
            0x12, 0xf9, 0xe3, 0xfa, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x43,
            0xd3, 0x00, 0x00, 0x63, 0xc8, 0x55, 0xb8, 0x86, 0x02, 0x51, 0xe5, 0xdd,
            0x4e, 0xd4, 0x2b, 0xdd, 0x5c, 0xdf, 0x53, 0x98, 0x50, 0x88, 0x00, 0x00,
            0x00, 0x86, 0x00, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00, 0x63, 0xd2, 0xad,
            0x57, 0x4c, 0x32, 0x85, 0x41, 0xf7, 0x8c, 0x6d, 0x8f, 0xb3, 0x17, 0x4a,
            0x60, 0xb2, 0xfa, 0x72, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x1b,
            0xf2, 0x00, 0x00, 0x63, 0xe1, 0xd7, 0x55, 0x6b, 0x95, 0x4e, 0x44, 0xab,
            0xf2, 0x8a, 0xe6, 0x19, 0x8b, 0xa8, 0x2c, 0x8a, 0x60, 0x0d, 0x00, 0x00,
            0x00, 0x88, 0x00, 0x00, 0x00, 0x08, 0x34, 0x00, 0x00, 0x63, 0xe7, 0x0a,
            0x71, 0x6c, 0x5b, 0x19, 0x40, 0x7b, 0xc2, 0xd9, 0x61, 0x28, 0x60, 0x7e,
            0x9c, 0xbe, 0x15, 0x39, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x0e,
            0xdd, 0x00, 0x00, 0x63, 0xe9, 0xb0, 0xaa, 0xd3, 0x03, 0x33, 0xef, 0x48,
            0xc6, 0x31, 0x90, 0xc1, 0xe8, 0x9d, 0xad, 0x49, 0x39, 0x59, 0x00, 0x00,
            0x00, 0x8a, 0x00, 0x00, 0x00, 0x0f, 0xbf, 0x00, 0x00, 0x63, 0xec, 0xec,
            0xe0, 0xb9, 0xde, 0xfb, 0x72, 0x19, 0x4e, 0x06, 0xa1, 0x0d, 0x6b, 0x4f,
            0x06, 0x4f, 0xe9, 0x86, 0x00, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x12,
            0xc3, 0x00, 0x00, 0x63, 0xef, 0xc6, 0x01, 0x27, 0x05, 0xa0, 0x13, 0xbe,
            0x47, 0xc3, 0x00, 0xd0, 0x01, 0x7d, 0x00, 0x88, 0x00, 0x06, 0xc1, 0x5d,
            0xf8, 0x81, 0xff, 0xdb, 0xff, 0xb4, 0xff, 0xba, 0xff, 0x55, 0xfe, 0x8e,
            0xfe, 0x59, 0xff, 0x4b, 0xfe, 0xbb, 0xfe, 0xbe, 0xff, 0x72, 0xff, 0xbe,
            0xff, 0xe4, 0xff, 0xc7, 0xff, 0xf5, 0xff, 0xd4, 0xff, 0xe8, 0xff, 0xce,
            0xff, 0x71, 0xfe, 0xf3, 0xff, 0x02, 0xfe, 0xd0, 0xff, 0x95, 0xfe, 0xfd,
            0xff, 0xca, 0xff, 0x44, 0xfe, 0xac, 0xff, 0x9b, 0xff, 0xc9, 0xff, 0xb5,
            0xff, 0xee, 0xff, 0xb3, 0xff, 0xbd, 0xff, 0xc0, 0xff, 0xcb, 0xff, 0xef,
            0xff, 0xea, 0xff, 0xf7, 0xff, 0xf8, 0xff, 0xda, 0xff, 0x80, 0xff, 0xd2,
            0xff, 0xad, 0xff, 0xbb, 0xff, 0xd0, 0xff, 0xe0, 0x00, 0x00, 0xff, 0xe5,
            0xff, 0xe1, 0xff, 0xa6, 0xff, 0xd1, 0xff, 0x9c, 0xff, 0xb9, 0xff, 0xc3,
            0xff, 0xcc, 0xff, 0xc3, 0xff, 0xe5, 0xff, 0xf8, 0xff, 0xfb, 0x00, 0x00,
            0x00, 0x00, 0xff, 0xe2, 0xff, 0xe5, 0xff, 0x10, 0xff, 0xd9, 0xff, 0x16,
            0xff, 0x9f, 0xff, 0xa3, 0xff, 0x17, 0xff, 0xe8, 0x00, 0x00, 0xff, 0xbd,
            0xff, 0xd5, 0xff, 0x7e, 0xff, 0xd1, 0xff, 0xd9, 0xff, 0xcf, 0xff, 0xbc,
            0xff, 0xef, 0xff, 0xd6, 0xff, 0x5f, 0xff, 0x7a, 0xff, 0xe4, 0xff, 0x74,
            0xff, 0xd0, 0xff, 0xbf, 0xff, 0xb9, 0x5c, 0x1e, 0xc7, 0xa0, 0xfe, 0x69,
            0xfe, 0x65, 0xff, 0x0f, 0xfe, 0xdc, 0xfe, 0xdb, 0xf8, 0x65, 0x05, 0x43,
            0x00, 0x00, 0x00, 0x00, 0x00, 0xed, 0x00, 0x54, 0x00, 0x0c, 0x1e, 0xb3,
            0x15, 0x2b, 0x18, 0x68, 0x0f, 0x4d, 0x19, 0x09, 0x17, 0xc3, 0x17, 0xb1,
            0x13, 0xdb, 0x1a, 0xa5, 0x12, 0x42, 0x1a, 0xdd, 0x17, 0xf1, 0x12, 0x9b,
            0x12, 0x49, 0x0e, 0xe4, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x0c, 0x1f, 0x78,
            0x10, 0x50, 0x11, 0x2f, 0x13, 0x6d, 0x07, 0xd7, 0x0a, 0x58, 0x0f, 0x2a,
            0x05, 0x33, 0x02, 0xa6, 0x03, 0x3c, 0x02, 0xda, 0x02, 0x2f
    };
    static const size_t plainText_len = 1030;


  u8 plainText[plainText_len];

  Rijndael rijndael;
  rijndael.MakeKey(psarcKey);
  rijndael.Decrypt(encrypted_data, plainText, plainText_len);

  for (size_t i = 0; i < plainText_len; ++i)
    ASSERT(plainText[i] == plainText_data[i]);
}

#ifdef TEST_BUILD
int main(int argc, char* argv[])
{
    rijndaelTest();

  return 0;
}
#endif // TEST_BUILD