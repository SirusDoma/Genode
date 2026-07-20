#pragma once

#include <Genode/Entities.hpp>
#include <Genode/System/Context.hpp>
#include <Genode/System/Module.hpp>
#include <Genode/IO/Resource.hpp>

#include <SFML/Graphics.hpp>

#include <typeindex>
#include <stack>
#include <Genode/IO/ResourceContext.hpp>

namespace Gx
{
    class ResourceContext;
    template <typename T>
    using SceneDeserializer = std::function<ResourcePtr<T>(const ResourceContext&)>;

    class Scene;
    class SceneInitializer
    {
    public:
        SceneInitializer() = default;

        SceneInitializer(std::nullptr_t)
        {
        }

        SceneInitializer(std::function<void(Scene&)> initializer)
        {
            if (initializer)
                m_invocable = std::make_unique<Invocable<std::function<void(Scene&)>>>(std::move(initializer));
        }

        template<typename Fn, typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<Fn>, SceneInitializer>            &&
            !std::is_same_v<std::decay_t<Fn>, std::function<void(Scene&)>> &&
            !std::is_same_v<std::decay_t<Fn>, std::nullptr_t>
        >>
        SceneInitializer(Fn&& initializer) :
            m_invocable(std::make_unique<Invocable<std::decay_t<Fn>>>(std::forward<Fn>(initializer)))
        {
        }

        SceneInitializer(SceneInitializer&& initializer) = default;
        SceneInitializer& operator=(SceneInitializer&& initializer) = default;

        SceneInitializer& operator=(std::nullptr_t)
        {
            m_invocable = nullptr;
            return *this;
        }

        explicit operator bool() const
        {
            return m_invocable != nullptr;
        }

        void operator()(Scene& scene) const
        {
            m_invocable->Invoke(scene);
        }

    private:
        struct InvocableBase
        {
            virtual ~InvocableBase() = default;
            virtual void Invoke(Scene& scene) = 0;
        };

        template<typename Fn>
        struct Invocable final : InvocableBase
        {
            explicit Invocable(Fn callback) : Callback(std::move(callback))
            {
            }

            void Invoke(Scene& scene) override
            {
                Callback(scene);
            }

            Fn Callback;
        };

        std::unique_ptr<InvocableBase> m_invocable;
    };

    class Scene;
    class Application;
    class SceneDirector final : public Module, public Renderable, public Updatable, public Inputable
    {
    public:
        explicit SceneDirector(Application& app);
        explicit SceneDirector(RenderSurface& surface);

        template<typename T, typename... Args>
        SceneDirector(RenderSurface& surface, T& scene, Args&&... args);

        template<typename T, typename... Args>
        SceneDirector(RenderSurface& surface, T&& scene, Args&&... args) = delete;

        template<typename T, typename... Args>
        SceneDirector(Application& app, T& scene, Args&&... args);

        template<typename T, typename... Args>
        SceneDirector(Application& app, T&& scene, Args&&... args) = delete;

        RenderStates Render(RenderSurface& surface, RenderStates states) const override;
        void Update(const sf::Time& delta) override;
        bool Input(const sf::Event& ev) override;
        void Focus(bool focused) const;
        void Resize(const sf::Vector2u& size) const;
        bool Close() const;

        [[nodiscard]] Context& GetContext() const;
        [[nodiscard]] Scene& GetPresentingScene() const;

        [[nodiscard]] const sf::View& GetView() const;
        [[nodiscard]] const sf::View& GetDefaultView() const;
        void SetView(const sf::View& view);

        template<typename T>
        [[nodiscard]] bool IsPresenting() const;

        template<typename T, typename... Args>
        std::enable_if_t<std::is_base_of_v<Scene, T>, void>
        Present(T& scene, Args&&... args);

        template <typename T, typename Ctx, typename... Args>
        std::enable_if_t<std::is_base_of_v<Scene, T> && std::is_base_of_v<ResourceContext, Ctx>, void>
        Present(const Ctx& context, Args&&... args);

        template<typename T, typename... Args>
        std::enable_if_t<
            std::is_base_of_v<Scene, T> &&
            (sizeof...(Args) == 0 || (
                !std::is_base_of_v<ResourceContext, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>> &&
                !std::is_base_of_v<T, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>>
            )),
        void>
        Present(Args&&... args);

        template<typename T>
        void Present(T&& scene) = delete;

        template <typename T, typename... Args>
        std::enable_if_t<std::is_base_of_v<Scene, T>, bool>
        Dismiss(const ResourceContext& context, Args&&... args);

        template<typename T, typename... Args>
        std::enable_if_t<
            std::is_base_of_v<Scene, T> &&
            (
                sizeof...(Args) == 0 ||
                !std::is_base_of_v<ResourceContext, std::decay_t<std::tuple_element_t<0, std::tuple<Args..., void>>>>
            ),
        bool>
        Dismiss(Args&&... args);

        bool Dismiss();

        bool Dismiss(const ResourceContext& context);

        void ProcessDelegates() const;

        void Reset();

        [[nodiscard]] Application& GetApplication() const;

        template<typename T>
        std::enable_if_t<std::is_base_of_v<Scene, T>, void>
        Register();

        template<typename T>
        std::enable_if_t<std::is_base_of_v<Scene, T>, void>
        Register(const SceneDeserializer<T>& deserializer);

    private:
        struct ScenePresentationData
        {
            std::type_index                   Type;
            std::function<void(Scene&)>       Initializer{nullptr};
            std::shared_ptr<ResourceContext>  Context{nullptr};
            SceneDeserializer<Scene>          Deserializer{nullptr};

            template<typename Ctx>
            ScenePresentationData(
                std::type_index type,
                const std::function<void(Scene&)>& initializer,
                const Ctx& context,
                const SceneDeserializer<Scene>& deserializer
            );
        };

        using SceneGenericDeserializer = std::function<ResourcePtr<Scene>(const ResourceContext&)>;
        using SceneDeserializerMap     = std::unordered_map<std::type_index, SceneGenericDeserializer>;
        using ScenePresentationStack   = std::stack<ScenePresentationData>;

        void Stage();
        void Unstage() const;

        RenderSurface&          m_surface;
        SceneDeserializerMap    m_deserializers{};
        ScenePresentationStack  m_stack{};
        ResourcePtr<Scene>      m_currentScene{};
        ResourcePtr<Scene>      m_nextScene{};
        SceneInitializer        m_initializer{};
        mutable Context         m_context{};
        mutable bool            m_staged{false};
    };
}

#include <Genode/SceneGraph/SceneDirector.inl>
