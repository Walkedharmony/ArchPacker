
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include "arch_crypto.h"

// Third Party
#include "zlib.h"


namespace ArchConstants {
    const uint32_t MAGIC = 0x48435241; // 'ARCH' in little-endian
    const uint32_t VERSION = 1;
    const size_t MAX_FILENAME_LENGTH = 260; 
    const size_t HEADER_SIZE = 64; 
}