#include "decoder.h"

#include <stdexcept>
#include <fstream>

Decoder::Decoder(std::string file_in) : file(file_in) {
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_in);
    }
}

Inst Decoder::next() {
    std::string line;
    if(std::getline(file, line)) {
        Inst inst;
        return inst;
    }else {
        throw std::runtime_error("No more instructions");
    }
}