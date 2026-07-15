#pragma once

#include <Genode/IO/ResourceLoaderFactory.hpp>
#include <Genode/IO/Json.hpp>
#include <Genode/Entities/ContextAware.hpp>
#include <Genode/Graphics/Font.hpp>
#include <Genode/System/Context.hpp>
#include <Genode/System/Exception.hpp>
#include <Genode/Utilities/StringHelper.hpp>

#include <string>
#include <filesystem>
#include <vector>

namespace Gx
{
    template<typename B, typename R>
    class ResourceLoaderFactory::AdaptorLoader : public ResourceLoader<B>
    {
    public:
        AdaptorLoader() : m_loader(ResourceLoaderFactory::CreateLoader<R>()) {}

        template<typename U = std::string>
        explicit AdaptorLoader(const type_identity_t<U>& id) : m_loader(ResourceLoaderFactory::CreateLoader<R, U>(id)) {}

        ResourceLoader<R>* GetLoader()
        {
            return m_loader.get();
        }

        ResourcePtr<B> LoadFromFile(const std::filesystem::path& fileName, const ResourceContext& ctx) const override
        {
            if (!m_loader)
                return nullptr;

            auto resource = m_loader->LoadFromFile(fileName, ctx);
            if (!resource)
                return nullptr;
                
            return Cast<B>(std::move(resource));
        }
        
        ResourcePtr<B> LoadFromMemory(void* data, std::size_t size, const ResourceContext& ctx) const override
        {
            if (!m_loader)
                return nullptr;

            auto resource = m_loader->LoadFromMemory(data, size, ctx);
            if (!resource)
                return nullptr;
                
            return Cast<B>(std::move(resource));
        }
        
        ResourcePtr<B> LoadFromStream(sf::InputStream& stream, const ResourceContext& ctx) const override
        {
            if (!m_loader)
                return nullptr;

            
            auto resource = m_loader->LoadFromStream(stream, ctx);
            if (!resource)
                return nullptr;
                
            return Cast<B>(std::move(resource));
        }

        ResourcePtr<B> LoadFromJson(const Json& json, const ResourceContext& ctx) const override
        {
            if (!m_loader)
                return nullptr;

            auto resource = m_loader->LoadFromJson(json, ctx);
            if (!resource)
                return nullptr;

            return Cast<B>(std::move(resource));
        }
        
    private:
        std::unique_ptr<ResourceLoader<R>> m_loader;
    };

    template<typename R, typename L>
    void ResourceLoaderFactory::Register()
    {
        Register<R, L>(StringHelper::GetTypeName<R>(false));
    }

    template<typename R, typename L>
    void ResourceLoaderFactory::Register(std::function<std::unique_ptr<ResourceLoader<R>>()> builder)
    {
        Register<R, L>(StringHelper::GetTypeName<R>(false), builder);
    }

    template<typename R, typename L, std::enable_if_t<std::is_base_of_v<ResourceLoader<R>, L>, int>>
    std::unique_ptr<ResourceLoaderFactory::LoaderBuilder<R>> ResourceLoaderFactory::CreateLoaderBuilder()
    {
        auto factory = std::make_unique<LoaderBuilder<R>>();
        factory->Instantiate = []
        {
            std::unique_ptr<L> loader;
            if constexpr (!std::is_default_constructible_v<L>)
            {
                if (m_context)
                    loader = m_context->Instantiate<L>();
                else
                    throw Exception(std::string(typeid(L).name()) + " is not constructible without application context");
            }
            else
                loader = std::make_unique<L>();

            loader->SetResourceInstantiator([] (const ResourceContext& ctx) { return Instantiate<R>(); });
            return loader;
        };
        
        return factory;
    }

    template<typename R, typename L, typename U>
    void ResourceLoaderFactory::Register(const type_identity_t<U>& id)
    {
        auto factory = CreateLoaderBuilder<R, L>();
        factory->OnRemoved = [id] { L::OnRemoved(id); };

        auto builder = factory->Instantiate;

        auto& loaders = m_loaders[typeid(R)];
        loaders[LoaderKey(id)] = std::move(factory);

        L::OnRegistered(id, builder);
    }

    template<typename R, typename L, typename U>
    void ResourceLoaderFactory::Register(const type_identity_t<U>& id, std::function<std::unique_ptr<ResourceLoader<R>>()> builder)
    {
        auto factory = std::make_unique<LoaderBuilder<R>>();
        factory->Instantiate = builder;
        factory->OnRemoved = [id] { L::OnRemoved(id); };

        auto& loaders = m_loaders[typeid(R)];
        loaders[LoaderKey(id)] = std::move(factory);

        L::OnRegistered(id, builder);
    }

    template<typename B, typename ... Rs>
    void ResourceLoaderFactory::Map()
    {
        (Map<B, Rs>(StringHelper::GetTypeName<Rs>(false)), ...);
    }

    template<typename B, typename R, typename U,
        std::enable_if_t<std::is_base_of_v<B, R>, int>>
    void ResourceLoaderFactory::Map(const type_identity_t<U>& id)
    {
        if (const auto it = m_loaders.find(typeid(R)); it != m_loaders.end() && it->second.find(LoaderKey(id)) != it->second.end())
        {
            auto& loaders = m_loaders[typeid(B)];
            loaders[LoaderKey(id)] = std::make_unique<LoaderBuilder<B>>
            (
                [=]
                {
                    return std::make_unique<AdaptorLoader<B, R>>(id);
                }
            );

            return;
        }

        Map<B, R, U>(id, std::function<std::unique_ptr<R>(const ResourceContext&)>{[] (const ResourceContext&)
        {
            return Instantiate<R>();
        }});
    }

