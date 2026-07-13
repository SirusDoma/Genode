#pragma once

namespace Gx
{
    class Context;
    class ContextAware
    {
    public:
        [[nodiscard]] virtual Context& GetContext() = 0;
        virtual void SetContext(Context&& context) = 0;

    protected:
        virtual ~ContextAware() = default;
    };
}