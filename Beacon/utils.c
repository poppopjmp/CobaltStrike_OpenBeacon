#include "pch.h"

#include "utils.h"

DWORD ExpandEnvironmentStrings_s(const char* lpSrc, char* lpDst, size_t size) {
	// determine the size of the buffer required to store the expanded string
	DWORD nSize = ExpandEnvironmentStringsA(lpSrc, NULL, 0);

	// if the size of the buffer is too small, return 0
	if (nSize == 0 || size <= nSize + 1) {
		return 0;
	}

	// expand the string
	return ExpandEnvironmentStringsA(lpSrc, lpDst, size);
}

int RoundToNearestMultiple(int value, int multiple)
{
	return value - value % multiple;
}

int RoundToNearestEven(int value)
{
	return RoundToNearestMultiple(value, 2);
}

int RandomInt(void)
{
	int out;
	rng_get_bytes((unsigned char*)&out, sizeof(out), NULL);
	return out;
}

int RandomEvenInt(void)
{
	return RoundToNearestEven(RandomInt());
}