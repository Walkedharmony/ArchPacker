#pragma once
#include "stdafx.h"

#pragma pack(push, 1) 

struct ArchHeader {
    uint32_t magic;         // 4 byte
    uint32_t version;       // 4 byte (total 8)
    uint32_t fileCount;     // 4 byte (total 12)
    uint32_t indexOffset;   // 4 byte (total 16)
    uint32_t flags;         // 4 byte (total 20)
    uint8_t reserved[44];   // 44 byte (total 64)

    ArchHeader() :
        magic(ArchConstants::MAGIC),
        version(ArchConstants::VERSION),
        fileCount(0),
        indexOffset(0),
        flags(0) {
        memset(reserved, 0, sizeof(reserved));
    }
};

struct FileEntry {
    char filename[ArchConstants::MAX_FILENAME_LENGTH]; // 260 byte
    uint32_t offset;        // 4 byte (total 264)
    uint32_t size;          // 4 byte (total 268)
    uint32_t compressedSize;// 4 byte (total 272)
    uint32_t checksum;      // 4 byte (total 276)
    uint32_t flags;         // 4 byte (total 280)
    uint64_t timestamp;     // 8 byte (total 288)
    uint8_t compressionType;// 1 byte (total 289)
    uint8_t encryptionType; // 1 byte (total 290)
    uint16_t nameFlags;     // 2 byte (total 292) 


    static constexpr uint32_t FLAG_HAS_ORIGINAL_NAME = 0x1;
    static constexpr uint32_t FLAG_NAME_IS_GARBLED = 0x2; 
};

static_assert(sizeof(ArchHeader) == ArchConstants::HEADER_SIZE,
    "ArchHeader size mismatch (harus tepat 64 byte)");
static_assert(sizeof(FileEntry) == 292,
    "FileEntry size mismatch (harus tepat 300 byte)");

#pragma pack(pop)