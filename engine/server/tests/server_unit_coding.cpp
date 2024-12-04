
#include <gtest/gtest.h>

#include "decoder.h"

class TestGenericDecoder : public GenericDecoder {
public:
    std::vector<unsigned char> get_bytes(size_t bytes) override {
        std::vector<unsigned char> res;
        this->buffer_lock.lock();
        int original_bytes = bytes;
        while (true) {
            if (bytes == 0 || this->buffer.empty()) break;
            auto bytes_to_copy = std::min(bytes, this->buffer.front().size() - this->start);
            res.insert(res.end(), this->buffer.front().begin() + this->start,
                this->buffer.front().begin() + this->start + bytes_to_copy);
            bytes -= bytes_to_copy;
            this->start += bytes_to_copy;
            if (this->buffer.front().size() <= this->start) {
                this->buffer.pop_front();
                this->start = 0;
            }
        }
        this->buffer_lock.unlock();
        if (res.size() != original_bytes) throw std::invalid_argument("Not enough data to extract given bytes.");
        return res;
    }

    void _add_to_buffer(std::vector<unsigned char> block) {
        add_to_buffer(block);
    }

    void read_data() override {
        return;
    }
};


template <typename T>
bool vec_equ(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    if (vec1.size() != vec2.size()) {
        return false;
    }

    return std::equal(vec1.begin(), vec1.end(), vec2.begin());
}

template <typename T>
unsigned char get_nth_byte(T val, int n) {
    return (unsigned char)((*(unsigned long long*)&val)>>(n*8));
}


void print_vec(std::vector<unsigned char> &vec) {
    std::cout << "vek:";
    for (auto &x : vec) std::cout << (int)x << ", ";
    std::cout << std::endl;
}

TEST(UnitTests_Decoder, read_write_bytes) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0xaa, 0xbb, 0xcc};

    decoder._add_to_buffer(data);
    auto res = decoder.get_bytes(3);

    ASSERT_TRUE(vec_equ(data, res));
    ASSERT_TRUE(decoder.is_empty());
    decoder._add_to_buffer(data);
    decoder.clear_buffer(0);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_string_1) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0x05, 'H', 'e', 'l', 'l', 'o'};

    decoder._add_to_buffer(data);
    auto res = decoder.to_string();

    ASSERT_TRUE(res.compare("Hello") == 0);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_string_2) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_string();

    ASSERT_TRUE(res.compare("") == 0);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_int_1) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0, 0, 0, 0x05};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == 5);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_int_2) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0, 0, 0, 0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == 0);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_int_3) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    int val = -1;
    std::vector<unsigned char> data{get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_double_1) {
    ASSERT_TRUE(sizeof(double) == 8);
    TestGenericDecoder decoder{};

    double val = 129;
    std::vector<unsigned char> data{
        get_nth_byte(val, 7), get_nth_byte(val, 6), get_nth_byte(val, 5), get_nth_byte(val, 4),
        get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_double();

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_double_2) {
    ASSERT_TRUE(sizeof(double) == 8);
    TestGenericDecoder decoder{};

    double val = 0;
    std::vector<unsigned char> data{
        get_nth_byte(val, 7), get_nth_byte(val, 6), get_nth_byte(val, 5), get_nth_byte(val, 4),
        get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_double();

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_double_3) {
    ASSERT_TRUE(sizeof(double) == 8);
    TestGenericDecoder decoder{};

    double val = -1654.98451;
    std::vector<unsigned char> data{
        get_nth_byte(val, 7), get_nth_byte(val, 6), get_nth_byte(val, 5), get_nth_byte(val, 4),
        get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_double();

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_acknak) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0xFF, 0xFF, 0x00, 0xFe, 0x00};

    decoder._add_to_buffer(data);

    ASSERT_TRUE(decoder.to_acknak() == 1);
    ASSERT_TRUE(decoder.to_acknak() == 1);
    ASSERT_TRUE(decoder.to_acknak() == 0);
    ASSERT_TRUE(decoder.to_acknak() == -1);
    ASSERT_TRUE(decoder.to_acknak() == 0);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_bool) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{true, false, true};

    decoder._add_to_buffer(data);

    ASSERT_TRUE(decoder.to_bool());
    ASSERT_TRUE(!decoder.to_bool());
    ASSERT_TRUE(decoder.to_bool());
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_unsigned_1) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long val = 1235488456;
    std::vector<unsigned char> data{
        get_nth_byte(val, 7), get_nth_byte(val, 6), get_nth_byte(val, 5), get_nth_byte(val, 4),
        get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_unsigned(8);

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());
}

TEST(UnitTests_Decoder, to_unsigned_2) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0x42, 0x08, 0x42};

    decoder._add_to_buffer(data);
    auto res = decoder.to_unsigned(3);

    ASSERT_TRUE(res == 4327490);
    ASSERT_TRUE(decoder.is_empty());
}