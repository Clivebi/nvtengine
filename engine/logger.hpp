#pragma once

#include <iostream>

#define LOG(msg)  (std::cout<<__FUNCTION__<<":"<<__LINE__<<"\t"<<msg<<std::endl) 