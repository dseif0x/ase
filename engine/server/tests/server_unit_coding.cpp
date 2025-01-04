/*
* Copyright 2024 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */

#include <algorithm>
#include <gtest/gtest.h>

#include "decoder.h"
#include "encoder.h"
#include "utils.h"
#include "TestCoding.h"




TEST(UnitTests_Coding, read_write_bytes) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0xaa, 0xbb, 0xcc};

    decoder._add_to_buffer(data);
    auto res = decoder.get_bytes(3);

    ASSERT_TRUE(vec_equ(data, res));
    ASSERT_TRUE(decoder.is_empty());
    decoder._add_to_buffer(data);
    decoder.clear_buffer(0);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    ASSERT_TRUE(encoder.is_empty());
    encoder._get_bytes(8); // empty, nothing happens
    encoder._add_to_buffer({0x44});
    ASSERT_FALSE(encoder.is_empty());
    encoder.clear_buffer();
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, string_1) {
    TestGenericDecoder decoder{};

    std::string check = "Hello";
    std::vector<unsigned char> data{0x05, 'H', 'e', 'l', 'l', 'o'};

    decoder._add_to_buffer(data);
    auto decres = decoder.to_string();

    ASSERT_TRUE(decres.compare(check) == 0);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_string(check);

    auto encres = encoder._get_bytes(6);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, string_2) {
    TestGenericDecoder decoder{};

    std::string check = "";
    std::vector<unsigned char> data{0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_string();

    ASSERT_TRUE(res.compare(check) == 0);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_string(check);

    auto encres = encoder._get_bytes(1);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, int_1) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    int check = 5;
    std::vector<unsigned char> data{0, 0, 0, 0x05};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_int(check);

    auto encres = encoder._get_bytes(4);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, int_2) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    int check = 0;
    std::vector<unsigned char> data{0, 0, 0, 0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_int(check);

    auto encres = encoder._get_bytes(4);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, int_3) {
    ASSERT_TRUE(sizeof(int) == 4);
    TestGenericDecoder decoder{};

    int val = -1;
    std::vector<unsigned char> data{get_nth_byte(val, 3), get_nth_byte(val, 2), get_nth_byte(val, 1), get_nth_byte(val, 0)};

    decoder._add_to_buffer(data);
    auto res = decoder.to_int();

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_int(val);

    auto encres = encoder._get_bytes(4);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, double_1) {
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

    TestGenericEncoder encoder{};
    encoder.from_double(val);

    auto encres = encoder._get_bytes(8);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, double_2) {
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

    TestGenericEncoder encoder{};
    encoder.from_double(val);

    auto encres = encoder._get_bytes(8);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, double_3) {
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

    TestGenericEncoder encoder{};
    encoder.from_double(val);

    auto encres = encoder._get_bytes(8);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, acknak) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{0xFF, 0xFF, 0x00, 0xFe, 0x00};

    decoder._add_to_buffer(data);

    ASSERT_TRUE(decoder.to_acknak() == 1);
    ASSERT_TRUE(decoder.to_acknak() == 1);
    ASSERT_TRUE(decoder.to_acknak() == 0);
    ASSERT_TRUE(decoder.to_acknak() == -1);
    ASSERT_TRUE(decoder.to_acknak() == 0);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_ack();
    encoder.from_ack();
    encoder.from_nak();
    encoder._add_to_buffer({0xfe});
    encoder.from_nak();

    auto encres = encoder._get_bytes(5);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, bool_1) {
    TestGenericDecoder decoder{};

    std::vector<unsigned char> data{true, false, true};

    decoder._add_to_buffer(data);

    ASSERT_TRUE(decoder.to_bool());
    ASSERT_TRUE(!decoder.to_bool());
    ASSERT_TRUE(decoder.to_bool());
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_bool(true);
    encoder.from_bool(false);
    encoder.from_bool(true);

    auto encres = encoder._get_bytes(3);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, unsigned_1) {
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

    TestGenericEncoder encoder{};
    encoder.from_unsigned(8, val);

    auto encres = encoder._get_bytes(8);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, unsigned_2) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long val = 4327490;
    std::vector<unsigned char> data{0x42, 0x08, 0x42};

    decoder._add_to_buffer(data);
    auto res = decoder.to_unsigned(3);

    ASSERT_TRUE(res == val);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_unsigned(3, val);

    auto encres = encoder._get_bytes(3);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, expanding_unsigned_1) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long check = 66;
    std::vector<unsigned char> data{0x42};

    decoder._add_to_buffer(data);
    auto res = decoder.to_expanding_unsigned();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_expanding_unsigned(check);

    auto encres = encoder._get_bytes(1);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, expanding_unsigned_2) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long check = 17488823362;
    std::vector<unsigned char> data{0xF4, 0x12, 0x6A, 0x44, 0x42};

    decoder._add_to_buffer(data);
    auto res = decoder.to_expanding_unsigned();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_expanding_unsigned(check);

    auto encres = encoder._get_bytes(5);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, expanding_unsigned_3) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long check = 4909524494427243586;
    std::vector<unsigned char> data{0xFF, 0x44, 0x22, 0x22, 0x84, 0x12, 0x6A, 0x44, 0x42};

    decoder._add_to_buffer(data);
    auto res = decoder.to_expanding_unsigned();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_expanding_unsigned(check);

    auto encres = encoder._get_bytes(9);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, expanding_unsigned_4) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long check = 0;
    std::vector<unsigned char> data{0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_expanding_unsigned();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_expanding_unsigned(check);

    auto encres = encoder._get_bytes(1);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, expanding_unsigned_5) {
    ASSERT_TRUE(sizeof(unsigned long long) == 8);
    TestGenericDecoder decoder{};

    unsigned long long check = 1073741824;
    std::vector<unsigned char> data{0xf0, 0x40, 0x00, 0x00, 0x00};

    decoder._add_to_buffer(data);
    auto res = decoder.to_expanding_unsigned();

    ASSERT_TRUE(res == check);
    ASSERT_TRUE(decoder.is_empty());

    TestGenericEncoder encoder{};
    encoder.from_expanding_unsigned(check);

    auto encres = encoder._get_bytes(5);
    print_vec(data);
    print_vec(encres);
    ASSERT_TRUE(vec_equ(encres, data));
    ASSERT_TRUE(encoder.is_empty());
}

