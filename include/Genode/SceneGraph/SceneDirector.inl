#pragma once

#include <Genode/System/Exception.hpp>
#include <Genode/IO/ResourceContext.hpp>
#include <Genode/Utilities/StringHelper.hpp>

#include <tuple>

namespace Gx
{
    template<typename Ctx>
    SceneDirector::ScenePresentationData::ScenePresentationData(
        const std::type_index type,
        const std::function<void(Scene&)>& initializer,
        const Ctx& context,
        const SceneDeserializer<Scene>& deserializer
    ) :
        Type(type),
        Initializer(initializer),
        Context(std::make_shared<Ctx>(context)),
        Deserializer(deserializer)
    {
        Context->Unbind();
    }

    template<typename T, typename... Args>
    SceneDirector::SceneDirector(RenderSurface& surface, T& scene, Args&&... args) :
        SceneDirector::SceneDirector(surface)
    {
        Present<T>(scene, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    SceneDirector::SceneDirector(Application& app, T& scene, Args&&... args) :
        SceneDirector::SceneDirector(app)
    {
        Present<T>(scene, std::forward<Args>(args)...);
    }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Scene, T>, void>
    SceneDirector::Register()
    {
        m_deserializers[typeid(T)] = SceneGenericDeserializer([this] (const ResourceContext&) -> ResourcePtr<Scene>
        {
            auto context = GetContext().CreateScope();
            auto scene   = context.Instantiate<T>();

            scene->SetContext(context);
            return Cast<Scene>(std::move(scene));
        });
    }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Scene, T>, void>
    SceneDirector::Register(const SceneDeserializer<T>& deserializer)
    {
        m_deserializers[typeid(T)] = SceneGenericDeserializer([deserializer] (const ResourceContext& ctx)
        {
            return Cast<Scene>(deserializer(ctx));
        });
    }

    template<typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<Scene, T>, void>
    SceneDirector::Present(T& scene, Args&&... args)
    {
        m_deserializers[typeid(T)] = SceneGenericDeserializer([&scene] (const ResourceContext&) -> ResourcePtr<Scene>
        {
            return Cast<Scene>(ResourcePtr<T>(&scene, [] (auto) {}));
        });

        return Present<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename Ctx, typename... Args>
    std::enable_if_t<std::is_base_of_v<Scene, T> && std::is_base_of_v<ResourceContext, Ctx>, void>
    SceneDirector::Present(const Ctx& context, Args&&... args)
    {
        ResourcePtr<Scene> scene = nullptr;
        SceneGenericDeserializer deserializer = nullptr;

        if (const auto it = m_deserializers.find(typeid(T)); it != m_deserializers.end())
        {
            deserializer = it->second;
            scene        = deserializer(context);
        }

        if (!scene)
        {
            if constexpr (std::is_default_constructible_v<T>)
            {
                Register<T>();
                return Present<T>(context, std::forward<Args>(args)...);
            }
        }

        if (!scene)
            throw ArgumentException(StringHelper::GetTypeName(typeid(T)) + " is not registered");

        std::function<void(Scene&)> initializer = nullptr;
        if constexpr(sizeof...(Args) > 0)
        {
            if constexpr((std::is_copy_constructible_v<std::decay_t<Args>> && ...))
            {
                initializer = [arguments = std::make_tuple(std::forward<Args>(args)...)] (Scene& target)
                {
                    std::apply([&target] (const auto&... unpacked) { dynamic_cast<T&>(target).Initialize(unpacked...); }, arguments);
                };

                m_initializer = initializer;
            }
            else
            {
                m_initializer = [arguments = std::make_tuple(std::forward<Args>(args)...)] (Scene& target) mutable
                {
                    std::apply([&target] (auto&... unpacked) { dynamic_cast<T&>(target).Initialize(std::move(unpacked)...); }, arguments);
                };
            }
        }
        else
            m_initializer = nullptr;

        if (T::IsTrackable())
            m_stack.emplace(typeid(T), initializer, context, deserializer);

        m_nextScene = std::move(scene);
        Unstage();
    }

    template<typename T, typename... Args>
    std::enable_if_t<
        std::is_base_of_v<Scene, T> &&
        (sizeof...(Args) == 0 || (
            !std::is_base_of_v<ResourceContext, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>> &&
            !std::is_base_of_v<T, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>>
        )),
    void>
    SceneDirector::Present(Args&&... args)
    {
        Present<T>(ResourceContext::Default, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<Scene, T>, bool>
    SceneDirector::Dismiss(const ResourceContext& context, Args&&... args)
    {
        if (m_stack.empty())
            return false;

        auto stack = m_stack;
        stack.pop();

        while (!stack.empty() && stack.top().Type != typeid(T))
            stack.pop();

        if (stack.empty())
            return false;

        m_stack = std::move(stack);
        const auto& presentation = m_stack.top();

        auto scene    = presentation.Deserializer(context);
        m_initializer = presentation.Initializer;
        if constexpr(sizeof...(Args) > 0)
        {
            m_initializer = [arguments = std::make_tuple(std::forward<Args>(args)...)] (Scene& target) mutable
            {
                std::apply([&target] (auto&... unpacked) { dynamic_cast<T&>(target).Initialize(std::move(unpacked)...); }, arguments);
            };
        }

        m_nextScene = std::move(scene);
        Unstage();

        return true;
    }

    template<typename T, typename... Args>
    std::enable_if_t<
        std::is_base_of_v<Scene, T> &&
        (
            sizeof...(Args) == 0 ||
            !std::is_base_of_v<ResourceContext, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>>
        ),
    bool>
    SceneDirector::Dismiss(Args&&... args)
    {
        if (m_stack.empty())
            return false;

        auto stack = m_stack;
        stack.pop();

        while (!stack.empty() && stack.top().Type != typeid(T))
            stack.pop();

        if (stack.empty())
            return false;

        m_stack = std::move(stack);
        const auto& presentation = m_stack.top();

        ResourceContext* context = nullptr;
        if (presentation.Context)
        {
            context = presentation.Context.get();
            context->Unbind();
        }

        auto scene    = presentation.Deserializer(context ? *context : ResourceContext::Default);
        m_initializer = presentation.Initializer;
        if constexpr(sizeof...(Args) > 0)
        {
            m_initializer = [arguments = std::make_tuple(std::forward<Args>(args)...)] (Scene& target) mutable
            {
                std::apply([&target] (auto&... unpacked) { dynamic_cast<T&>(target).Initialize(std::move(unpacked)...); }, arguments);
            };
        }

        m_nextScene = std::move(scene);
        Unstage();

        return true;
    }

    template<typename T>
    bool SceneDirector::IsPresenting() const
    {
        return m_currentScene && dynamic_cast<T*>(m_currentScene.get()) != nullptr;
    }
}
