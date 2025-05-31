#include <algorithm>

#define NOMINMAX
#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include "helpful.hpp"

std::byte* get_image_base() noexcept
{
    return reinterpret_cast<std::byte*>(
            *reinterpret_cast<usize*>(
                0x10 + reinterpret_cast<std::byte*>(reinterpret_cast<PPEB>(__readgsqword(0x60)))));
}

std::byte* get_rva(usize file_offset) noexcept
{
    return get_image_base() + 0x1000 + (file_offset - 0x400);
}

std::byte* malloc(usize size, void* nearby) noexcept
{
    auto const current_process = ::GetCurrentProcess();

    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    const usize PAGE_SIZE = sysInfo.dwPageSize;

    usize startAddr = (usize(nearby) & ~(PAGE_SIZE - 1)); // round down to nearest page boundary
    usize minAddr = std::min(startAddr - 0x7FFFFF00, (usize)sysInfo.lpMinimumApplicationAddress);
    usize maxAddr = std::max(startAddr + 0x7FFFFF00, (usize)sysInfo.lpMaximumApplicationAddress);

    usize startPage = (startAddr - (startAddr % PAGE_SIZE));

    usize pageOffset = 1;
    while (true)
    {
        usize byteOffset = pageOffset * PAGE_SIZE;
        usize highAddr = startPage + byteOffset;
        usize lowAddr = (startPage > byteOffset) ? startPage - byteOffset : 0;

        bool needsExit = highAddr > maxAddr && lowAddr < minAddr;

        if (highAddr < maxAddr)
        {
            void* outAddr = ::VirtualAllocEx(current_process, (void*)highAddr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (outAddr)
            {
                return reinterpret_cast<std::byte*>(outAddr);
            }
        }

        if (lowAddr > minAddr)
        {
            void* outAddr = ::VirtualAllocEx(current_process, (void*)lowAddr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (outAddr)
            {
                return reinterpret_cast<std::byte*>(outAddr);
            }
        }

        pageOffset++;

        if (needsExit)
        {
            break;
        }
    }

    return nullptr;
}
