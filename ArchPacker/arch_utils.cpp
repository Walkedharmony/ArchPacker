#include "stdafx.h"
#include "arch_utils.h"

uint32_t ArchUtils::CalculateChecksum(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return 0;

    uint32_t checksum = 0;
    char byte;
    while (file.get(byte)) {
        checksum = (checksum << 5) + checksum + static_cast<uint8_t>(byte);
    }
    return checksum;
}

void ArchUtils::CompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    z_stream zs = { 0 };
    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
        throw std::runtime_error("deflateInit failed: " + GetLastErrorString());
    }

    zs.next_in = const_cast<Bytef*>(input.data());
    zs.avail_in = static_cast<uInt>(input.size());

    int ret;
    char buffer[32768];

    do {
        zs.next_out = reinterpret_cast<Bytef*>(buffer);
        zs.avail_out = sizeof(buffer);

        ret = deflate(&zs, Z_FINISH);
        if (output.size() < zs.total_out) {
            output.insert(output.end(), buffer, buffer + (zs.total_out - output.size()));
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        throw std::runtime_error("Compression failed: " + std::to_string(ret));
    }
}

std::string ArchUtils::GetLastErrorString() {
    DWORD error = GetLastError();
    if (error == 0) return "";

    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, error, 0, (LPSTR)&buffer, 0, NULL);

    std::string message(buffer, size);
    LocalFree(buffer);
    return message;
}

bool IsValidFilename(const std::string& filename) {
    if (filename.empty() || filename.length() >= ArchConstants::MAX_FILENAME_LENGTH) {
        return false;
    }

    const char invalidChars[] = "<>:\"/\\|?*";
    return filename.find_first_of(invalidChars) == std::string::npos;
}
bool ArchUtils::DecompressData(const std::vector<char>& input,
    std::vector<char>& output,
    uint32_t originalSize) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        return false;
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zs.avail_in = static_cast<uInt>(input.size());

    output.resize(originalSize);
    zs.next_out = reinterpret_cast<Bytef*>(output.data());
    zs.avail_out = originalSize;

    int ret = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        std::cerr << "Error decompression: " << zError(ret)
            << " (" << ret << ")\n";
        return false;
    }

    return true;
}