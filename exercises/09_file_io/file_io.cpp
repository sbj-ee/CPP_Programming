// file_io.cpp - Exercise 09: File I/O
// Demonstrates: ifstream/ofstream/fstream, text I/O, binary I/O,
//               stringstream, error checking, RAII (Resource Acquisition Is Initialisation) close, filesystem temp

#include <iostream>
#include <fstream>        // ifstream, ofstream, fstream
#include <sstream>        // istringstream, ostringstream
#include <string>
#include <vector>
#include <filesystem>     // std::filesystem (C++17)
#include <cstdint>        // uint32_t
#include <cstring>        // std::memcpy

namespace fs = std::filesystem;

// Helper: build a temp path so the exercise is self-contained
static fs::path tmp_path(const std::string& filename)
{
    return fs::temp_directory_path() / filename;
}

// ── Section 1: ofstream (write text) ─────────────────────────────────────────

void section_write_text()
{
    std::cout << "=== Section 1: Writing a Text File (ofstream) ===\n";

    fs::path fpath = tmp_path("cpp_ex09_text.txt");

    // ofstream opens for writing; file is created or truncated.
    // RAII: file closes automatically when the stream goes out of scope.
    {
        std::ofstream ofs(fpath);
        if (!ofs.is_open()) {
            std::cerr << "Failed to open " << fpath << " for writing\n";
            return;
        }

        // operator<< works just like std::cout
        ofs << "Line 1: Hello, File I/O!\n";
        ofs << "Line 2: pi = " << 3.14159265 << "\n";
        ofs << "Line 3: The quick brown fox\n";
        // No explicit close() needed; destructor handles it.
    }
    std::cout << "Wrote text file: " << fpath << "\n";
}

// ── Section 2: ifstream (read text) ──────────────────────────────────────────

void section_read_text()
{
    std::cout << "\n=== Section 2: Reading a Text File (ifstream) ===\n";

    fs::path fpath = tmp_path("cpp_ex09_text.txt");

    std::ifstream ifs(fpath);
    if (!ifs.is_open()) {
        std::cerr << "Cannot open " << fpath << "\n";
        return;
    }

    // getline: reads a whole line (excludes '\n')
    std::string line;
    int lineno = 0;
    while (std::getline(ifs, line)) {
        ++lineno;
        std::cout << "  [" << lineno << "] " << line << "\n";
    }

    // Check why the loop ended
    if (ifs.eof())
        std::cout << "  Reached end of file.\n";
    if (ifs.fail() && !ifs.eof())
        std::cout << "  I/O error occurred.\n";

    // ifs closed automatically here

    // Reading word by word with >>
    std::ifstream ifs2(fpath);
    std::string word;
    int word_count = 0;
    while (ifs2 >> word)
        ++word_count;
    std::cout << "Word count (>>): " << word_count << "\n";
}

// ── Section 3: fstream (read + write, binary) ─────────────────────────────────

struct RecordHeader {
    uint32_t magic;     // 0xDEADBEEF
    uint32_t count;     // number of records
};

struct DataRecord {
    int32_t  id;
    double   value;
    char     label[8];
};

void section_binary_io()
{
    std::cout << "\n=== Section 3: Binary I/O (fstream + seekg/seekp) ===\n";

    fs::path fpath = tmp_path("cpp_ex09_binary.bin");

    // ── Write binary ──────────────────────────────────────────────────────────
    {
        std::ofstream ofs(fpath, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            std::cerr << "Cannot open binary file for writing\n";
            return;
        }

        RecordHeader hdr{0xDEADBEEF, 3};
        ofs.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

        DataRecord records[3] = {
            {1, 1.11, "alpha\0\0"},
            {2, 2.22, "beta\0\0\0"},
            {3, 3.33, "gamma\0\0"},
        };
        ofs.write(reinterpret_cast<const char*>(records), sizeof(records));

        std::cout << "Wrote binary file: " << fpath << "\n";
        std::cout << "  tellp (position after write) = " << ofs.tellp() << " bytes\n";
    }

    // ── Read binary ───────────────────────────────────────────────────────────
    {
        std::ifstream ifs(fpath, std::ios::binary);
        if (!ifs) {
            std::cerr << "Cannot open binary file for reading\n";
            return;
        }

        RecordHeader hdr{};
        ifs.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
        std::cout << "  magic=0x" << std::hex << hdr.magic << std::dec
                  << "  count=" << hdr.count << "\n";

        for (uint32_t i = 0; i < hdr.count; ++i) {
            DataRecord rec{};
            ifs.read(reinterpret_cast<char*>(&rec), sizeof(rec));
            // label may not be null-terminated if full 8 chars used
            char label[9] = {};
            std::memcpy(label, rec.label, 8);
            std::cout << "  record[" << i << "]: id=" << rec.id
                      << " value=" << rec.value
                      << " label=\"" << label << "\"\n";
        }

        // Seek to a specific record (record index 1 = offset after header)
        std::streamoff rec1_pos = static_cast<std::streamoff>(sizeof(RecordHeader))
                                + static_cast<std::streamoff>(sizeof(DataRecord));
        ifs.seekg(rec1_pos);
        DataRecord rec{};
        ifs.read(reinterpret_cast<char*>(&rec), sizeof(rec));
        char label[9] = {};
        std::memcpy(label, rec.label, 8);
        std::cout << "  seekg to record[1]: id=" << rec.id << " label=\"" << label << "\"\n";

        // File size via seek
        ifs.seekg(0, std::ios::end);
        std::streamsize file_size = ifs.tellg();
        std::cout << "  File size: " << file_size << " bytes\n";
    }
}

