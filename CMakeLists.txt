cmake_minimum_required(VERSION 4.4)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(RLOption
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION ""
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(a.out)

target_compile_options(a.out PRIVATE
    -Werror
    -Weverything

    # 2. Selectively disable conflicting/noisy warnings AFTER -Weverything
    -Wno-unsafe-buffer-usage
    -Wno-lifetime-safety
    -Wno-lifetime-safety-all
    -Wno-lifetime-safety-permissive 
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat-local-type-template-args
    -Wno-c99-designator
    -Wno-gnu-case-range
    -Wno-pre-c++14-compat
    -Wno-pre-c++14-compat-pedantic
    -Wno-pre-c++17-compat 
    -Wno-pre-c++17-compat-pedantic
    -Wno-pre-c++20-compat
    -Wno-pre-c++20-compat-pedantic
    -Wno-pre-c++23-compat
    -Wno-pre-c++23-compat-pedantic
    -Wno-pre-c++26-compat
    -Wno-pre-c++26-compat-pedantic
    -Wno-switch-default
    -Wno-covered-switch-default
    -Wno-unused-command-line-argument
    -Wno-weak-vtables
    -Wno-exit-time-destructors
    -Wno-global-constructors
    -Wno-missing-prototypes
    -Wno-nrvo
    -Wno-padded

    -fsafe-buffer-usage-suggestions
    -fno-omit-frame-pointer
    -ferror-limit=999999               # 999 is sane; 999999 will crash IDEs
    -fcolor-diagnostics
    # -fno-exceptions

    # -Xclang -freflection
    -fsanitize=local-bounds
    -D_GLIBCXX_ASSERTIONS
    -D_GLIBCXX_DEBUG           # catches it harder (O(n) but good in CI)
    -std=c++26
    -stdlib=libstdc++

)

target_sources(a.out PUBLIC
    src/Option.cpp
)

target_include_directories(a.out PRIVATE
    include/
)

target_link_options(a.out PRIVATE 
    -fsanitize=address,undefined 
    -static-libsan                  # <- Statically embeds ASan & UBSan into the binary
    -static-libstdc++               # <- Statically embeds libstdc++
    -lstdc++exp 
    -O3 
    -flto
    -DNDEBUG
    -march=native
)

target_link_libraries(a.out PRIVATE -lstdc++exp)
