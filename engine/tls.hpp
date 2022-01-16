
#pragma once
#include <stdio.h>

namespace Interpreter {
class TLS {
public:
    static size_t Allocate();
    static void* GetValue(size_t index);
    static void  SetValue(size_t index,void* value);
};
}; // namespace Interpreter