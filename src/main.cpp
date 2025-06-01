#include <string_view>

#include <windows.h>
#include <detours.h>

#include "helpful.hpp"
#include "ark.hpp"

std::byte* status_string_address1; // L"%.1f / "
std::byte* status_string_address2; // L"%.1f %%"

static void u8_to_str(u8 value, wchar_t* string) noexcept
{
    if (value < 10)
    {
        string[0] = ' ';
        string[1] = ' ';
        string[2] = '0' + value % 10;
    }
    else if (value < 100)
    {
        string[0] = ' ';
        string[1] = '0' + (value / 10) % 10;
        string[2] = '0' + value % 10;
    }
    else
    {
        string[0] = '0' + (value / 100) % 10;
        string[1] = '0' + (value / 10) % 10;
        string[2] = '0' + value % 10;
    }
}

NAMESPACE_BEGIN(original)

constexpr std::wstring_view status_string1 = L"%.1f / ";
constexpr std::wstring_view status_string2 = L"%.1f %%";

static void(__fastcall* GetStatusValueString)(void* self, void* result, int ValueType, void* bValueOnly);

NAMESPACE_END(original)

NAMESPACE_BEGIN(hook)

constexpr std::wstring_view status_string1 = L"254 | %.1f / ";
constexpr std::wstring_view status_string2 = L"254 | %.1f %%";

void __fastcall GetStatusValueString(void* self, void* result, int ValueType, void* bValueOnly)
{
    ark::Status status;
    switch (static_cast<ark::Status>(ValueType))
    {
        using enum ark::Status;
        case health:
        case stamina:
        // case torpor:
        case oxygen:
        case food:
        case water:
        case weight:
        case melee_damage:
        case movement_speed:
        case fortitude:
        case crafting_speed:
        {
            status = static_cast<ark::Status>(ValueType);
            break;
        }
    default:
        status = none;
        break;
    }

    if (!self || status == ark::Status::none)
    {
        if (ValueType < 8)
        {
            std::memcpy(status_string_address1, original::status_string1.data(), (original::status_string1.size() + 1) * sizeof(wchar_t));
        }
        else
        {
            std::memcpy(status_string_address2, original::status_string2.data(), (original::status_string2.size() + 1) * sizeof(wchar_t));
        }
        original::GetStatusValueString(self, result, ValueType, bValueOnly);
        return;
    }

    auto const NumberOfLevelUpPointsApplied = reinterpret_cast<u8*>(reinterpret_cast<std::byte*>(self) + 0x138);
    auto const point = NumberOfLevelUpPointsApplied[ValueType];

    if (ValueType < 8)
    {
        auto str = reinterpret_cast<wchar_t*>(status_string_address1);
        std::memcpy(status_string_address1, status_string1.data(), (status_string1.size() + 1) * sizeof(wchar_t));
        u8_to_str(point, str);
    }
    else
    {
        auto str = reinterpret_cast<wchar_t*>(status_string_address2);
        std::memcpy(status_string_address2, status_string2.data(), (status_string2.size() + 1) * sizeof(wchar_t));
        u8_to_str(point, str);
    }

    original::GetStatusValueString(self, result, ValueType, bValueOnly);
}

NAMESPACE_END(hook)

// FString::Printf__VA
bool patch_constant_string(std::byte* address, const std::byte* string)
{
    auto const current_process = ::GetCurrentProcess();
    DWORD lpflOldProtect;
    if (!::VirtualProtectEx(current_process, address, 7, PAGE_EXECUTE_READWRITE, &lpflOldProtect))
    {
        return false;
    }
    // lea rdx, [rip + offset]
    // 48 8D 15
    auto cur_rip = address + 7;
    usize offset = string - cur_rip;

    usize bytesWritten;
    BOOL result = ::WriteProcessMemory(current_process, address + 3, &offset, 4, &bytesWritten);
    ::VirtualProtectEx(current_process, address, 7, lpflOldProtect, &lpflOldProtect);

    return result == TRUE && bytesWritten == 4;
}

bool patch()
{
    {
        auto const target_address = get_rva(0x5A7C09);
        status_string_address1 = malloc((hook::status_string1.size() + 1) * 2, target_address);
        if (!status_string_address1)
        {
    #ifdef PROJECT_DEBUG
            MessageBoxA(
                nullptr,
                "Failed to allocate memory for string patch.",
                "Error",
                MB_OK | MB_ICONERROR);
    #endif
            return false;
        }
    
        patch_constant_string(target_address, status_string_address1);
        // std::memcpy(status_string_address1, string.data(), size);
    }

    {
        auto const target_address = get_rva(0x5A7A53);
        status_string_address2 = malloc((hook::status_string2.size() + 1) * 2, target_address);
        if (!status_string_address2)
        {
    #ifdef PROJECT_DEBUG
            MessageBoxA(
                nullptr,
                "Failed to allocate memory for string patch.",
                "Error",
                MB_OK | MB_ICONERROR);
    #endif
            return false;
        }
    
        patch_constant_string(target_address, status_string_address2);
    }
    return true;
}

static void attach() noexcept
{
    original::GetStatusValueString = reinterpret_cast<decltype(original::GetStatusValueString)>(get_rva(0x5A7710));

    auto src = std::addressof(reinterpret_cast<PVOID&>(original::GetStatusValueString));
    auto dst = reinterpret_cast<PVOID>(hook::GetStatusValueString);
    ::DetourAttach(src, dst);
}

static void detach() noexcept
{
    auto src = std::addressof(reinterpret_cast<PVOID&>(original::GetStatusValueString));
    auto dst = reinterpret_cast<PVOID>(hook::GetStatusValueString);
    ::DetourDetach(src, dst);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (!ark::is_running())
    {
        return TRUE;
    }

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
#ifdef PROJECT_DEBUG
        MessageBoxA(
            nullptr,
            "DLL_PROCESS_ATTACH",
            "",
            MB_OK | MB_ICONINFORMATION);
#endif

        if (!patch())
        {
            return TRUE;
        }

        ::DetourRestoreAfterWith();

        ::DetourTransactionBegin();
        ::DetourUpdateThread(::GetCurrentThread());

        attach();

        ::DetourTransactionCommit();
    }
    // else if (fdwReason == DLL_PROCESS_DETACH)
    // {
    //     ::DetourTransactionBegin();
    //     ::DetourUpdateThread(::GetCurrentThread());

    //     detach();

    //     ::DetourTransactionCommit();
    // }
    return TRUE;
}
