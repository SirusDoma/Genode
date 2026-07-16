# Genode

Genode (**G**ame **E**ngi**N**e **O**n **DE**mand) is game framework that written in C++17 and built on top of [SFML](https://github.com/SFML/SFML) V3.

As the name suggest, this project is on the contranary side of other common engine or framework; initially, it is not built for general use case in mind.
It began with the idea that it would only evolve or change to satisfy the *demands* of my own game development needs, which favor certain conventions, patterns, and even my own personal preferences.

After successfully developed my game with this framework, I've decided to put this into an open source repository.

# Integration
This framework can be either added into the game source tree directly as an additional CMake target or built and included separately.

## Repository Structure ##

| Directory | Contents                                                                                                                     |
|-----------|------------------------------------------------------------------------------------------------------------------------------|
| bin       | Final build compilation output, categorized by OS and build config (e.g: `bin/windows/Debug/`). Does not include any assets. |
| build     | CMake build directory and object files that generated during compilation.                                                    |
| extlibs   | External libraries                                                                                                           |
| include   | Headers directory                                                                                                            |
| src       | Source code directory                                                                                                        |

## Adding as a CMake target
Place the Genode source tree inside your project (e.g., as a git submodule) and add it to your `CMakeLists.txt`:

```cmake
add_subdirectory(modules/Genode)

target_link_libraries(MyGame PRIVATE Genode)
```

Alternatively, let CMake download Genode for you via [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```cmake
include(FetchContent)

FetchContent_Declare(
    Genode
    GIT_REPOSITORY https://github.com/SirusDoma/Genode.git
    GIT_TAG        master # or pin a specific commit
)
FetchContent_MakeAvailable(Genode)

target_link_libraries(MyGame PRIVATE Genode)
```

Linking the `Genode` target is all that is required: the include directories and the dependencies propagate to the consuming target automatically.

## Compiling the project
To build the library separately instead, use a standard CMake build and include the headers inside [`include`](/include) directory to your game project:

```shell
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

> [!Note]
> Set `OUTPUT_DIR` flag in your cmake to override the compilation output location.
> See [Configuring your SFML build](https://www.sfml-dev.org/tutorials/3.1/getting-started/build-from-source/#configuring-your-sfml-build) for more options to configure the SFML build used by Genode.

You can find the compilation output in [`bin`](/bin) folder which you can use to link against your project.

The project is fully relies on [CMake](https://cmake.org/), and it makes use of [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html) to manage the dependencies.  
No additional setup or commands are required to build the project other than standard cmake build.

List of dependencies:
- [SFML](https://github.com/SFML/SFML)
- [freetype2](https://freetype.org/)
- [HarfBuzz](https://github.com/harfbuzz/harfbuzz)
- [SheenBidi](https://github.com/Tehreer/SheenBidi)
- [fmt](https://github.com/fmtlib/fmt)
- [nlohmann_json](https://github.com/nlohmann/json)
- [Boost.PFR](https://github.com/boostorg/pfr)

# Documentation

Please refer to the [wiki](https://github.com/SirusDoma/Genode/wiki) for the Documentation.

# Design Goals
There are myriads of 2D Game Framework out there, and each have its reasons and goals to exist. This framework had these design goals:

## Intuitive syntax and pattern
The framework provides range of modules and lifecycles that is simple, clear and hopefully intuitive to use. For an instance, you can construct a Scene that requires a lot of dependencies with a non-intrusive IoC container and with minimum to no configuration.

## Resource management freedom
Many frameworks or engines include built-in functionalities for resource serialization, deserialization, and management/allocation. Often, these approaches can be restrictive. For example, these kind of engines usually won't allow you to change the content pipeline or file format, and it is unintuitive to customize and workaround.

This framework streamline on how you interact with resources while still allowing you to choose how they are deserialized, managed, and allocated.

## SFML Extension
The entire modules are built side by side with SFML. Some of the SFML classes are re-implemented to fit the framework usage, however, you can always use SFML built-in classes.
On top of that, there's some classes that allows you to extend SFML capabilities without making intrusive changes into SFML directly.

## Only pay for what you use
There are several ranges of abstraction levels in this framework. High abstraction classes are made using one or more lower abstraction classes and it provide convenience but may introduce higher performance overhead.  

This framework is designed so that lower-level and higher-level abstractions can work together, often directly alongside SFML, letting you choose the right level of abstraction for your needs.

## Conveniences over speed
This framework was initially designed by a single developer in a limited scope, and thus conveniences are priortized over performance and efficiencies. It is still important aspect to certain degree, after all, the game that was built with this framework is a rhythm game that has hundreds moving objects at a time and require smooth framerate and high timing precision.

That being said, there are certainly faster 2D game frameworks/engines out there that are faster and more capable in handling bigger scope in more sophisticated/intuitive/elegant way. However, the goal is to speed up my game development process. If you find yourself in a similar situation, consider give Genode a try.

# License

This open-source software is licensed under the [zlib/libpng](LICENSE) license.

External libraries used by Genode (including SFML and its dependencies) are distributed under their own licenses.
See [SFML License](https://github.com/SFML/SFML?tab=readme-ov-file#license) and [External libraries used by SFML](https://github.com/SFML/SFML?tab=readme-ov-file#external-libraries-used-by-sfml) for more details.
