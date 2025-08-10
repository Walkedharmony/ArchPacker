
#pragma once
#include "stdafx.h"

namespace ArchUtils {
    uint32_t CalculateChecksum(const std::string& filename);
    void CompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    std::string GetLastErrorString();
    bool ValidateFilename(const std::string& filename);
    bool DecompressData(const std::vector<char>& input,
        std::vector<char>& output,
        uint32_t originalSize);

}
