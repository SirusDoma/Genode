#pragma once

namespace Gx
{
    class Module
    {
    public:
        virtual ~Module() = 0;
    };

    inline Module::~Module() = default;
}
