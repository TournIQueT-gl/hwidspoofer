cmake_minimum_required(VERSION 3.16)
project(SafetyParserTests LANGUAGES CXX)

enable_testing()

add_executable(smbios_parser_tests
    test_smbios_record_parser.cpp
)

target_compile_features(smbios_parser_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(smbios_parser_tests PRIVATE /W4 /WX)
else()
    target_compile_options(smbios_parser_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_test(NAME smbios_parser_tests COMMAND smbios_parser_tests)

add_executable(config_value_parser_tests
    test_config_value_parser.cpp
)

target_compile_features(config_value_parser_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(config_value_parser_tests PRIVATE /W4 /WX)
else()
    target_compile_options(config_value_parser_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_test(NAME config_value_parser_tests COMMAND config_value_parser_tests)

add_executable(lifecycle_controller_tests
    test_lifecycle_controller.cpp
)

target_compile_features(lifecycle_controller_tests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(lifecycle_controller_tests PRIVATE /W4 /WX)
else()
    target_compile_options(lifecycle_controller_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

add_test(NAME lifecycle_controller_tests COMMAND lifecycle_controller_tests)
