#pragma once

#include <Genode/System/Context.hpp>
#include <Genode/System/Exception.hpp>

#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <utility>

namespace Gx
{
    template <typename TModule>
    std::enable_if_t<std::is_base_of_v<Module, TModule>, void>
    Application::Install()
    {
        if (FindModule<TModule>())
        {
            throw InvalidOperationException(
                std::string("Module is already installed: ") + typeid(TModule).name()
            );
        }

        m_context.Provide<TModule>(Context::Scope::Singleton);
        AddModule<TModule>();
    }

    template <typename TInterface, typename TConcrete>
    std::enable_if_t<
        std::is_base_of_v<Module, TInterface> &&
        std::is_base_of_v<TInterface, TConcrete>, void
    >
    Application::Install()
    {
        if (FindModule<TInterface>())
        {
            throw InvalidOperationException(
                std::string("Module is already installed: ") + typeid(TInterface).name()
            );
        }

        m_context.Provide<TInterface, TConcrete>(Context::Scope::Singleton);
        AddModule<TInterface>();
    }

    template <typename TModule>
    std::enable_if_t<std::is_base_of_v<Module, TModule>, void>
    Application::Install(std::function<std::unique_ptr<TModule>(const Context&)> builder)
    {
        if (FindModule<TModule>())
        {
            throw InvalidOperationException(
                std::string("Module is already installed: ") + typeid(TModule).name()
            );
        }

        m_context.Provide<TModule>(std::move(builder), Context::Scope::Singleton);
        AddModule<TModule>();
    }

    template <typename TModule>
    std::enable_if_t<std::is_base_of_v<Module, TModule>, bool>
    Application::Uninstall()
    {
        const auto key = std::type_index(typeid(TModule));
        for (auto it = m_modules.begin(); it != m_modules.end(); ++it)
        {
            if (it->Type == key)
                return m_modules.erase(it) != m_modules.end();
        }

        return false;
    }

    template <typename TModule>
    std::enable_if_t<std::is_base_of_v<Module, TModule>, TModule&>
    Application::GetModule() const
    {
        if constexpr (std::is_same_v<TModule, Context>)
        {
            return m_context;
        }
        else
        {
            if (FindModule<TModule>())
                return m_context.Require<TModule>();

            throw InvalidOperationException(
                std::string("Module is not installed: ") + typeid(TModule).name()
            );
        }
    }

    template <typename TModule>
    void Application::AddModule()
    {
        auto& module = m_context.Require<TModule>();
        m_modules.push_back(InternalModule{
            std::type_index(typeid(TModule)),
            &module,
            dynamic_cast<Renderable*>(&module),
            dynamic_cast<Updatable*>(&module),
            dynamic_cast<Inputable*>(&module)
        });
    }

    template <typename TModule>
    TModule* Application::FindModule() const
    {
        const auto key = std::type_index(typeid(TModule));
        for (const auto& entry : m_modules)
        {
            if (entry.Type == key)
                return static_cast<TModule*>(entry.Instance);
        }

        return nullptr;
    }
}
