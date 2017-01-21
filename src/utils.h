#pragma once

#include <iostream>

#ifdef NDEBUG
#    define ERROR if(false) std::cerr
#else
#    define ERROR std::cerr
#endif