    template<typename B, typename R>
    void ResourceLoaderFactory::Map(const std::function<std::unique_ptr<R>(const ResourceContext&)>& instantiator)
    {
        Map<B, R>(StringHelper::GetTypeName<R>(false), instantiator);
    }

    template<typename B, typename R, typename U,
        std::enable_if_t<std::is_base_of_v<B, R>, int>>
    void ResourceLoaderFactory::Map(const type_identity_t<U>& id, const std::function<std::unique_ptr<R>(const ResourceContext&)>& instantiator)
    {
        auto& baseLoaders = m_loaders[typeid(B)];
        if (baseLoaders.find(LoaderKey(StringHelper::GetTypeName<B>(false))) == baseLoaders.end())
            throw Exception(StringHelper::GetTypeName<R>(false) + " cannot be mapped: no loader registered for " + StringHelper::GetTypeName<R>(false) + " nor " + StringHelper::GetTypeName<B>(false));

        auto& loaders = m_loaders[typeid(R)];
        loaders[LoaderKey(id)] = std::make_unique<LoaderBuilder<R>>
        (
            [=]
            {
                auto loader = std::make_unique<AdaptorLoader<R, B>>(id);
                loader->GetLoader()->SetResourceInstantiator(instantiator);

                return loader;
            }
        );

        if (baseLoaders.find(LoaderKey(id)) == baseLoaders.end())
        {
            baseLoaders[LoaderKey(id)] = std::make_unique<LoaderBuilder<B>>
            (
                [=]
                {
                    auto loader = CreateLoader<B>();
                    loader->SetResourceInstantiator(instantiator);
                    return loader;
                }
            );
        }
    }

    template<typename R>
    bool ResourceLoaderFactory::Remove()
    {
        const auto it = m_loaders.find(typeid(R));
        if (it == m_loaders.end())
            return false;

        std::vector<std::function<void()>> callbacks;
        for (const auto& [key, builder] : it->second)
        {
            if (builder->OnRemoved)
                callbacks.push_back(std::move(builder->OnRemoved));
        }

        m_loaders.erase(it);

        for (const auto& callback : callbacks)
            callback();

        return true;
    }

    template<typename R, typename U>
    bool ResourceLoaderFactory::Remove(const type_identity_t<U>& id)
    {
        const auto it = m_loaders.find(typeid(R));
        if (it == m_loaders.end())
            return false;

        auto& loaders = it->second;
        const auto entry = loaders.find(LoaderKey(id));
        if (entry == loaders.end())
            return false;

        auto callback = std::move(entry->second->OnRemoved);
        loaders.erase(entry);

        if (loaders.empty())
            m_loaders.erase(it);

        if (callback)
            callback();

        return true;
    }

    template<typename R>
    std::unique_ptr<ResourceLoader<R>> ResourceLoaderFactory::CreateLoader()
    {
        EnsureDefaultLoadersRegistered();

        const auto it = m_loaders.find(typeid(R));
        if (it == m_loaders.end())
            return nullptr;

        const auto& loaders = it->second;
        if (loaders.empty())
            return nullptr;

        auto loader = loaders.begin();
        if (loaders.size() > 1)
            loader = loaders.find(LoaderKey(StringHelper::GetTypeName<R>(false)));

        if (loader != loaders.end())
        {
            if (auto factory = dynamic_cast<LoaderBuilder<R>*>(loader->second.get());factory)
                return factory->Instantiate();
        }

        return nullptr;
    }

    template<typename R, typename U>
    std::unique_ptr<ResourceLoader<R>> ResourceLoaderFactory::CreateLoader(const type_identity_t<U>& id)
    {
        EnsureDefaultLoadersRegistered();

        const auto it = m_loaders.find(typeid(R));
        if (it == m_loaders.end())
            return nullptr;

        const auto& loaders = it->second;
        if (const auto loader = loaders.find(LoaderKey(id)); loader != loaders.end())
        {
            auto factory = static_cast<LoaderBuilder<R>*>(loader->second.get());
            return factory->Instantiate();
        }

        return nullptr;
    }

    template<typename R>
    std::unique_ptr<R> ResourceLoaderFactory::Instantiate()
    {
        if constexpr (!std::is_default_constructible_v<R>)
        {
            if (!m_context)
                throw Exception(std::string(typeid(R).name()) + " loader is not constructible without application context");

            if constexpr (std::is_base_of_v<ContextAware, R>)
            {
                auto context  = m_context->CreateScope();
                auto instance = context.Instantiate<R>();

                const auto provider = dynamic_cast<ContextAware*>(instance.get());
                provider->SetContext(std::move(context));

                return instance;
            }
            else
            {
                return m_context->Instantiate<R>();
            }
        }
        else
        {
            if constexpr (std::is_base_of_v<ContextAware, R>)
            {
                auto context  = m_context->CreateScope();
                auto instance = context.Instantiate<R>();

                const auto provider = dynamic_cast<ContextAware*>(instance.get());
                provider->SetContext(std::move(context));

                return instance;
            }
            else
            {
                return std::make_unique<R>();
            }
        }
    }
}
