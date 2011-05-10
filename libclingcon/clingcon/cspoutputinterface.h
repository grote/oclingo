#pragma once

#include <gringo/lparseconverter.h>

namespace Clingcon
{

class CSPOutputInterface : public LparseConverter
{

public:
    CSPOutputInterface(std::ostream *out, bool b) : LparseConverter(out,b)
    {}

    virtual uint32_t symbol(const std::string& name, bool freeze) = 0;

};

}

