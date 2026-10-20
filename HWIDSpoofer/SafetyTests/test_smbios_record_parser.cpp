#include "smbios_record_parser.hpp"

#include <array>
#include <cstdint>
#include <iostream>

using safety_tests::ParseResult;
using safety_tests::SmbiosRecordView;
using safety_tests::parse_smbios_record;

namespace {
int failures = 0;

#define CHECK(expression) do { \
    if (!(expression)) { \
        std::cerr << "FAIL line " << __LINE__ << ": " #expression "\n"; \
        ++failures; \
    } \
} while (false)
}

int main() {
    SmbiosRecordView view{};

    const std::array<std::uint8_t, 8> valid{
        1, 4, 0, 0, 'X', 0, 0, 0
    };
    CHECK(parse_smbios_record(valid.data(), 7, view) == ParseResult::record);
    CHECK(view.type == 1);
    CHECK(view.formatted_length == 4);
    CHECK(view.total_length == 7);
    CHECK(view.strings_length == 1);

    CHECK(parse_smbios_record(nullptr, 0, view) == ParseResult::invalid);
    CHECK(view.formatted == nullptr);
    CHECK(parse_smbios_record(valid.data(), 3, view) == ParseResult::invalid);

    const std::array<std::uint8_t, 6> short_length{1, 3, 0, 0, 0, 0};
    CHECK(parse_smbios_record(short_length.data(), short_length.size(), view)
          == ParseResult::invalid);
    const std::array<std::uint8_t, 6> oversized_length{1, 9, 0, 0, 0, 0};
    CHECK(parse_smbios_record(oversized_length.data(), oversized_length.size(), view)
          == ParseResult::invalid);

    const std::array<std::uint8_t, 6> no_terminator{1, 4, 0, 0, 'A', 'B'};
    CHECK(parse_smbios_record(no_terminator.data(), no_terminator.size(), view)
          == ParseResult::invalid);

    const std::array<std::uint8_t, 6> end_record{127, 4, 0, 0, 0, 0};
    CHECK(parse_smbios_record(end_record.data(), end_record.size(), view)
          == ParseResult::end_of_table);

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All bounded-parser tests passed.\n";
    return 0;
}