// ── Section 4: std::stringstream ─────────────────────────────────────────────

void section_stringstream()
{
    std::cout << "\n=== Section 4: std::stringstream ===\n";

    // ostringstream: build a string in memory like cout
    std::ostringstream oss;
    oss << "Name: Alice, Score: " << 95.5 << ", Pass: " << std::boolalpha << true;
    std::string result = oss.str();
    std::cout << "ostringstream result: \"" << result << "\"\n";

    // istringstream: parse a string like cin
    std::string data = "42 3.14 hello";
    std::istringstream iss(data);
    int    i;
    double d;
    std::string s;
    iss >> i >> d >> s;
    std::cout << "istringstream parsed: int=" << i << " double=" << d
              << " string=" << s << "\n";

    // Parse a CSV line using istringstream + getline with delim
    std::string csv = "Alice,30,Engineer";
    std::istringstream csv_stream(csv);
    std::string field;
    std::cout << "CSV fields: ";
    while (std::getline(csv_stream, field, ','))
        std::cout << "[" << field << "] ";
    std::cout << "\n";
}

// ── Section 5: Error checking ─────────────────────────────────────────────────

void section_error_checking()
{
    std::cout << "\n=== Section 5: Error Checking ===\n";

    // is_open()
    std::ifstream missing("/nonexistent/path/file.txt");
    std::cout << "is_open(\"/nonexistent\"): " << std::boolalpha << missing.is_open() << "\n";
    if (!missing)   // operator bool checks failbit
        std::cout << "Stream in error state (operator bool)\n";

    // fail(), eof(), bad()
    std::istringstream broken_iss("hello");
    int num;
    broken_iss >> num;          // "hello" cannot be parsed as int -> fail
    std::cout << "After bad >> : fail()=" << broken_iss.fail()
              << " eof()=" << broken_iss.eof()
              << " bad()=" << broken_iss.bad() << "\n";
    broken_iss.clear();         // reset error flags
    std::cout << "After clear(): fail()=" << broken_iss.fail() << "\n";
}

// ── Section 6: RAII and std::filesystem temp path ────────────────────────────

void section_raii_and_fs()
{
    std::cout << "\n=== Section 6: RAII Close + std::filesystem ===\n";

    fs::path tempdir = fs::temp_directory_path();
    std::cout << "temp_directory_path() = " << tempdir << "\n";

    // RAII demo: stream closes automatically
    fs::path fpath = tmp_path("cpp_ex09_raii.txt");
    {
        std::ofstream ofs(fpath);
        ofs << "RAII auto-close demo\n";
        std::cout << "File open inside block: " << ofs.is_open() << "\n";
    } // <- ofs destructor calls close() here
    // We can verify it is no longer open by trying to open the file again
    std::ifstream verify(fpath);
    std::cout << "File readable after block: " << verify.is_open() << "\n";
    verify.close();

    // Clean up temp files
    std::error_code ec;
    fs::remove(tmp_path("cpp_ex09_text.txt"),   ec);
    fs::remove(tmp_path("cpp_ex09_binary.bin"),  ec);
    fs::remove(tmp_path("cpp_ex09_raii.txt"),    ec);
    std::cout << "Temp files removed.\n";

    std::cout << "\nNotes:\n";
    std::cout << "  - Streams close automatically when they go out of scope (RAII).\n";
    std::cout << "  - Always check is_open() or operator bool after opening.\n";
    std::cout << "  - Text mode: newline translation may occur on Windows.\n";
    std::cout << "  - Binary mode (ios::binary): exact byte representation.\n";
    std::cout << "  - seekg/seekp move read/write positions independently in fstream.\n";
    std::cout << "  - stringstream is useful for in-memory formatting and parsing.\n";
}

int main()
{
    section_write_text();
    section_read_text();
    section_binary_io();
    section_stringstream();
    section_error_checking();
    section_raii_and_fs();
    return 0;
}
