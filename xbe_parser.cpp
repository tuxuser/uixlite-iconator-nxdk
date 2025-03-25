#include "xbe_parser.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <string.h>

XBEParser::XBEParser() : xbe_data(nullptr), xbe_size(0) {}

XBEParser::~XBEParser() {
    if (xbe_data) {
        delete[] xbe_data;
    }
}

bool XBEParser::LoadXBE(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    xbe_size = file.tellg();
    file.seekg(0, std::ios::beg);

    xbe_data = new uint8_t[xbe_size];
    if (!file.read(reinterpret_cast<char*>(xbe_data), xbe_size)) {
        delete[] xbe_data;
        xbe_data = nullptr;
        return false;
    }

    return ReadXBE();
}

bool XBEParser::ReadXBE() {
    if (!xbe_data || xbe_size < sizeof(XBE_HEADER)) {
        return false;
    }

    memcpy(&header, xbe_data, sizeof(XBE_HEADER));

    // Verify XBE magic number
    if (header.magic != 0x48454258) { // "XBEH"
        return false;
    }

    return ReadCertificate() && ReadSections();
}

bool XBEParser::ReadCertificate() {
    if (!xbe_data || header.certificate_address >= xbe_size) {
        return false;
    }

    memcpy(&certificate, 
           xbe_data + header.certificate_address, 
           sizeof(XBE_CERTIFICATE));

    return true;
}

bool XBEParser::ReadSections() {
    if (!xbe_data || header.section_headers_address >= xbe_size) {
        return false;
    }

    // Clear any existing sections
    sections.clear();

    // Read each section header
    for (uint32_t i = 0; i < header.number_of_sections; i++) {
        XBE_SECTION section;
        uint32_t section_offset = header.section_headers_address + (i * sizeof(XBE_SECTION));
        
        if (section_offset + sizeof(XBE_SECTION) > xbe_size) {
            return false;
        }

        memcpy(&section, xbe_data + section_offset, sizeof(XBE_SECTION));
        sections.push_back(section);
    }

    return true;
}

bool XBEParser::GetSectionByName(const std::string& name, XBE_SECTION& section) {
    for (const auto& sec : sections) {
        // Section name is stored as an offset from base address
        if (sec.section_name_address >= xbe_size) {
            continue;
        }

        std::string section_name;
        const char* name_ptr = reinterpret_cast<const char*>(xbe_data + sec.section_name_address);
        
        // Read until null terminator or end of file
        while (*name_ptr && (reinterpret_cast<const uint8_t*>(name_ptr) - xbe_data) < xbe_size) {
            section_name += *name_ptr++;
        }

        if (section_name == name) {
            section = sec;
            return true;
        }
    }
    return false;
}

bool XBEParser::ExtractTitleID(std::string& title_id) {
    if (!xbe_data) {
        return false;
    }

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) 
       << certificate.title_id;
    
    title_id = ss.str();
    return true;
}

bool XBEParser::ExtractTitle(std::string& title) {
    if (!xbe_data) {
        return false;
    }

    // Convert wide string to regular string
    title.clear();
    for (int i = 0; i < TITLE_LEN && certificate.title_name[i] != 0; i++) {
        title += static_cast<char>(certificate.title_name[i]);
    }
    return true;
}

bool XBEParser::ExtractTitleImage(std::vector<uint8_t>& image_data) {
    if (!xbe_data) {
        return false;
    }

    XBE_SECTION section = {0};

    if (!GetSectionByName("$$XTIMAGE", section)) {
        return false;
    }

    const uint8_t *xtimage_data = xbe_data + section.raw_address;

    // Copy the entire bitmap data
    image_data.resize(section.raw_size);
    memcpy(image_data.data(), xtimage_data, section.raw_size);

    return true;
} 