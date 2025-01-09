#ifndef TESTCODING_H
#define TESTCODING_H

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

class TestGenericEncoder : public GenericEncoder {
public:
    void _add_to_buffer(std::vector<unsigned char> block) {
        add_to_buffer(block);
    }

    std::vector<unsigned char> _get_bytes(size_t bytes) {
        auto data_pair = get_bytes(bytes);
        if (data_pair.second <= 0) {
            return {};
        }

        return std::vector<unsigned char>(data_pair.first.get(), data_pair.first.get() + data_pair.second);
    }

    std::vector<unsigned char> _get_all_bytes() {
        std::vector<unsigned char> res{};
        while (!this->is_empty()) {
            res.push_back(this->_get_bytes(1)[0]);
        }
        return res;
    }

    bool write_data() override {
        return true;
    }


};

#endif //TESTCODING_H
