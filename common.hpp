#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <string>
#include <cstdint>

typedef std::uint8_t  u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
static_assert(sizeof(u8)  == 1, "sizeof u8");
static_assert(sizeof(u16) == 2, "sizeof u16");
static_assert(sizeof(u32) == 4, "sizeof u32");
static_assert(sizeof(u64) == 8, "sizeof u64");

const float PI = 3.14159265359f;
const float TwoPI = 2.f * PI;
const float PI2 = PI / 2.f;

// Notes:
// C++11 std::string requires contiguous underlying storage,
// passing pointers (e.g., &buffer[0]) is safe.
// 
// std::string == std::basic_string<char>
// std::string is convenient to parse with stringstream
typedef std::string ByteBuffer;

ByteBuffer getFileContents(const std::string& filename);
u64 getFileModificationTime(const std::string& filename);

#endif
