#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace ArchCrypto {


    std::vector<uint8_t> GenerateKey(const std::string& passphrase);


    void EncryptData(std::vector<uint8_t>& data, const std::vector<uint8_t>& key);

    void DecryptData(std::vector<uint8_t>& data, const std::vector<uint8_t>& key);


    void XorWithKey(std::vector<uint8_t>& data, const std::vector<uint8_t>& key);
    void PrintGeneratedKey(const std::string& passphrase);
    /*std::string GarbleFilename(const std::string& filename, const std::vector<uint8_t>& key);
    std::string DegarbleFilename(const std::string& garbledName, const std::vector<uint8_t>& key);*/
}
