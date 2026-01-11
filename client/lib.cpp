#include <iostream>

#include "hopper/hopper.h"

extern "C" {
void hello() { std::cout << "hello world\n" << std::endl; }
}
