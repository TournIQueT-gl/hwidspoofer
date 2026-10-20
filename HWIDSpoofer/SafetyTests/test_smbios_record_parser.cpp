#include "smbios_record_parser.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

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

void check_invalid_clears_view(const std::uint8_t* data, std::size_t size) {
    SmbiosRecordView view{1, data, 2, data, 3, 4};
    CHECK(parse_smbios_record(data, size, view) == ParseResult::invalid);
    CHECK(view.type == 0);
    CHECK(view.formatted == nullptr);
    CHECK(view.formatted_length == 0);
    CHECK(view.strings == nullptr);
    CHECK(view.strings_length == 0);
    CHECK(view.total_length == 0);
}
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

    check_invalid_clears_view(nullptr, 0);
    CHECK(parse_smbios_record(valid.data(), 3, view) == ParseResult::invalid);

    const std::array<std::uint8_t, 6> short_length{1, 3, 0, 0, 0, 0};
    check_invalid_clears_view(short_length.data(), short_length.size());
    const std::array<std::uint8_t, 6> oversized_length{1, 9, 0, 0, 0, 0};
    check_invalid_clears_view(oversized_length.data(), oversized_length.size());

    const std::array<std::uint8_t, 6> no_terminator{1, 4, 0, 0, 'A', 'B'};
    check_invalid_clears_view(no_terminator.data(), no_terminator.size());

    const std::array<std::uint8_t, 6> end_record{127, 4, 0, 0, 0, 0};
    CHECK(parse_smbios_record(end_record.data(), end_record.size(), view)
          == ParseResult::end_of_table);
    CHECK(view.type == 127);
    CHECK(view.total_length == end_record.size());

    // Deterministic malformed-input sweep: every byte value, short spans,
    // and changing header lengths. Returned views must stay within the span.
    for (std::size_t size = 0; size <= 64; ++size) {
        for (unsigned value = 0; value <= 255; ++value) {
            std::array<std::uint8_t, 64> bytes{};
            bytes.fill(static_cast<std::uint8_t>(value));
            if (size >= 2) {
                bytes[0] = static_cast<std::uint8_t>(value);
                bytes[1] = static_cast<std::uint8_t>((value + size) & 0xffU);
            }
            const auto result = parse_smbios_record(bytes.data(), size, view);
            if (result == ParseResult::invalid) {
                CHECK(view.formatted == nullptr);
                CHECK(view.strings == nullptr);
                CHECK(view.total_length == 0);
                continue;
            }
            CHECK(view.formatted == bytes.data());
            CHECK(view.formatted_length >= 4);
            CHECK(view.formatted_length <= size);
            CHECK(view.total_length >= view.formatted_length);
            CHECK(view.total_length <= size);
            CHECK(view.strings == bytes.data() + view.formatted_length);
            CHECK(view.strings_length <= view.total_length - view.formatted_length);
            if (result == ParseResult::end_of_table) {
                CHECK(view.type == 127);
            } else {
                CHECK(view.type != 127);
            }
        }
    }

    // Reproducible pseudo-random fuzzing over arbitrary byte spans. This
    // intentionally uses no system SMBIOS data and cannot modify the machine.
    std::uint32_t state = 0xC0FFEEU;
    for (std::size_t iteration = 0; iteration < 100000; ++iteration) {
        state = state * 1664525U + 1013904223U;
        const std::size_t size = static_cast<std::size_t>((state >> 24U) % 65U);
        std::array<std::uint8_t, 64> bytes{};
        for (std::size_t i = 0; i < size; ++i) {
            state = state * 1664525U + 1013904223U;
            bytes[i] = static_cast<std::uint8_t>(state >> 24U);
        }
        const auto result = parse_smbios_record(bytes.data(), size, view);
        if (result == ParseResult::invalid) {
            CHECK(view.formatted == nullptr);
            CHECK(view.strings == nullptr);
            CHECK(view.total_length == 0);
        } else {
            CHECK(view.formatted == bytes.data());
            CHECK(view.formatted_length >= 4 && view.formatted_length <= size);
            CHECK(view.total_length >= view.formatted_length && view.total_length <= size);
            CHECK(view.strings == bytes.data() + view.formatted_length);
            CHECK(view.strings_length <= view.total_length - view.formatted_length);
            CHECK((result == ParseResult::end_of_table) == (view.type == 127));
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All bounded-parser tests passed.\n";
    return 0;
}
