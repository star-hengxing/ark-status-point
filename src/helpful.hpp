#pragma once

#include <cstddef>

#include "base/basic_type.hpp"

std::byte* get_image_base() noexcept;

std::byte* get_rva(usize file_offset) noexcept;

std::byte* malloc(usize size, void* nearby) noexcept;
