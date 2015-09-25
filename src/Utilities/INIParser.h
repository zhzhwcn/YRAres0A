#pragma once

#include "..\Ares.h"
#include "Parser.h"

#include <CCINIClass.h>

class INI_EX {
	CCINIClass* pINI;

	size_t buflen() {
		return Ares::readLength;
	}

public:
	INI_EX(CCINIClass *iniFile)
	 : pINI(iniFile)
	{};

	char* value() const {
		return Ares::readBuffer;
	}

	bool empty() const {
		return !Ares::readBuffer[0];
	}

	// basic string reader
	size_t ReadString(const char* pSection, const char* pKey) {
		auto const res = pINI->ReadString(
			pSection, pKey, "", this->value(), this->buflen());
		return static_cast<size_t>(res);
	}

	// parser template
	template <typename T, size_t Count>
	bool Read(const char* pSection, const char* pKey, T* pBuffer) {
		if(this->ReadString(pSection, pKey)) {
			return Parser<T, Count>::Parse(this->value(), pBuffer) == Count;
		}
		return false;
	}

	// helpers

	bool ReadBool(const char* pSection, const char* pKey, bool* bBuffer) {
		return Read<bool, 1>(pSection, pKey, bBuffer);
	}

	bool ReadInteger(const char* pSection, const char* pKey, int* nBuffer) {
		return Read<int, 1>(pSection, pKey, nBuffer);
	}

	bool Read2Integers(const char* pSection, const char* pKey, int* nBuffer) {
		return Read<int, 2>(pSection, pKey, nBuffer);
	}

	bool Read3Integers(const char* pSection, const char* pKey, int* nBuffer) {
		return Read<int, 3>(pSection, pKey, nBuffer);
	}

	bool Read4Integers(const char* pSection, const char* pKey, int* nBuffer) {
		return Read<int, 4>(pSection, pKey, nBuffer);
	}

	bool Read3Bytes(const char* pSection, const char* pKey, byte* nBuffer) {
		return Read<byte, 3>(pSection, pKey, nBuffer);
	}

	bool ReadDouble(const char* pSection, const char* pKey, double* nBuffer) {
		return Read<double, 1>(pSection, pKey, nBuffer);
	}
};
