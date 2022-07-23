#pragma once

#include <array>
#include <type_traits>

namespace variant_i {
template<typename T, typename... Types>
struct MaxSizeof {
private:
    constexpr static auto maxSizeof = MaxSizeof<Types...>::value;
public:
    constexpr static auto value = sizeof(T) > maxSizeof ? sizeof(T) : maxSizeof;
};

template<typename T>
struct MaxSizeof<T> {
    constexpr static auto value = sizeof(T);
};

template<typename Search, size_t Idx, typename T, typename... Types>
struct IndexOfImpl {
    static constexpr size_t value = IndexOfImpl<Search, Idx + 1, Types...>::value;
};

template<typename Search, size_t Idx, typename... Types>
struct IndexOfImpl<Search, Idx, Search, Types...> {
    static constexpr size_t value = Idx;
};

template<typename T, typename... Types>
struct IndexOf {
    static constexpr size_t value = IndexOfImpl<T, 0, Types...>::value;
};

template<size_t Idx, size_t CurIdx, typename T, typename... Types>
struct TypeAtIdxImpl {
    using type = typename TypeAtIdxImpl<Idx, CurIdx + 1, Types...>::type;
};

template<size_t Idx, typename T, typename... Types>
struct TypeAtIdxImpl<Idx, Idx, T, Types...> {
    using type = T;
};

template<size_t Idx, typename... Types>
struct TypeAtIdx {
    using type = typename TypeAtIdxImpl<Idx, 0, Types...>::type;
};

template<size_t Idx, typename T>
struct VariantAlternative;

template<size_t Idx, typename T>
using VariantAlternativeT = typename VariantAlternative<Idx, T>::type;

template<typename... Types>
class Variant {
public:
    template<typename T>
    Variant(T&& var)
    : type_idx_{IndexOf<std::decay_t<T>, Types...>::value} {
        new(storage_.data()) std::decay_t<T>(std::forward<T>(var));
    }

    Variant()
    : type_idx_{0} {
        using T = typename TypeAtIdx<0, Types...>::type;
        new(storage_.data()) T();
    }

    ~Variant() {
        destroy<0>();
    }

    template<size_t Idx, typename... Args>
    friend std::add_pointer_t<const VariantAlternativeT<Idx, Variant<Args...>>> get_if(const Variant<Args...>* variant);

    size_t index() {
        return type_idx_;
    }

    size_t index() const {
        return type_idx_;
    }

    void swap(Variant& other) {
        std::swap(type_idx_, other.type_idx_);
        std::swap(storage_, other.storage_);
    }

private:
    template<typename T>
    const T* any_cast() const {
        return static_cast<const T*>(static_cast<const void*>(storage_.data()));
    }

    template<size_t Idx>
    void destroy() {
        if (Idx != type_idx_) {
            destroy<Idx + 1>();
        } else {
            using T = typename TypeAtIdx<Idx, Types...>::type;
            any_cast<T>()->~T();
        }
    }

    template<>
    void destroy<sizeof...(Types)>() {}

    size_t type_idx_;
    std::array<std::byte, MaxSizeof<Types...>::value> storage_;
};

template<size_t Idx, typename... Types>
struct VariantAlternative<Idx, Variant<Types...>> {
    using type = typename TypeAtIdx<Idx, Types...>::type;
};

template<size_t Idx, typename... Args>
std::add_pointer_t<
    const VariantAlternativeT<Idx, Variant<Args...>>
> get_if(const Variant<Args...>* variant) {
    if(variant == nullptr || Idx != variant->type_idx_) {
        return nullptr;
    } else {
        using T = VariantAlternativeT<Idx, Variant<Args...>>;
        return variant-> template any_cast<T>();
    }
}

template<size_t Idx, typename... Args>
std::add_pointer_t<
    VariantAlternativeT<Idx, Variant<Args...>>
> get_if(Variant<Args...>* variant) {
    const auto* cvariant = variant;
    using T = VariantAlternativeT<Idx, Variant<Args...>>;
    return const_cast<std::add_pointer_t<T>>(get_if<Idx>(cvariant));
}

template<size_t Idx, typename... Args>
const VariantAlternativeT<Idx, Variant<Args...>>& get(const Variant<Args...>& variant) {
    const auto* content = get_if<Idx>(&variant);
    if (content) {
        return *content;
    } else {
        throw std::runtime_error("Variant holds different alternative");
    }
}

template<size_t Idx, typename... Args>
VariantAlternativeT<Idx, Variant<Args...>>& get(Variant<Args...>& variant) {
    using T = VariantAlternativeT<Idx, Variant<Args...>>;
    const auto& cvariant = variant;
    return const_cast<T&>(get<Idx>(cvariant));
}

template<size_t Idx, typename... Args>
VariantAlternativeT<Idx, Variant<Args...>>&& get(Variant<Args...>&& variant) {
    return std::move(get<Idx>(variant));
}

template<size_t Idx, typename... Args>
const VariantAlternativeT<Idx, Variant<Args...>>&& get(const Variant<Args...>&& variant) {
    return std::move(get<Idx>(variant));
}

template<typename T, typename... Args>
std::add_pointer_t<const T> get_if(const Variant<Args...>* variant) {
    return get_if<IndexOf<T, Args...>::value>(variant);
}

template<typename T, typename... Args>
std::add_pointer_t<T> get_if(Variant<Args...>* variant) {
    return get_if<IndexOf<T, Args...>::value>(variant);
}

template<typename T, typename... Args>
const T& get(const Variant<Args...>& variant) {
    return get<IndexOf<T, Args...>::value>(variant);
}

template<typename T, typename... Args>
T& get(Variant<Args...>& variant) {
    return get<IndexOf<T, Args...>::value>(variant);
}

template<typename T, typename... Args>
T&& get(Variant<Args...>&& variant) {
    return std::move(get<T>(variant));
}

template<typename T, typename... Args>
const T&& get(const Variant<Args...>&& variant) {
    return std::move(get<T>(std::move(variant)));
}

template<typename T>
struct VariantSize;

template<typename... Types>
struct VariantSize<Variant<Types...>>: std::integral_constant<size_t, sizeof...(Types)> {
};

template<typename T>
inline constexpr size_t VariantSizeV = VariantSize<T>::value;

template<typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& v) {
    return v.index() == IndexOf<T, Types...>::value;
}

} // namespace variant_i