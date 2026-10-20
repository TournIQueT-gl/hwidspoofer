#pragma once

#include <charconv>
#include <cctype>
#include <cstdint>
#include <string>

namespace config_value_parser {

// Extract a non-negative decimal integer from a simple JSON-like config string.
// This helper intentionally preserves the existing loader's limited parsing
// model; it is not a complete JSON parser. Malformed or out-of-range values
// return the caller-provided default.
inline uint32_t extract_json_uint(const std::string& json,
                                  const std::string& key,
                                  uint32_t default_value = 0) {
    const std::string search = "\"" + key + "\"";
    const size_t key_pos = json.find(search);
    if (key_pos == std::string::npos) return default_value;

    size_t pos = json.find(':', key_pos + search.size());
    if (pos == std::string::npos) return default_value;
    ++pos;

    while (pos < json.size() &&
           std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }

    const size_t start = pos;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        ++pos;
    }
    if (start == pos) return default_value;

    uint32_t value = 0;
    const char* first = json.data() + start;
    const char* last = json.data() + pos;
    const auto parsed = std::from_chars(first, last, value, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != last) {
        return default_value;
    }

    return value;
}

}  // namespace config_value_parser