TEST(UnitTests_Coding, general_1) {
    std::string python_rep = "[[1, 2], [3, 4, 5]]";
    std::string encoded_rep = "a0 02 82 02 00000001 00000002 82 03 00000003 00000004 00000005";
    std::vector<unsigned char> data = make_hex_vector(encoded_rep.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    decoder._add_to_buffer(data);
    auto val = decoder.to_general();
    ASSERT_TRUE(val);
    ASSERT_TRUE(decoder.is_empty());
    std::string pyres = CopiedValue::get_python_style(*val);
    val->print();
    std::cout << std::endl << pyres << std::endl;

    ASSERT_TRUE(python_rep.compare(pyres) == 0);
    encoder.from_general(*val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(encoded_rep));
    ASSERT_TRUE(encoder.is_empty());
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, general_2) {
    std::string python_rep = "[[\"AB\", \"CD\"], 4]";
    std::string encoded_rep = "a0 02 83 02 02 4142 02 4344 02 00000004";
    std::vector<unsigned char> data = make_hex_vector(encoded_rep.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    decoder._add_to_buffer(data);
    auto val = decoder.to_general();
    ASSERT_TRUE(val);
    ASSERT_TRUE(decoder.is_empty());
    std::string pyres = CopiedValue::get_python_style(*val);
    val->print();
    std::cout << std::endl << pyres << std::endl;

    ASSERT_TRUE(python_rep.compare(pyres) == 0);
    encoder.from_general(*val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(encoded_rep));
    ASSERT_TRUE(encoder.is_empty());
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, general_3) {
    std::string python_rep = "((\"A\", 1), (\"B\", [2, 3]))";
    std::string encoded_rep = "60 43 01 41 02 00000001 43 01 42 82 02 00000002 00000003";
    std::vector<unsigned char> data = make_hex_vector(encoded_rep.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    decoder._add_to_buffer(data);
    auto val = decoder.to_general();
    ASSERT_TRUE(val);
    ASSERT_TRUE(decoder.is_empty());
    std::string pyres = CopiedValue::get_python_style(*val);
    val->print();
    std::cout << std::endl << pyres << std::endl;

    ASSERT_TRUE(python_rep.compare(pyres) == 0);
    encoder.from_general(*val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(encoded_rep));
    ASSERT_TRUE(encoder.is_empty());
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, general_4) {
    std::string python_rep = "{a1: [1, 2, 3], a2: {b1: \"lol\", b2: (\"4\", [5, \"6\"])}}";
    std::string python_rep2 = "{a1: [1, 2, 3], a2: {b2: (\"4\", [5, \"6\"]), b1: \"lol\"}}";
    std::string python_rep3 = "{a2: {b1: \"lol\", b2: (\"4\", [5, \"6\"])}, a1: [1, 2, 3]}";
    std::string python_rep4 = "{a2: {b2: (\"4\", [5, \"6\"]), b1: \"lol\"}, a1: [1, 2, 3]}";
    std::string encoded_rep = "10 02 02 6131 82 03 00000001 00000002 00000003 02 6132 10 02 02 6231 03 03 6c6f6c 02 6232 43 01 34 a0 02 02 00000005 03 01 36";
    std::vector<unsigned char> data = make_hex_vector(encoded_rep.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    decoder._add_to_buffer(data);
    auto val = decoder.to_general();
    ASSERT_TRUE(val);
    ASSERT_TRUE(decoder.is_empty());
    std::string pyres = CopiedValue::get_python_style(*val);
    std::cout << python_rep << std::endl << pyres << std::endl;

    ASSERT_TRUE(
        python_rep.compare(pyres) == 0 ||
        python_rep2.compare(pyres) == 0 ||
        python_rep3.compare(pyres) == 0 ||
        python_rep4.compare(pyres) == 0);
    encoder.from_general(*val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(encoded_rep));
    ASSERT_TRUE(encoder.is_empty());

    //weaker comparison, as only the existence of the correct bytes is checked instead of one of the possible correct sequences
    std::ranges::sort(data);
    std::ranges::sort(encoded);
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, general_all_values) {
    std::string python_rep = "[false, 1, \"2\", 3.000, sem<4>, ref<5>, {6: \"7\"}, (8, 9)]";
    std::string encoded_rep = "a0 08 01 00 02 00000001 03 01 32 04 4008000000000000 05 00000004 06 01 35 10 01 01 36 03 01 37 42 00000008 02 00000009";
    std::vector<unsigned char> data = make_hex_vector(encoded_rep.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    decoder._add_to_buffer(data);
    auto val = decoder.to_general();
    ASSERT_TRUE(val);
    ASSERT_TRUE(decoder.is_empty());
    std::string pyres = CopiedValue::get_python_style(*val);
    val->print();
    std::cout << std::endl << python_rep << std::endl;

    ASSERT_TRUE(python_rep.compare(pyres) == 0);
    encoder.from_general(*val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(encoded_rep));
    ASSERT_TRUE(encoder.is_empty());
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, list_t_1) {
    // example from TCP2 Protocol document chapter tcp_list
    std::string result = "10 02 02 6231 03 03 6c6f6c 02 6232 48"; // {b1: "lol"; b2: <pair>}
    std::vector<unsigned char> data = make_hex_vector(result.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    std::unordered_map<std::string, std::pair<ValueType, std::variant<std::monostate, bool, int, std::string, double>>> mapData = {
        {"b1", {ValueType::STRING, "lol"}},
        {"b2", {ValueType::PAIR, std::monostate{}}}
    };

    DB::list_t val = mapData;

    encoder.from_list_t(val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(result));
    ASSERT_TRUE(encoder.is_empty());
    std::ranges::sort(data);
    std::ranges::sort(encoded);
    ASSERT_TRUE(vec_equ(data, encoded));
}

TEST(UnitTests_Coding, list_t_2) {
    std::string result = "10 00"; // {}
    std::vector<unsigned char> data = make_hex_vector(result.c_str());

    TestGenericDecoder decoder{};
    TestGenericEncoder encoder{};

    std::unordered_map<std::string, std::pair<ValueType, std::variant<std::monostate, bool, int, std::string, double>>> mapData = {
    };

    DB::list_t val = mapData;

    encoder.from_list_t(val);
    auto encoded = encoder._get_bytes(count_hex_valid_chars(result));
    ASSERT_TRUE(encoder.is_empty());
    ASSERT_TRUE(vec_equ(data, encoded));
}