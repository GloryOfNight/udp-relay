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