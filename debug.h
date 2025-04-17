
#pragma once

extern void missing(char const* file_name, int const line);

#define MISSING() missing(__FILE__,__LINE__)
