#pragma once

#include <array>

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
    using type = TypeAtIdxImpl<Idx, CurIdx + 1, Types...>;
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
    template<typename U>
    Variant(U&& var)
    : type_idx_{IndexOf<U, Types...>::value} {
        new(storage_.data()) U(std::forward<U>(var));
    }

    ~Variant() {
        destroy<0>();
    }

    template<typename U, typename... Args>
    friend const U& get(const Variant<Args...>& variant);

private:
    template<typename U>
    U* any_cast() const {
        return static_cast<U*>(static_cast<void*>(storage_.data()));
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
    mutable std::array<std::byte, MaxSizeof<Types...>::value> storage_;
};

template<typename U, typename... Args>
const U& get(const Variant<Args...>& variant) {
    if (IndexOf<U, Args...>::value == variant.type_idx_) {
        return *variant. template any_cast<U>();
    } else {
        throw std::runtime_error("Type is not right");
    }
}

template<typename U, typename... Args>
U& get(Variant<Args...>& variant) {
    const auto& cvariant = variant;
    return const_cast<U&>(get<U>(cvariant));
}

template<typename U, typename... Args>
U&& get(Variant<Args...>&& variant) {
    return std::move(get<U>(variant));
}

template<typename U, typename... Args>
const U&& get(const Variant<Args...>&& variant) {
    const auto& cvariant = variant;
    return std::move(get<U>(cvariant));
}

template<size_t Idx, typename... Args>
const VariantAlternativeT<Idx, Variant<Args...>>& get(const Variant<Args...>& variant) {
    using U = VariantAlternativeT<Idx, Variant<Args...>>;
    return get<U>(variant);
}

template<size_t Idx, typename... Args>
VariantAlternativeT<Idx, Variant<Args...>>& get(Variant<Args...>& variant) {
    using U = VariantAlternativeT<Idx, Variant<Args...>>;
    return get<U>(variant);
}

template<size_t Idx, typename... Args>
VariantAlternativeT<Idx, Variant<Args...>>&& get(Variant<Args...>&& variant) {
    using U = VariantAlternativeT<Idx, Variant<Args...>>;
    return std::move(get<U>(variant));
}

template<size_t Idx, typename... Args>
const VariantAlternativeT<Idx, Variant<Args...>>&& get(const Variant<Args...>&& variant) {
    using U = VariantAlternativeT<Idx, Variant<Args...>>;
    return std::move(get<U>(variant));
}

template<size_t Idx, typename... Types>
struct VariantAlternative<Idx, Variant<Types...>> {
    using type = typename TypeAtIdx<Idx, Types...>::type;
};

} // namespace variant_i