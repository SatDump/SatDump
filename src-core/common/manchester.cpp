#include "manchester.h"

// G. E. Thomas Manchester decoder
uint8_t manchester_decode(uint8_t partOne, uint8_t partTwo)
{
	uint8_t data = 0x00;
	uint8_t temp = 0x00;
	uint8_t bitCount = 0, bitOneCount = 0, bitTwoCount = 0, bitByOneCount = 1, bitByTwoCount = 1, firstHalfCount = 0;

	while (bitCount != 8)
	{

		if (firstHalfCount <= 6)
		{
			temp |= ((partOne >> (bitOneCount + bitByOneCount)) & 0x01);

			if (temp == 0x01)
			{
				data |= (temp << bitCount);
			}

			if (temp == 0x00)
			{
				data |= (temp << bitCount);
			}

			bitCount++;
			bitOneCount++;
			bitByOneCount++;
		}
		else
		{
			temp |= ((partTwo >> (bitTwoCount + bitByTwoCount)) & 0x01);

			if (temp == 0x01)
			{
				data |= (temp << bitCount);
			}

			if (temp == 0x00)
			{
				data |= (temp << bitCount);
			}

			bitCount++;
			bitTwoCount++;
			bitByTwoCount++;
		}

		firstHalfCount = firstHalfCount + 2;
		temp = 0x00;
	}
	return data;
}

int manchesterDecoder(uint8_t *in, int length, uint8_t *out)
{
	for (int i = 0; i < length; i += 2)
	{
		out[i / 2] = manchester_decode(in[i + 1], in[i]);
	}
	return length / 2;
}