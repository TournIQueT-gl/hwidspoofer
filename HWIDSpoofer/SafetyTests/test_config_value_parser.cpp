#include "../SpooferDLL/src/utils/config_value_parser.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct TestCase {
    const char* name;
    std::string input;
    std::string key;
    uint32_t fallback;
    uint32_t expected;
};

bool run(const TestCase& tc) {
    const uint32_t actual =
        config_value_parser::extract_json_uint(tc.input, tc.key, tc.fallback);
    if (actual == tc.expected) return true;
    std::cerr << "FAIL: " << tc.name << " expected " << tc.expected
              << ", got " << actual << '\n';
    return false;
}

}  // namespace

int main() {
    const std::vector<TestCase> cases = {
        {"ordinary value", R"({"volume_serial":12345})", "volume_serial", 7, 12345},
        {"zero", R"({"n":0})", "n", 7, 0},
        {"leading whitespace", "{\"n\": \t\r\n42}", "n", 7, 42},
        {"maximum uint32", R"({"n":4294967295})", "n", 7,
         std::numeric_limits<uint32_t>::max()},
        {"missing key", R"({"other":42})", "n", 7, 7},
        {"missing colon", R"({"n" 42})", "n", 7, 7},
        {"empty value", R"({"n":})", "n", 7, 7},
        {"negative value", R"({"n":-1})", "n", 7, 7},
        {"plus sign", R"({"n":+1})", "n", 7, 7},
        {"uint32 overflow", R"({"n":4294967296})", "n", 7, 7},
        {"very large overflow", R"({"n":999999999999999999999999999999})", "n", 7, 7},
        {"string value", R"({"n":"12"})", "n", 7, 7},
        {"trailing letters", R"({"n":12abc})", "n", 7, 7},
        {"decimal value", R"({"n":12.5})", "n", 7, 7},
        {"exponent value", R"({"n":12e3})", "n", 7, 7},
        {"invalid closing delimiter", R"({"n":12])", "n", 7, 7},
        {"valid comma delimiter", R"({"n":12,"other":1})", "n", 7, 12},
        {"valid whitespace delimiter", "{\\"n\\":12 \\n}", "n", 7, 12},
        {"default zero", R"({"n":-1})", "n", 0, 0},
    };

    bool passed = true;
    for (const auto& tc : cases) passed = run(tc) && passed;
    if (!passed) return 1;
    std::cout << "All numeric config parser tests passed (" << cases.size()
              << " cases).\n";
    return 0;
}
