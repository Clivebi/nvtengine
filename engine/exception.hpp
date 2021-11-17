#pragma once
#include <stdexcept>
#include <string>
namespace Interpreter {
class RuntimeException : public  std::runtime_error {
    public:
    explicit RuntimeException(std::string msg):std::runtime_error(msg){}
};
} // namespace Interpreter