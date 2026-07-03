#pragma once
#include "Alloc.hpp"
#include "Prim.hpp"
#include <algorithm>
#include <filesystem>
#include <future>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <variant>


namespace axm {

    using String = std::basic_string<char, std::char_traits<char>, mi_stl_allocator<char>>;

    template <typename _Ty, typename Allocator = mi_stl_allocator<_Ty>>
    using Vector = std::vector<_Ty, Allocator>;

    template <typename _Ty, size_t N>
    using Array = std::array<_Ty, N>;

    template<typename _Ty, size_t _Extent = std::dynamic_extent>
    using Span = std::span<_Ty, _Extent>;

    template <typename _Ty1, typename _Ty2>
    using Pair = std::pair<_Ty1, _Ty2>;

    template<typename _Ty1, typename ... _Others>
    using Variant = std::variant<_Ty1, _Others...>;

    template <typename _Ty>
    using Unique = std::unique_ptr<_Ty, mimalloc_default_delete<_Ty>>;

    template <typename _Ty>
    using RCP = std::shared_ptr<_Ty>;

    template <typename _Key, typename _Value>
    using HashMap =
        std::unordered_map<_Key, _Value, std::hash<_Key>, std::equal_to<_Key>,
                           mi_stl_allocator<std::pair<const _Key, _Value>>>;

    template <typename _Ty>
    using Future = std::future<_Ty>;

    // template utils
    template <typename _Ty>
    static bool IsFutureReady(Future<_Ty> const &o) {
        return o.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    template <typename _Ty, typename... Args>
    Unique<_Ty> MakeUnique(Args &&...args) {
        void *memory_loc = mi_malloc(sizeof(_Ty));
        Unique<_Ty> ptr = Unique<_Ty>(new (memory_loc) _Ty(std::forward<Args>(args)...));
        return std::move(ptr);
    }

    template <typename _Ty, typename... Args>
    RCP<_Ty> MakeRCP(Args &&...args) {
        return std::allocate_shared<_Ty>(STLMimallocAllocator<_Ty>(), std::forward<Args>(args)...);
    }

    namespace Filesystem = std::filesystem;

    str_hash HashString(const String &input);
} // namespace harmony
