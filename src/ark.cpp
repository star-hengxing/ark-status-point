#include <string_view>

#include <windows.h>

#include "ark.hpp"

NAMESPACE_BEGIN(ark)

bool is_running() noexcept
{
    char buffer[MAX_PATH];
    DWORD size = ::GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (size > 0)
    {
        std::string_view program_name{buffer, size};
        usize last_slash_index = program_name.find_last_of("\\/");
        if (last_slash_index != std::string_view::npos)
        {
            program_name.remove_prefix(last_slash_index + 1);
        }

        if (!program_name.empty() && program_name == "ShooterGame.exe")
        {
            return true;
        }
    }
    return false;
}

NAMESPACE_END(ark)
