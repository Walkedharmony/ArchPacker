
#include "stdafx.h"
#include "arch_packer.h"
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;


void ShowHelp() {
    std::cout << "ArchPacker - Pembuat Archive untuk Platform x86/x32\n\n";
    std::cout << "Penggunaan:\n";
    std::cout << "  arch_packer [options] <output.arch> <file1> [file2 ...]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c       Aktifkan kompresi (default)\n";
    std::cout << "  -x       Extract Archives\n";
    std::cout << "  -nc      Nonaktifkan kompresi\n";
    std::cout << "  -e       Aktifkan enkripsi\n";
    std::cout << "  -p <pw>  Tentukan passphrase untuk enkripsi\n";
    std::cout << "  -v       Tampilkan versi\n";
    std::cout << "  -h       Tampilkan bantuan ini\n";
    std::cout << "\nContoh:\n";
    std::cout << "  arch_packer game.arch asset/*.png\n";
    std::cout << "  arch_packer -nc data.arch file1.bin file2.dat\n";
    std::cout << "  arch_packer -e -p \"passwordku\" rahasia.arch dokumen/*\n";
}
void ShowVersion() {
    std::cout << "ArchPacker v1.0 (x86/x32)\n";
    std::cout << "Format versi: " << ArchConstants::VERSION << "\n";
    std::cout << "Ukuran header: " << sizeof(ArchHeader) << " bytes\n";
}

int ProcessCommandLine(int argc, char* argv[],
    std::string& outputFile,
    std::vector<std::string>& inputFiles,
    bool& enableCompression,
    bool& enableEncryption,
    std::string& passphrase) {
    if (argc < 2) {
        ShowHelp();
        return 1;
    }


  
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            ShowHelp();
            return 0;
        }
        else if (strcmp(argv[i], "-v") == 0) {
            ShowVersion();
            return 0;
        }
        else if (strcmp(argv[i], "-c") == 0) {
            enableCompression = true;
        }
        else if (strcmp(argv[i], "-nc") == 0) {
            enableCompression = false;
        }
        else if (strcmp(argv[i], "-e") == 0) {
            enableEncryption = true;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: Opsi -p membutuhkan passphrase\n";
                return 1;
            }
            passphrase = argv[++i];
            enableEncryption = true;
        }
        else if (argv[i][0] == '-') {
            std::cerr << "Error: Opsi tidak dikenali '" << argv[i] << "'\n";
            return 1;
        }
        else {
            
            if (outputFile.empty()) {
                outputFile = argv[i];
            }
            else {
                
#ifdef _WIN32
                if (strchr(argv[i], '*')) {
                    WIN32_FIND_DATAA fd;
                    HANDLE hFind = FindFirstFileA(argv[i], &fd);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        do {
                            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                inputFiles.push_back(std::string(fd.cFileName));
                            }
                        } while (FindNextFileA(hFind, &fd));
                        FindClose(hFind);
                    }
                }
                else
#endif
                {
                    inputFiles.push_back(std::string(argv[i]));
                }
            }
        }
    }

    if (outputFile.empty()) {
        std::cerr << "Error: Nama file output harus dispesifikasikan\n";
        return 1;
    }

    if (inputFiles.empty()) {
        std::cerr << "Error: Tidak ada file input yang spesifikasikan\n";
        return 1;
    }

    if (enableEncryption && passphrase.empty()) {
        std::cerr << "Error: Enkripsi diaktifkan tetapi passphrase tidak diberikan\n";
        return 1;
    }

    return -1; 
}

int main(int argc, char* argv[]) {
    try {
        if (argc >= 2 && strcmp(argv[1], "-x") == 0) {
            if (argc < 3) {
                std::cerr << "Error: Mohon spesifikasikan file archive untuk extract\n";
                std::cerr << "Contoh: " << argv[0] << " -x archive.arch [-p password] [output_dir]\n";
                return 1;
            }

            std::string archiveFile;
            std::string outputDir;
            std::string passphrase;
            bool hasPassphrase = false;
            bool preserveStructure = false;

            for (int i = 2; i < argc; i++) {
                if (strcmp(argv[i], "-p") == 0) {
                    if (i + 1 < argc) {
                        passphrase = argv[++i];
                        hasPassphrase = true;
                    }
                    else {
                        std::cerr << "Error: Opsi -p membutuhkan passphrase\n";
                        return 1;
                    }
                }
                else if (strcmp(argv[i], "--preserve") == 0) {
                    preserveStructure = true;
                }
                else if (archiveFile.empty()) {
                    archiveFile = argv[i];
                }
                else {
                    outputDir = argv[i];
                }
            }

            if (archiveFile.empty()) {
                std::cerr << "Error: Nama file archive harus dispesifikasikan\n";
                return 1;
            }

            ArchPacker packer;
            if (hasPassphrase) {
                packer.SetEncryptionKey(passphrase);
            }

            std::cout << "Memulai ekstraksi archive: " << archiveFile << "\n";
            if (hasPassphrase) {
                std::cout << "Menggunakan passphrase untuk dekripsi\n";
            }
            if (preserveStructure) {
                std::cout << "Mempertahankan struktur folder\n";
            }

            return packer.ExtractArchive(archiveFile, outputDir, preserveStructure) ? 0 : 1;
        }

        std::string outputFile;
        std::vector<std::string> inputFiles;
        bool enableCompression = true;
        bool enableEncryption = false;
        std::string passphrase;

        int result = ProcessCommandLine(argc, argv, outputFile, inputFiles,
            enableCompression, enableEncryption, passphrase);
        if (result != -1) {
            return result;
        }

        if (enableEncryption && passphrase.empty()) {
            std::cerr << "Error: Enkripsi diaktifkan tetapi passphrase kosong\n";
            return 1;
        }

        uint64_t totalSize = 0;
        for (const auto& file : inputFiles) {
            try {
                if (fs::exists(file)) {
                    totalSize += fs::file_size(file);
                }
                else {
                    std::cerr << "Warning: File tidak ditemukan - " << file << "\n";
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error menghitung ukuran " << file << ": " << e.what() << "\n";
            }
        }

        std::cout << "Membuat archive '" << outputFile << "' dengan "
            << inputFiles.size() << " file ("
            << (totalSize / 1024) << " KB)\n";
        std::cout << "Kompresi: " << (enableCompression ? "AKTIF" : "NONAKTIF") << "\n";
        std::cout << "Enkripsi: " << (enableEncryption ? "AKTIF" : "NONAKTIF") << "\n";

        ArchPacker packer;
        if (enableEncryption) {
            packer.SetEncryptionKey(passphrase);
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        if (!packer.CreateArchive(outputFile, inputFiles, enableCompression)) {
            std::cerr << "Gagal membuat archive!\n";
            return 1;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        try {
            std::ifstream in(outputFile, std::ios::binary);
            if (in) {
                ArchHeader header;
                if (in.read(reinterpret_cast<char*>(&header), sizeof(header))) {
                    if (header.magic == ArchConstants::MAGIC) {
                        std::cout << "\nArchive berhasil dibuat!\n";
                        std::cout << "Waktu proses: " << duration.count() << " ms\n";
                        std::cout << "Detail Archive:\n";
                        std::cout << "  Jumlah file: " << header.fileCount << "\n";
                        std::cout << "  Ukuran file: " << fs::file_size(outputFile) << " bytes\n";
                        return 0;
                    }
                }
            }
            std::cerr << "Warning: Gagal membaca info archive setelah pembuatan\n";
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "Warning: Error verifikasi archive - " << e.what() << "\n";
            return 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\nError FATAL: " << e.what() << "\n";
        return 1;
    }
}