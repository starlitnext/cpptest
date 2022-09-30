/*
 * @Author: silentwind vipxxq@foxmail.com
 * @Date: 2022-09-30 11:48:00
 * @LastEditors: silentwind vipxxq@foxmail.com
 * @LastEditTime: 2022-09-30 16:36:15
 * @Description: 按bit的方式读写buffer
 */
#ifndef MEMORY_BIT_STREAM_HPP
#define MEMORY_BIT_STREAM_HPP

#include <cstdint>
#include <cstdlib>
#include <string>
#include <cstring>

inline uint16_t ByteSwap2(uint16_t in_data)
{
    return (in_data >> 8) | (in_data << 8);
}

inline uint32_t ByteSwap4(uint32_t in_data)
{
    return ((in_data >> 24) & 0x000000ff) |
        ((in_data >> 8) & 0x0000ff00) |
        ((in_data << 8) & 0x00ff0000) |
        ((in_data << 24) & 0xff000000);
}

inline uint32_t ByteSwap8(uint64_t in_data)
{
    return ((in_data >> 56) & 0x00000000000000ff) |
        ((in_data >> 40) & 0x000000000000ff00) |
        ((in_data >> 24) & 0x0000000000ff0000) |
        ((in_data >> 8)  & 0x00000000ff000000) |
        ((in_data << 8)  & 0x000000ff00000000) |
        ((in_data << 24) & 0x0000ff0000000000) |
        ((in_data << 40) & 0x00ff000000000000) |
        ((in_data << 56) & 0xff00000000000000);
}

template<typename From, typename To>
class TypeAliaser
{
public:
    TypeAliaser(From from) : from_(from) {}
    To& Get() { return to_; }

    union
    {
        From from_;
        To to_;
    };
};

template<typename T, size_t Size> class ByteSwapper;

template<typename T>
class ByteSwapper<T, 2>
{
public:
    T Swap(T in_data) const
    {
        uint16_t result = ByteSwap2(TypeAliaser<T, uint16_t>(in_data).Get());
        return TypeAliaser<uint16_t, T>(result).Get();
    }
};

template<typename T>
class ByteSwapper<T, 4>
{
public:
    T Swap(T in_data) const
    {
        uint32_t result = ByteSwap4(TypeAliaser<T, uint32_t>(in_data).Get());
        return TypeAliaser<uint32_t, T>(result).Get();
    }
};

template<typename T>
class ByteSwapper<T, 8>
{
public:
    T Swap(T in_data) const
    {
        uint64_t result = ByteSwap8(TypeAliaser<T, uint64_t>(in_data).Get());
        return TypeAliaser<uint64_t, T>(result).Get();
    }
};

template<typename T>
T ByteSwap(T in_data)
{
    return ByteSwapper<T, sizeof(T)>().Swap(in_data);
}

class OutputMemoryBitStream
{
public:
    OutputMemoryBitStream() :
        buffer_(nullptr),
        head_(0)
    {
        // 默认分配1500 byte
        ReallocBuffer(1500 * 8);
    }
    ~OutputMemoryBitStream() { std::free(buffer_); }

    void WriteBits(uint8_t in_data, size_t bit_count);
    void WriteBits(const void* in_data, size_t bit_count);

    const char* GetBufferPtr() const { return buffer_; }
    uint32_t GetBitLength() const { return head_; }
    uint32_t GetByteLength() const { return (head_ + 7) >> 3; }

    void WriteBytes(const void* in_data, uint32_t byte_count) { WriteBits(in_data, byte_count << 3); }

    template<typename T>
    void Write(T in_data, uint32_t bit_count = sizeof(T) * 8)
    {
        static_assert(
            std::is_arithmetic<T>::value ||
            std::is_enum<T>::value,
            "Generic Write only supports primitive data types"
        );
        // 字节序问题, WriteBits 按照小端的方式来写入(先写低位，再写高位)
        // 例如一个数字 0x12345678
        // 内存地址 0x01000 0x01001 0x01002 0x01003
        // 小端     0x78    0x56    0x34    0x12
        // 大端     0x12    0x34    0x56    0x78
        // 假设我们只想要写入8bit，则小端写入 0x78 而大端写入 0x12
        // 要支持大端的话，这里需要先交换bit再写
    #ifdef BIG_ENDIAN
        in_data = ByteSwap(in_data);
    #endif // BIG_ENDIAN
        WriteBits(&in_data, bit_count);
    }

    void Write(bool in_data) { WriteBits(&in_data, 1); }

    void Write(const std::string& in_str)
    {
        uint32_t element_count = static_cast<uint32_t>(in_str.size());
        Write(element_count);
        for (const auto& element : in_str)
        {
            Write(element);
        }
    }

private:
    void ReallocBuffer(uint32_t in_capacity);
    
private:
    char*           buffer_;
    uint32_t        head_;
    uint32_t        capacity_;
};

