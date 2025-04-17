#pragma once

#include <string>
#include <fstream>

struct Inst {
    
};

class Decoder {
    std::ifstream file;
public:
    Decoder(std::string file_in);
    Inst next();
};