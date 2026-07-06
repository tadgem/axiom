#pragma once
#include <algorithm>
#include <array>
#include <filesystem>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "Alloc.hpp"
#include "Prim.hpp"


namespace axm {

    using String = std::basic_string<char, std::char_traits<char>, mi_stl_allocator<char>>;

    template<typename Type, typename Allocator = mi_stl_allocator<Type>>
    using Vector = std::vector<Type, Allocator>;

    template<typename Type, size_t N>
    using Array = std::array<Type, N>;

    template<typename Type, size_t Extent = std::dynamic_extent>
    using Span = std::span<Type, Extent>;

    template<typename Type1, typename Type2>
    using Pair = std::pair<Type1, Type2>;

    template<typename Type1, typename... Others>
    using Variant = std::variant<Type1, Others...>;

    template<typename Type>
    using Unique = std::unique_ptr<Type, mimalloc_default_delete<Type>>;

    template<typename Type>
    using RCP = std::shared_ptr<Type>;

    template<typename Key, typename Value>
    using HashMap = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                                       mi_stl_allocator<std::pair<const Key, Value>>>;

    template<typename Type>
    using Future = std::future<Type>;

    // template utils
    template<typename Type>
    static bool IsFutureReady(Future<Type> const &o) {
        return o.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template<typename Type, typename... Args>
    Unique<Type> MakeUnique(Args &&...args) {
        void *memory_loc = mi_malloc(sizeof(Type));
        Unique<Type> ptr = Unique<Type>(new (memory_loc) Type(std::forward<Args>(args)...));
        return std::move(ptr);
    }

    template<typename Type, typename... Args>
    RCP<Type> MakeRCP(Args &&...args) {
        return std::allocate_shared<Type>(STLMimallocAllocator<Type>(), std::forward<Args>(args)...);
    }

    namespace Filesystem = std::filesystem;

    str_hash HashString(const String &input);
} // namespace axm
