# Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <bit>
#ifndef __cpp_lib_byteswap
#error feature not available
#endif
int main() {}
" HAS_CPP_LIB_BYTESWAP)

check_cxx_source_compiles("
#include <iostream>
#include <format>
#ifndef __cpp_lib_format
#error feature not available
#endif
int main() {std::cout << std::format(\"{}, {}!\", \"Hello\", \"World\") << std::endl;}
" HAS_CPP_LIB_FORMAT)

check_cxx_source_compiles("
#include <print>
int main() {std::println(\"{}, {}!\", \"Hello\", \"World\");}
" HAS_CPP_LIB_PRINT)

check_cxx_source_compiles("
#include <stacktrace>
#include <iostream>
#ifndef __cpp_lib_stacktrace
#error feature not available
#endif
int main() { std::cout << std::stacktrace::current() << std::endl;}
" HAS_CPP_LIB_STACKTRACE)

check_cxx_source_compiles("
#include <flap_map>
#ifndef __cpp_lib_flat_map
#error feature not available
#endif
int main() {}
" HAS_CPP_LIB_FLAT_MAP)