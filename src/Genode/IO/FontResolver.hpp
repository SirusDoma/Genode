#pragma once

#if defined(_WIN32)
#include <Genode/IO/Impl/Win32/FontResolver.hpp>
#elif defined(__APPLE__)
#include <Genode/IO/Impl/macOS/FontResolver.hpp>
#else
#include <Genode/IO/Impl/Unix/FontResolver.hpp>
#endif
