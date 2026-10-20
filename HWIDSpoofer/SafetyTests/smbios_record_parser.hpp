#pragma once

#include <cstddef>
#include <cstdint>

namespace safety_tests {

struct SmbiosRecordView {
    std::uint8_t type{};
    const std::uint8_t* formatted{};
    std::size_t formatted_length{};
    const std::uint8_t* strings{};
    std::size_t strings_length{};
    std::size_t total_length{};
};

enum class ParseResult {
    record,
    end_of_table,
    invalid
};

// Parses one SMBIOS structure from a caller-owned byte span.
// This standalone helper is for synthetic-data tests and is not wired into
// the HWID-changing or interception code.
inline ParseResult parse_smbios_record(
    const std::uint8_t* data,
    std::size_t size,
    SmbiosRecordView& out) noexcept
{
    out = {};
    if (data == nullptr || size < 4) {
        return ParseResult::invalid;
    }

    const std::size_t formatted_length = data[1];
    if (formatted_length < 4 || formatted_length > size) {
        return ParseResult::invalid;
    }

    // The string-set terminator must fit entirely inside the provided span.
    std::size_t terminator = formatted_length;
    bool found = false;
    while (terminator + 1 < size) {
        if (data[terminator] == 0 && data[terminator + 1] == 0) {
            found = true;
            break;
        }
        ++terminator;
    }
    if (!found) {
        return ParseResult::invalid;
    }

    out.type = data[0];
    out.formatted = data;
    out.formatted_length = formatted_length;
    out.strings = data + formatted_length;
    out.strings_length = terminator - formatted_length;
    out.total_length = terminator + 2;

    return out.type == 127 ? ParseResult::end_of_table : ParseResult::record;
}

} // namespace safety_tests
