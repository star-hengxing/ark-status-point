#include <string_view>

#include <windows.h>
#include <detours.h>

#include <fmt/xchar.h>

#include "base/basic_type.hpp"
#include "helpful.hpp"
#include "ark.hpp"

std::byte* status_string_address1;
std::byte* status_string_address2;

NAMESPACE_BEGIN(original)

constexpr std::wstring_view status_string1 = L"%.1f / ";
constexpr std::wstring_view status_string2 = L"%.1f %%";

using fn_IsPrimalDino = bool(__fastcall*)(void* self);

static void(__fastcall* GetStatusValueString)(void* self, void* result, int ValueType, void* bValueOnly);
static auto const GetPrimalCharacter = reinterpret_cast<void*(__fastcall*)(void* self)>(get_rva(0x5A0BD0));

NAMESPACE_END(original)

NAMESPACE_BEGIN(hook)

// constexpr std::wstring_view person_status_string1 = L"254 | %.1f / ";
// constexpr std::wstring_view person_status_string2 = L"254 | %.1f %%";

constexpr std::wstring_view dino_status_string1 = L"254 + 254 | %.1f / ";
constexpr std::wstring_view dino_status_string2 = L"254 + 254 | %.1f %%";

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

    auto const AActor = original::GetPrimalCharacter(self);
    auto const AActor_vtable_address = *reinterpret_cast<usize*>(AActor);
    auto const AActor_IsPrimalDino_address = *reinterpret_cast<usize*>(AActor_vtable_address + 0x860);
    auto const IsPrimalDino = reinterpret_cast<original::fn_IsPrimalDino>(AActor_IsPrimalDino_address);
    if (IsPrimalDino(AActor))
    {
        auto const NumberOfLevelUpPointsAppliedTamed = reinterpret_cast<u8*>(reinterpret_cast<std::byte*>(self) + 0x144);
        auto const tame_point = NumberOfLevelUpPointsAppliedTamed[ValueType];
        if (ValueType < 8)
        {
            auto str = reinterpret_cast<wchar_t*>(status_string_address1);
            auto end = fmt::format_to(str, L"{} + {} | %.1f / ", point, tame_point);
            *end = '\0';
        }
        else
        {
            auto str = reinterpret_cast<wchar_t*>(status_string_address2);
            auto end = fmt::format_to(str, L"{} + {} | %.1f %%", point, tame_point);
            *end = '\0';
        }
    }
    else
    {
        if (ValueType < 8)
        {
            auto str = reinterpret_cast<wchar_t*>(status_string_address1);
            auto end = fmt::format_to(str, L"{} | %.1f / ", point);
            *end = '\0';
        }
        else
        {
            auto str = reinterpret_cast<wchar_t*>(status_string_address2);
            auto end = fmt::format_to(str, L"{} | %.1f %%", point);
            *end = '\0';
        }
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
        status_string_address1 = malloc((hook::dino_status_string1.size() + 1) * 2, target_address);
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
        status_string_address2 = malloc((hook::dino_status_string2.size() + 1) * 2, target_address);
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
