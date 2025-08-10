#pragma once
#include <fstream>
#include <vector>
#include <string>
#include "arch_struct.h"
#include "arch_utils.h"

class ArchPacker {
public:
    ArchPacker();
    ~ArchPacker();

    bool CreateArchive(const std::string& outputFile,
        const std::vector<std::string>& inputFiles,
        bool enableCompression = true);

    bool ExtractArchive(const std::string& inputFile,
        const std::string& outputDir = "",
        bool preserveStructure = true);

    bool ReadHeader(std::ifstream& in, ArchHeader& header);
    bool ReadFileEntries(std::ifstream& in, std::vector<FileEntry>& entries, uint32_t indexOffset);

    uint64_t CalculateTotalSize(const std::vector<std::string>& files) const;
    void SetEncryptionKey(const std::string& passphrase);

private:
    void WriteHeader(std::ofstream& out, uint32_t fileCount, uint32_t indexOffset);
    void ProcessFile(const std::string& filePath,
        std::ofstream& out,
        std::vector<FileEntry>& entries,
        bool enableCompression);
    void ProcessFolder(const std::string& folderPath,
        std::ofstream& out,
        std::vector<FileEntry>& entries,
        bool enableCompression,
        const std::string& relativePath = "");
    bool VerifyFile(const std::string& filePath) const;
    std::vector<uint8_t> m_encryptionKey;
    bool m_useEncryption;
    bool m_printedKeyOnce = false;

    ArchPacker(const ArchPacker&) = delete;
    ArchPacker& operator=(const ArchPacker&) = delete;
};