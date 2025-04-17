#include "decoder.h"

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <memory>

Inst Decoder::next() {
    // std::string line;
    // if(std::getline(file, line)) {
    //     std::cerr << line << std::endl;
    //     Inst inst;
    //     return inst;
    // }else {
    //     throw std::runtime_error("No more instructions");
    // }

    return Inst();
}