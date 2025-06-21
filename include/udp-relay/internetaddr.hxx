// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#pragma once

#include <memory>

#if __unix
#include "unix/internetaddrUnix.hxx"
using internetaddr = internetaddrUnix;
#elif _WIN32
#include "win/internetaddrWin.hxx"
using internetaddr = internetaddrWin;
#endif

using uniqueInternetaddr = std::unique_ptr<internetaddr>;
using sharedInternetaddr = std::shared_ptr<internetaddr>;