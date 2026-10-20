#include "smbios_record_parser.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

using safety_tests::ParseResult;
using safety_tests::SmbiosRecordView;
using safety_tests::parse_smbios_record;

int main() {
    SmbiosRecordView view{};

    // Valid type-1-like record: 4-byte formatted section, one string, NUL NUL.
    const std::array<std::uint8_t, 8> valid{
        1, 4, 0, 0, 'X', 0, 0, 0
    };
    assert(parse_smbios_record(valid.data(), 7, view) == ParseResult::record);
    assert(view.type == 1);
    assert(view.formatted_length == 4);
    assert(view.total_length == 7);

    // Null and truncated headers must be rejected.
    assert(parse_smbios_record(nullptr, 0, view) == ParseResult::invalid);
    assert(parse_smbios_record(valid.data(), 3, view) == ParseResult::invalid);

    // Formatted length must include the header and remain within the span.
    const std::array<std::uint8_t, 6> short_length{1, 3, 0, 0, 0, 0};
    assert(parse_smbios_record(short_length.data(), short_length.size(), view)
           == ParseResult::invalid);
    const std::array<std::uint8_t, 6> oversized_length{1, 9, 0, 0, 0, 0};
    assert(parse_smbios_record(oversized_length.data(), oversized_length.size(), view)
           == ParseResult::invalid);

    // Missing double-NUL string terminator must be rejected.
    const std::array<std::uint8_t, 6> no_terminator{1, 4, 0, 0, 'A', 'B'};
    assert(parse_smbios_record(no_terminator.data(), no_terminator.size(), view)
           == ParseResult::invalid);

    // Type 127 is recognized as the end-of-table record.
    const std::array<std::uint8_t, 6> end_record{127, 4, 0, 0, 0, 0};
    assert(parse_smbios_record(end_record.data(), end_record.size(), view)
           == ParseResult::end_of_table);

    std::cout << "All bounded-parser tests passed.\n";
    return 0;
}
