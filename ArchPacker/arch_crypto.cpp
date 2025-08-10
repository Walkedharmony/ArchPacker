#include "arch_crypto.h"
#include "stdafx.h"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>  


namespace ArchCrypto {
    
    std::vector<uint8_t> GenerateKey(const std::string& passphrase) {
        const size_t key_size = 32; 
        std::vector<uint8_t> key(key_size);

        uint32_t hash = 2166136261u;
        for (char c : passphrase) {
            hash = (hash ^ c) * 16777619u;
        }

        for (size_t i = 0; i < key_size; ++i) {
            key[i] = static_cast<uint8_t>(hash >> (i % 4 * 8));
            if (i % 8 == 0) {
                hash = hash * 1103515245 + 12345; 
            }
        }

        
        std::cout << "Generated Key (" << key_size << " bytes): ";
        for (const auto& byte : key) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;

        return key;
    }

    // Enkripsi menggunakan algoritma sederhana (XOR + shuffling)
    void EncryptData(std::vector<uint8_t>& data, const std::vector<uint8_t>& key) {
        if (key.empty() || data.empty()) return;


        XorWithKey(data, key);

        for (size_t i = 0; i < data.size(); ++i) {
            size_t swap_pos = (i + key[i % key.size()]) % data.size();
            std::swap(data[i], data[swap_pos]);
        }

        std::vector<uint8_t> reversed_key(key.rbegin(), key.rend());
        XorWithKey(data, reversed_key);
    }

    void DecryptData(std::vector<uint8_t>& data, const std::vector<uint8_t>& key) {
        if (key.empty() || data.empty()) return;

        std::vector<uint8_t> reversed_key(key.rbegin(), key.rend());
        XorWithKey(data, reversed_key);

        std::vector<size_t> shuffle_order(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            shuffle_order[i] = (i + key[i % key.size()]) % data.size();
        }
        for (size_t i = data.size(); i-- > 0; ) {
            size_t swap_pos = shuffle_order[i];
            if (swap_pos != i) {
                std::swap(data[i], data[swap_pos]);
            }
        }

        XorWithKey(data, key);
    }

    void XorWithKey(std::vector<uint8_t>& data, const std::vector<uint8_t>& key) {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] ^= key[i % key.size()];
        }
    }

    /**
    std::string GarbleFilename(const std::string& filename, const std::vector<uint8_t>& key) {
        if (key.empty()) return filename;

        std::vector<uint8_t> buffer(filename.begin(), filename.end());
        XorWithKey(buffer, key);

        // Konversi ke hexadecimal untuk memastikan karakter yang valid
        std::stringstream ss;
        for (const auto& byte : buffer) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }

    std::string DegarbleFilename(const std::string& garbledName, const std::vector<uint8_t>& key) {
        if (key.empty() || garbledName.size() % 2 != 0) return garbledName;

        std::vector<uint8_t> buffer;
        for (size_t i = 0; i < garbledName.size(); i += 2) {
            std::string byteStr = garbledName.substr(i, 2);
            buffer.push_back(static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16)));
        }

        XorWithKey(buffer, key);
        return std::string(buffer.begin(), buffer.end());
    }*/
}

