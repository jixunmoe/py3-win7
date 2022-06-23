#pragma once
#include <cstdint>

template<typename T>
void WriteMem(uintptr_t address, T value)
{
	DWORD oldProtect;
	VirtualProtect(LPVOID(address), sizeof T, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<T *>(address) = value;
	VirtualProtect(LPVOID(address), sizeof T, oldProtect, &oldProtect);
}

template<typename T>
void WriteMemRaw(uintptr_t address, T value)
{
	*reinterpret_cast<T *>(address) = value;
}

inline void WriteRelativeAddress(uintptr_t address, uintptr_t content)
{
	WriteMem<uintptr_t>(address, content - address - 4);
}

inline uintptr_t ReadRelativeAddress(uintptr_t address)
{
	// 00007FFD612A133E | E9 ED 0E 00 00 | jmp 7FFD612A2230
	// 7FFD612A2230 - 00007FFD612A133E - 5 = 0E ED

	return address + 4 + *reinterpret_cast<uint32_t*>(address);
}

inline DWORD GetWritableRelativeAddress(DWORD operand_address, DWORD target_address) {
	return target_address - operand_address - 4;
}

inline void WriteRelativeAddressAtPayload(uint8_t* payload, size_t payload_offset, uintptr_t payload_mapped_address, uintptr_t target_address)
{
	// 00D8020D | E8 CE86D474              | call 0x75AC88E0 ; <kernel32.CreateProcessW>                                                  |
	WriteMem(uintptr_t(payload + payload_offset), GetWritableRelativeAddress(DWORD(payload_mapped_address + payload_offset), DWORD(target_address)));
}