void OutputMemoryBitStream::ReallocBuffer(uint32_t in_capacity)
{
    if (buffer_ == nullptr)
    {
        buffer_ = static_cast<char*>(std::malloc(in_capacity >> 3));
        ::memset(buffer_, 0, in_capacity >> 3);
    }
    else
    {
        char* tmp_buffer = static_cast<char*>(std::malloc(in_capacity >> 3));
        ::memset(tmp_buffer, 0, in_capacity >> 3);
        ::memcpy(tmp_buffer, buffer_, in_capacity >> 3);
        std::free(buffer_);
        buffer_ = tmp_buffer;
    }

    capacity_ = in_capacity;
}

void OutputMemoryBitStream::WriteBits(uint8_t in_data, size_t bit_count)
{
    uint32_t next_bit_head = head_ + static_cast<uint32_t>(bit_count);

    if (next_bit_head > capacity_)
    {
        ReallocBuffer(std::max(capacity_ * 2, head_));
    }

    // 计算head当前byte的位置以及在这个byte中bit的位置
    uint32_t byte_offset = head_ >> 3;
    uint32_t bit_offset = head_ & 0x7;

    uint8_t current_mask = ~(0xff << bit_offset);
    buffer_[byte_offset] = (buffer_[byte_offset] & current_mask) | (in_data << bit_offset);

    uint32_t bits_free_this_byte = 8 - bit_offset;
    if (bits_free_this_byte < bit_count)
    {
        // 把剩余数据放到下一个byte中
        buffer_[byte_offset + 1] = in_data >> bits_free_this_byte;
    }

    head_ = next_bit_head;
}

void OutputMemoryBitStream::WriteBits(const void* in_data, size_t bit_count)
{
    const char* src_byte = static_cast<const char*>(in_data);
    while (bit_count > 8)
    {
        WriteBits(*src_byte, 8);
        ++ src_byte;
        bit_count -= 8;
    }
    // 写入剩余的bit
    if (bit_count > 0)    
    {
        WriteBits(*src_byte, bit_count);
    }
}


class InputMemoryBitStream
{
public:
    InputMemoryBitStream(char* in_buffer, uint32_t bit_count):
        buffer_(in_buffer),
        capacity_(bit_count),
        head_(0)
    {
    }

    const char* GetBufferPtr() const { return buffer_; }
    uint32_t GetRemainingBitCount() const { return capacity_ - head_; }

    void ReadBits(uint8_t& out_data, uint32_t bit_count);
    void ReadBits(void* out_data, uint32_t bit_count);

    void ReadBytes(void* out_data, uint32_t byte_count) { ReadBits(out_data, byte_count << 3); }

    template<typename T>
    void Read(T& in_data, uint32_t bit_count = sizeof(T) * 8)
    {
        static_assert(
            std::is_arithmetic<T>::value ||
            std::is_enum<T>::value,
            "Generic Write only supports primitive data types"
        );
        // 字节序问题, ReadBits 按照小端的方式来写入(先读低位，再读高位)
        // 低位放在低地址，高位放在高地址
        // 如果系统是大端，则需要交换一下字节序
        ReadBits(&in_data, bit_count);
#ifdef BIG_ENDIAN
        in_data = ByteSwap(in_data);
#endif // BIG_ENDIAN
    }

    void Read(bool& out_data) { ReadBits(&out_data, 1); }

    void Read(std::string& in_str)
    {
        uint32_t element_count;
        Read(element_count);
        in_str.resize(element_count);
        for (auto& element : in_str)
        {
            Read(element);
        }
    }

    void ResetToCapacity(uint32_t byte_capacity) { capacity_ = byte_capacity << 3; head_ = 0; }

private:
    char* buffer_;
    uint32_t capacity_;
    uint32_t head_;
};

void InputMemoryBitStream::ReadBits(uint8_t& out_data, uint32_t bit_count)
{
    uint32_t byte_offset = head_ >> 3;
    uint32_t bit_offset = head_ & 0x7;
    out_data = static_cast<uint8_t>(buffer_[byte_offset]) >> bit_offset;

    uint32_t bits_free_this_byte = 8 - bit_offset;
    if (bit_offset < bit_count)
    {
        out_data |= static_cast<uint8_t>(buffer_[byte_offset + 1]) << bits_free_this_byte;
    }

    out_data &= (~(0x00ff << bit_count));

    head_ += bit_count;
}

void InputMemoryBitStream::ReadBits(void* out_data, uint32_t bit_count)
{
    uint8_t* dest_byte = reinterpret_cast<uint8_t*>(out_data);
    while (bit_count > 8)
    {
        ReadBits(*dest_byte, 8);
        ++dest_byte;
        bit_count -= 8;
    }
    if (bit_count > 0)
    {
        ReadBits(*dest_byte, bit_count);
    }
}

#endif // !MEMORY_BIT_STREAM_HPP