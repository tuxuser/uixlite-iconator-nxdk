#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define TITLE_LEN 40
#define DIGEST_LEN 20
#define SIGNATURE_LEN 256
#define KEY_LEN 16

// XBE header structures
#pragma pack(push, 1)
struct XBE_HEADER {
    uint32_t magic;                    // 0x48454258 ("XBEH")
    uint8_t  signature[SIGNATURE_LEN]; // Digital signature
    uint32_t base_address;             // Base address of the XBE
    uint32_t size_of_headers;          // Size of all headers
    uint32_t size_of_image;            // Size of the entire image
    uint32_t size_of_image_header;     // Size of the image header
    uint32_t timestamp;                // Time/date stamp
    uint32_t certificate_address;      // Address of the certificate
    uint32_t number_of_sections;       // Number of sections
    uint32_t section_headers_address;  // Address of section headers
    uint32_t init_flags;               // Initialization flags
    uint32_t entry_point;              // Entry point address
    uint32_t tls_address;              // Thread local storage address
    uint32_t pe_stack_commit;          // Size of stack commit
    uint32_t pe_heap_reserve;          // Size of heap reserve
    uint32_t pe_heap_commit;           // Size of heap commit
    uint32_t pe_base_address;          // PE base address
    uint32_t pe_size_of_image;         // PE size of image
    uint32_t pe_checksum;              // PE checksum
    uint32_t pe_timestamp;             // PE timestamp
    uint32_t debug_pathname_address;   // Address of debug pathname
    uint32_t debug_filename_address;   // Address of debug filename
    uint32_t debug_unicode_filename_address; // Address of debug unicode filename
    uint32_t kernel_image_thunk_address;    // Address of kernel image thunk
    uint32_t nonkernel_import_dir_address;  // Address of non-kernel import directory
    uint32_t library_versions_count;        // Number of library versions
    uint32_t library_versions_address;      // Address of library versions
    uint32_t kernel_library_version_address; // Address of kernel library version
    uint32_t xapi_library_version_address;   // Address of XAPI library version
    uint32_t logo_bitmap_address;            // Address of logo bitmap
    uint32_t logo_bitmap_size;               // Size of logo bitmap
};

struct XBE_CERTIFICATE {
    uint32_t size;                     // Size of the certificate
    uint32_t timestamp;                // Time/date stamp
    uint32_t title_id;                 // Title ID
    uint16_t title_name[TITLE_LEN];           // Title name (wide string)
    uint8_t  alt_title_ids[16][4];     // Alternative title IDs
    uint32_t allowed_media;            // Allowed media types
    uint32_t game_region;              // Game region
    uint32_t game_ratings;             // Game ratings
    uint32_t disk_number;              // Disk number
    uint32_t version;                  // Version
    uint8_t  lan_key[KEY_LEN];              // LAN key
    uint8_t  signature_key[KEY_LEN];        // Signature key
    uint8_t  alt_signature_keys[16][KEY_LEN]; // Alternative signature keys
    uint32_t original_pe_timestamp;    // Original PE timestamp
    uint32_t original_pe_checksum;     // Original PE checksum
    uint32_t original_pe_size;         // Original PE size
};

struct XBE_SECTION {
    uint32_t section_flags;            // Section flags
    uint32_t virtual_address;          // Virtual address
    uint32_t virtual_size;             // Virtual size
    uint32_t raw_address;              // Raw address
    uint32_t raw_size;                 // Raw size
    uint32_t section_name_address;     // Section name address
    uint32_t section_name_ref_count;   // Section name reference count
    uint32_t head_shared_page_ref_count_address;  // Head shared page reference count address
    uint32_t tail_shared_page_ref_count_address;  // Tail shared page reference count address
    uint8_t  section_digest[DIGEST_LEN];       // Section digest
};
#pragma pack(pop)

struct GameInfo {
    std::string xbe_path;
    uint32_t title_id;
    std::string title;
    std::vector<uint8_t> title_image;  // Raw title image data (xbx)
};

class XBEParser {
public:
    XBEParser();
    ~XBEParser();

    bool LoadXBE(const std::string& filepath);
    bool ExtractTitleID(uint32_t& title_id);
    bool ExtractTitle(std::string& title);
    bool ExtractTitleImage(std::vector<uint8_t>& image_data);
    bool GetSectionByName(const std::string& name, XBE_SECTION& section);

private:
    XBE_HEADER header;
    XBE_CERTIFICATE certificate;
    uint8_t* xbe_data;
    size_t xbe_size;
    std::vector<XBE_SECTION> sections;

    bool ReadXBE();
    bool ReadCertificate();
    bool ReadSections();
}; 