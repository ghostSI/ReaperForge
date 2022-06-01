#include "base64.h"

#include <vector>

static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char padChar = '=';

std::string Base64::encode(const u8* in, u64 len)
{
	std::string out;
	out.reserve(((len / 3) + (len % 3 > 0)) * 4);
	u32 temp;
	for (size_t i = 0; i < len / 3; ++i)
	{
		temp = (*in++) << 16;
		temp += (*in++) << 8;
		temp += (*in++);
		out += chars[(temp & 0x00FC0000) >> 18];
		out += chars[(temp & 0x0003F000) >> 12];
		out += chars[(temp & 0x00000FC0) >> 6];
		out += chars[(temp & 0x0000003F)];
	}
	switch (len % 3)
	{
	case 1:
		temp = (*in++) << 16;
		out += chars[(temp & 0x00FC0000) >> 18];
		out += chars[(temp & 0x0003F000) >> 12];
		out += padChar;
		out += padChar;
		break;
	case 2:
		temp = (*in++) << 16;
		temp += (*in++) << 8;
		out += chars[(temp & 0x00FC0000) >> 18];
		out += chars[(temp & 0x0003F000) >> 12];
		out += chars[(temp & 0x00000FC0) >> 6];
		out += padChar;
		break;
	}
	return out;
}

u64 Base64::decode(const std::string& in, u8* out)
{
	assert(in.size() % 4 == 0);

	u8* p = out;

	u64 padding = 0;
	if (in.size())
	{
		if (in[in.size() - 1] == padChar)
			padding++;
		if (in[in.size() - 2] == padChar)
			padding++;
	}
	u32 temp = 0;
	i32 i = 0;
	while (i < in.size())
	{
		for (u64 quantumPosition = 0; quantumPosition < 4; ++quantumPosition)
		{
			temp <<= 6;
			if (in[i] >= 0x41 && in[i] <= 0x5A)
				temp |= in[i] - 0x41;
			else if (in[i] >= 0x61 && in[i] <= 0x7A)
				temp |= in[i] - 0x47;
			else if (in[i] >= 0x30 && in[i] <= 0x39)
				temp |= in[i] + 0x04;
			else if (in[i] == 0x2B)
				temp |= 0x3E;
			else if (in[i] == 0x2F)
				temp |= 0x3F;
			else if (in[i] == padChar)
			{
				switch (in.size() - i)
				{
				case 1:
					*p++ = (temp >> 16) & 0x000000FF;
					*p++ = (temp >> 8) & 0x000000FF;
					return p - out;
				case 2:
					*p++ = (temp >> 10) & 0x000000FF;
					return p - out;
				default:
					assert(false);
				}
			}
			else
				assert(false);
			i++;
		}
		*p++ = (temp >> 16) & 0x000000FF;
		*p++ = (temp >> 8) & 0x000000FF;
		*p++ = temp & 0x000000FF;
	}
	return p - out;
}
