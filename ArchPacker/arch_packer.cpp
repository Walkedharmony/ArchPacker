#include "stdafx.h"
#include "arch_packer.h"
#include <filesystem>
#include <chrono>     
namespace fs = std::filesystem;

ArchPacker::ArchPacker() {}
ArchPacker::~ArchPacker() {}
bool ArchPacker::ReadHeader(std::ifstream& in, ArchHeader& header) {
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    return in.good() && header.magic == ArchConstants::MAGIC;
}

bool ArchPacker::ReadFileEntries(std::ifstream& in, std::vector<FileEntry>& entries, uint32_t indexOffset) {
    in.seekg(indexOffset);
    if (!in) return false;

    while (in.peek() != EOF) {
        FileEntry entry;
        in.read(reinterpret_cast<char*>(&entry), sizeof(entry));
        if (!in) break;
        entries.push_back(entry);
    }

    return !entries.empty();
}


void ArchPacker::SetEncryptionKey(const std::string& passphrase) {
    m_encryptionKey = ArchCrypto::GenerateKey(passphrase);
    m_useEncryption = !passphrase.empty();
}

bool ArchPacker::CreateArchive(const std::string& outputFile,
    const std::vector<std::string>& inputPaths,
    bool enableCompression) {
    try {
        std::ofstream out(outputFile, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Cannot create output file: " + outputFile);
        }

        uint32_t totalFiles = 0;
        for (const auto& path : inputPaths) {
            if (fs::is_directory(path)) {
                for (const auto& entry : fs::recursive_directory_iterator(path)) {
                    if (entry.is_regular_file()) totalFiles++;
                }
            }
            else {
                totalFiles++;
            }
        }

        WriteHeader(out, totalFiles, 0);

        std::vector<FileEntry> entries;
        for (const auto& path : inputPaths) {
            if (fs::is_directory(path)) {
                ProcessFolder(path, out, entries, enableCompression);
            }
            else {
                ProcessFile(path, out, entries, enableCompression);
            }
        }

        uint32_t indexOffset = static_cast<uint32_t>(out.tellp());

        for (const auto& entry : entries) {
            out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
        }

        out.seekp(0);
        WriteHeader(out, static_cast<uint32_t>(entries.size()), indexOffset);

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

void ArchPacker::WriteHeader(std::ofstream& out, uint32_t fileCount, uint32_t indexOffset) {
    ArchHeader header;
    header.fileCount = fileCount;
    header.indexOffset = indexOffset;
    header.flags = 0;
    memset(header.reserved, 0, sizeof(header.reserved));

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

void ArchPacker::ProcessFile(const std::string& filePath,
    std::ofstream& out,
    std::vector<FileEntry>& entries,
    bool enableCompression) {
    FileEntry entry;
    memset(&entry, 0, sizeof(entry));

    fs::path path(filePath);
    std::string filename = path.filename().string();

    if (filename.length() >= ArchConstants::MAX_FILENAME_LENGTH) {
        throw std::runtime_error("Filename exceeds maximum length");
    }


    strncpy_s(entry.filename, filename.c_str(), ArchConstants::MAX_FILENAME_LENGTH - 1);
    entry.filename[ArchConstants::MAX_FILENAME_LENGTH - 1] = '\0';

    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("Cannot open input file: " + filePath);
    }

    entry.size = static_cast<uint32_t>(in.tellg());
    entry.offset = static_cast<uint32_t>(out.tellp());

    auto ftime = fs::last_write_time(filePath);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    entry.timestamp = std::chrono::system_clock::to_time_t(sctp);

    entry.checksum = ArchUtils::CalculateChecksum(filePath);

    in.seekg(0);
    std::vector<uint8_t> buffer(entry.size);
    in.read(reinterpret_cast<char*>(buffer.data()), entry.size);

    if (enableCompression) {
        std::vector<uint8_t> compressedData;
        ArchUtils::CompressData(buffer, compressedData);

        if (compressedData.size() < buffer.size()) {
            out.write(reinterpret_cast<char*>(compressedData.data()), compressedData.size());
            entry.compressionType = 1;
            entry.compressedSize = static_cast<uint32_t>(compressedData.size());
        }
        else {
            out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            entry.compressionType = 0;
            entry.compressedSize = 0;
        }
    }
    else {
        out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
        entry.compressionType = 0;
        entry.compressedSize = 0;
    }

    if (m_useEncryption) {
        ArchCrypto::EncryptData(buffer, m_encryptionKey);
        entry.encryptionType = 1;

    }
    else {
        entry.encryptionType = 0;
    }

    entries.push_back(entry);


}
void ArchPacker::ProcessFolder(const std::string& folderPath,
    std::ofstream& out,
    std::vector<FileEntry>& entries,
    bool enableCompression,
    const std::string& relativePath) {
    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_directory()) {
                std::string newRelativePath = relativePath + entry.path().filename().string() + "/";
                ProcessFolder(entry.path().string(), out, entries, enableCompression, newRelativePath);
            }
            else if (entry.is_regular_file()) {
                try {
                    std::string fullPath = entry.path().string();
                    std::string archivePath = relativePath + entry.path().filename().string();

                    FileEntry fileEntry;
                    memset(&fileEntry, 0, sizeof(fileEntry));

                    if (archivePath.length() >= ArchConstants::MAX_FILENAME_LENGTH) {
                        std::cerr << "Warning: Path terlalu panjang, file akan dilewati: "
                            << fullPath << std::endl;
                        continue;
                    }
                    strncpy_s(fileEntry.filename, archivePath.c_str(), ArchConstants::MAX_FILENAME_LENGTH - 1);

                    std::ifstream in(fullPath, std::ios::binary | std::ios::ate);
                    if (!in) {
                        std::cerr << "Warning: Tidak bisa membuka file: " << fullPath << std::endl;
                        continue;
                    }

                    fileEntry.size = static_cast<uint32_t>(in.tellg());
                    fileEntry.offset = static_cast<uint32_t>(out.tellp());

                    auto ftime = fs::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                    fileEntry.timestamp = std::chrono::system_clock::to_time_t(sctp);

                    fileEntry.checksum = ArchUtils::CalculateChecksum(fullPath);

                    in.seekg(0);
                    std::vector<uint8_t> buffer(fileEntry.size);
                    in.read(reinterpret_cast<char*>(buffer.data()), fileEntry.size);

                    if (enableCompression) {
                        std::vector<uint8_t> compressedData;
                        ArchUtils::CompressData(buffer, compressedData);

                        if (compressedData.size() < buffer.size()) {
                            if (m_useEncryption) {
                                ArchCrypto::EncryptData(compressedData, m_encryptionKey); 
                            }
                            out.write(reinterpret_cast<char*>(compressedData.data()), compressedData.size());
                            fileEntry.compressionType = 1;
                            fileEntry.compressedSize = static_cast<uint32_t>(compressedData.size());
                        }
                        else {
                            if (m_useEncryption) {
                                ArchCrypto::EncryptData(buffer, m_encryptionKey); 
                            }
                            out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
                            fileEntry.compressionType = 0;
                            fileEntry.compressedSize = 0;
                        }
                    }
                    else {
                        if (m_useEncryption) {
                            ArchCrypto::EncryptData(buffer, m_encryptionKey); 
                        }
                        out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
                        fileEntry.compressionType = 0;
                        fileEntry.compressedSize = 0;
                    }

                    fileEntry.encryptionType = m_useEncryption ? 1 : 0;
                    entries.push_back(fileEntry);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error memproses file " << entry.path().string()
                        << ": " << e.what() << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error memproses folder " + folderPath + ": " + e.what());
    }
}
bool ArchPacker::ExtractArchive(const std::string& inputFile,
    const std::string& outputDir,
    bool preserveStructure)
{
    try {
        std::ifstream in(inputFile, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Gagal membuka file archive: " + inputFile);
        }

        ArchHeader header;
        if (!ReadHeader(in, header)) {
            throw std::runtime_error("Format archive tidak valid atau corrupt");
        }

        std::vector<FileEntry> entries;
        if (!ReadFileEntries(in, entries, header.indexOffset)) {
            throw std::runtime_error("Gagal membaca tabel file entries");
        }


        fs::path outputPath = outputDir.empty() ? fs::path(inputFile).stem() : outputDir;
        if (!fs::exists(outputPath)) {
            fs::create_directories(outputPath);
        }

        int totalFiles = static_cast<int>(entries.size());
        int successCount = 0;
        int encryptedFiles = 0;
        bool hasEncryptionErrors = false;

        std::cout << "Memulai ekstraksi " << totalFiles << " file ke: "
            << outputPath.string() << "\n";


        for (const auto& entry : entries) {
            try {


                std::cout << "  [" << (successCount + 1) << "/" << totalFiles << "] "
                    << entry.filename;

                if (entry.encryptionType == 1) {
                    std::cout << " [ENCRYPTED]";
                    encryptedFiles++;
                }
                if (entry.compressionType == 1) {
                    std::cout << " [COMPRESSED]";
                }
                std::cout << "\n";

                fs::path filePath = outputPath;
                if (preserveStructure) {
                    filePath /= entry.filename;
                    fs::create_directories(filePath.parent_path());
                }
                else {
                    filePath /= fs::path(entry.filename).filename();
                }

                in.seekg(entry.offset);
                std::vector<uint8_t> fileData(entry.compressedSize > 0 ?
                    entry.compressedSize : entry.size);
                in.read(reinterpret_cast<char*>(fileData.data()), fileData.size());

                std::vector<uint8_t> processedData;
                if (entry.encryptionType == 1) {
                    if (m_encryptionKey.empty()) {
                        throw std::runtime_error("File terenkripsi tetapi passphrase tidak diberikan");
                    }
                    ArchCrypto::DecryptData(fileData, m_encryptionKey); // Dekripsi sebelum dekompresi
                }
                if (entry.compressionType == 1) {
                    std::vector<char> compressedData(fileData.begin(), fileData.end());
                    std::vector<char> decompressedData;
                    if (!ArchUtils::DecompressData(compressedData, decompressedData, entry.size)) {
                        throw std::runtime_error("Dekompresi gagal");
                    }
                    processedData.assign(decompressedData.begin(), decompressedData.end());
                }
                else {
                    processedData = std::move(fileData);
                }

                std::ofstream outFile(filePath, std::ios::binary);
                if (!outFile) {
                    throw std::runtime_error("Gagal membuat file output");
                }
                outFile.write(reinterpret_cast<const char*>(processedData.data()), processedData.size());


                auto ftime = std::chrono::system_clock::from_time_t(entry.timestamp);
                auto fsTime = std::chrono::time_point_cast<fs::file_time_type::duration>(
                    ftime - std::chrono::system_clock::now() + fs::file_time_type::clock::now());
                fs::last_write_time(filePath, fsTime);

                successCount++;
            }
            catch (const std::exception& e) {
                std::cerr << "    ERROR: " << e.what() << "\n";
                continue;
            }
        }

        std::cout << "\nEkstraksi selesai!\n";
        std::cout << "  File berhasil diekstrak: " << successCount << "/" << totalFiles << "\n";

        if (encryptedFiles > 0) {
            if (m_encryptionKey.empty()) {
                std::cout << "  PERINGATAN: " << encryptedFiles
                    << " file terenkripsi TIDAK diproses (perlu passphrase)\n";
            }
            else if (hasEncryptionErrors) {
                std::cout << "  PERINGATAN: Beberapa file terenkripsi gagal diproses\n";
            }
            else {
                std::cout << "  " << encryptedFiles << " file terenkripsi berhasil didekripsi\n";
            }
        }

        return successCount > 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\nERROR EKSTRAKSI: " << e.what() << "\n";
        return false;
    }
}