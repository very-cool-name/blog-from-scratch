#pragma once

#include <array>
#include <compare>
#include <concepts>
#include <new>
#include <type_traits>
#include <utility>

namespace variant_ii {
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

inline constexpr size_t variant_npos = -1;

template<typename T1, typename T2>
concept IsSame = std::is_same_v<T1, T2>;

template<typename From, typename To>
concept NotNarrow = requires (From&& from) {
    {std::type_identity_t<To[]>{std::forward<From>(from)} } -> IsSame<To[1]>;
};

template<
    typename From,
    typename... Types>
struct ChooseConvertible {
    static void F(...);
};

template<
    typename From,
    typename To,
    typename... Types>
struct ChooseConvertible<From, To, Types...>: ChooseConvertible<From, Types...> {
    using ChooseConvertible<From, Types...>::F;
};

template<
    typename From,
    typename To,
    typename... Types>
requires NotNarrow<From, To>
struct ChooseConvertible<From, To, Types...>: ChooseConvertible<From, Types...> {
    using ChooseConvertible<From, Types...>::F;

    static To F(To);
};

template<typename From, typename... Types>
using ChooseConvertibleT = decltype(ChooseConvertible<From, Types...>::F(std::declval<From>()));

template<typename From, typename... Types>
constexpr static bool IsConvertibleV = !std::is_same_v<ChooseConvertibleT<From, Types...>, void>;

template<typename T>
struct IsInPlaceSpecialization: std::false_type {
};

template<typename T>
struct IsInPlaceSpecialization<std::in_place_type_t<T>>: std::true_type {
};

template<size_t Idx>
struct IsInPlaceSpecialization<std::in_place_index_t<Idx>>: std::true_type {
};

template<typename T>
constexpr bool IsInPlaceSpecializationV = IsInPlaceSpecialization<T>::value;

template<typename... Types>
class Variant {
static_assert(sizeof...(Types) < variant_npos);
static constexpr bool IsTriviallyMoveAssignable = ((
    std::is_trivially_move_constructible_v<Types>
    && std::is_trivially_move_assignable_v<Types>
    && std::is_trivially_destructible_v<Types>) && ...);

static constexpr bool IsTriviallyCopyAssignable = ((
    std::is_trivially_copy_constructible_v<Types>
    && std::is_trivially_copy_assignable_v<Types>
    && std::is_trivially_destructible_v<Types>) && ...);

public:
    template<typename T>
    Variant(T&& var)
    noexcept(std::is_nothrow_constructible_v<ChooseConvertibleT<T, Types...>, T>)
    requires (
        !std::is_same_v<std::remove_cvref_t<T>, Variant>
        && !IsInPlaceSpecializationV<std::remove_cvref_t<T>>
        && IsConvertibleV<T, Types...>
        && std::is_constructible_v<ChooseConvertibleT<T, Types...>, T>
    )
    : type_idx_{IndexOf<ChooseConvertibleT<T, Types...>, Types...>::value} {
        new(storage_.data()) ChooseConvertibleT<T, Types...>(std::forward<T>(var));
    }

    Variant()
        noexcept(std::is_nothrow_default_constructible_v<typename TypeAtIdx<0, Types...>::type>)
        requires std::is_default_constructible_v<typename TypeAtIdx<0, Types...>::type>
    : type_idx_{0} {
        using T = typename TypeAtIdx<0, Types...>::type;
        new(storage_.data()) T();
    }

    ~Variant() = default;

    ~Variant()
    requires (!(std::is_trivially_destructible_v<Types> && ...)) {
        destroy<0>();
    }

    template<size_t Idx, typename... Args>
    friend std::add_pointer_t<const VariantAlternativeT<Idx, Variant<Args...>>> get_if(const Variant<Args...>* variant);

    size_t index() noexcept {
        return type_idx_;
    }

    size_t index() const noexcept {
        return type_idx_;
    }

    bool valueless_by_exception() const noexcept {
        return type_idx_ == variant_npos;
    }

    template<typename T, typename... Args>
    Variant(std::in_place_type_t<T>, Args&&... args)
        requires(IndexOf<T, Types...>::value < sizeof...(Types) && std::is_constructible_v<T, Args...>)
    :type_idx_{IndexOf<T, Types...>::value} {
        new(storage_.data()) T(std::forward<Args>(args)...);
    }

    template<size_t Idx, typename... Args>
    Variant(std::in_place_index_t<Idx>, Args&&... args)
        requires(Idx < sizeof...(Types) && std::is_constructible_v<typename TypeAtIdx<Idx, Types...>::type, Args...>)
    :type_idx_{Idx} {
        using T = typename TypeAtIdx<Idx, Types...>::type;
        new(storage_.data()) T(std::forward<Args>(args)...);
    }

    Variant(const Variant& other)
    requires (!(std::is_copy_constructible_v<Types> && ...)) = delete;

    Variant(const Variant& other)
    noexcept((std::is_nothrow_copy_constructible_v<Types> && ...))
    requires (std::is_trivially_copy_constructible_v<Types> && ...) = default;

    Variant(const Variant& other)
    noexcept((std::is_nothrow_copy_constructible_v<Types> && ...))
    requires ((std::is_copy_constructible_v<Types> && ...) && !(std::is_trivially_copy_constructible_v<Types> && ...))
    :type_idx_{other.type_idx_} {
        copy_construct<0>(other);
    }

    Variant(Variant&& other)
    noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
    requires (std::is_trivially_move_constructible_v<Types> && ...) = default;

    Variant(Variant&& other)
    noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
    requires ((std::is_move_constructible_v<Types> && ...) && !(std::is_trivially_move_constructible_v<Types> && ...))
    :type_idx_{other.type_idx_} {
        move_construct<0>(std::move(other));
    }

    Variant& operator=(Variant&& other)
    noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_move_assignable_v<Types>) && ...))
    requires(IsTriviallyMoveAssignable) = default;

    Variant& operator=(Variant&& other)
    noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_move_assignable_v<Types>) && ...))
    requires (!IsTriviallyMoveAssignable && ((std::is_move_constructible_v<Types> && std::is_move_assignable_v<Types>) && ...))
    {
        if (other.type_idx_ == variant_npos) {
            if (type_idx_ == variant_npos) {
                return *this;
            } else {
                destroy<0>();
                return *this;
            }
        } else if (type_idx_ == other.type_idx_) {
            move_assign_same_types<0>(std::move(other));
            return *this;
        } else {
            destroy<0>();
            move_construct<0>(std::move(other));
            type_idx_ = other.type_idx_;
            return *this;
        }
    }

    Variant& operator=(const Variant& other)
    requires (!IsTriviallyCopyAssignable && !((std::is_copy_constructible_v<Types> && ...) && (std::is_copy_assignable_v<Types> && ...))) = delete;

    Variant& operator=(const Variant& other)
    noexcept(((std::is_nothrow_copy_constructible_v<Types> && std::is_nothrow_copy_assignable_v<Types>) && ...))
    requires (IsTriviallyCopyAssignable) = default;

    Variant& operator=(const Variant& other)
    noexcept(((std::is_nothrow_copy_constructible_v<Types> && std::is_nothrow_copy_assignable_v<Types>) && ...))
    requires (!IsTriviallyCopyAssignable && (std::is_copy_constructible_v<Types> && ...) && (std::is_copy_assignable_v<Types> && ...)) {
        if (other.type_idx_ == variant_npos) {
            if (type_idx_ == variant_npos) {
                return *this;
            } else {
                destroy<0>();
                return *this;
            }
        } else if (type_idx_ == other.type_idx_) {
            copy_assign_same_types<0>(other);
            type_idx_ = other.type_idx_;
            return *this;
        } else {
            destroy<0>();
            copy_assign<0>(other);
            type_idx_ = other.type_idx_;
            return *this;
        }
    }

    template<class T>
    Variant& operator=(T&& t)
    noexcept(std::is_nothrow_constructible_v<ChooseConvertibleT<T, Types...>, T> && std::is_nothrow_assignable_v<ChooseConvertibleT<T, Types...>&, T>)
    requires (
        !std::is_same_v<std::remove_cvref_t<T>, Variant>
        && IsConvertibleV<T, Types...>
        && !IsInPlaceSpecializationV<std::remove_cvref_t<T>>
        && std::is_constructible_v<ChooseConvertibleT<T, Types...>, T>
        && std::is_assignable_v<ChooseConvertibleT<T, Types...>&, T>
    ) {
        convert_assign<0>(std::forward<T>(t));
        return *this;
    }

    template<size_t Idx, typename... Args>
    void emplace(Args&&... args) {
        using T = typename TypeAtIdx<Idx, Types...>::type;
        destroy<0>();
        new(storage_.data()) T(std::forward<Args>(args)...);
        type_idx_ = Idx;
    }

    template<typename T, typename... Args>
    void emplace(Args&&... args) {
        emplace<IndexOf<T, Types...>::value, Args...>(std::forward<Args>(args)...);
    }

    void swap(Variant& other)
    noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_swappable_v<Types>) && ...)) {
        if (valueless_by_exception() && other.valueless_by_exception()) {
            return;
        } else if (type_idx_ == other.type_idx_) {
            swap_same_types<0>(other);
        } else {
            Variant tmp(std::move(other));
            other.type_idx_ = variant_npos;
            other.move_construct<0>(std::move(*this));
            other.type_idx_ = type_idx_;
            type_idx_ = variant_npos;
            move_construct<0>(std::move(tmp));
            type_idx_ = tmp.type_idx_;
        }
    }

private:
    template<typename T>
    const T* any_cast() const {
        return std::launder(static_cast<const T*>(static_cast<const void*>(storage_.data())));
    }

    template<size_t Idx>
    void destroy() {
        if (Idx != type_idx_) {
            destroy<Idx + 1>();
        } else {
            using T = typename TypeAtIdx<Idx, Types...>::type;
            any_cast<T>()->~T();
            type_idx_ = variant_npos;
        }
    }

    template<>
    void destroy<sizeof...(Types)>() {}

    template<size_t Idx>
    void copy_construct(const Variant& other) {
        if (Idx != other.type_idx_) {
            copy_construct<Idx + 1>(other);
        } else {
            using T = typename TypeAtIdx<Idx, Types...>::type;
            new(storage_.data()) T(get<Idx>(other));
        }
    }

    template<>
    void copy_construct<sizeof...(Types)>(const Variant&) {}

    template<size_t Idx>
    void move_construct(Variant&& other) {
        if (Idx != other.type_idx_) {
            move_construct<Idx + 1>(std::move(other));
        } else {
            using T = typename TypeAtIdx<Idx, Types...>::type;
            new(storage_.data()) T(get<Idx>(std::move(other)));
        }
    }

    template<>
    void move_construct<sizeof...(Types)>(Variant&&) {}

    template<size_t Idx>
    void copy_assign_same_types(const Variant& other) {
        if (Idx != other.type_idx_) {
            copy_assign_same_types<Idx + 1>(other);
        } else {
            get<Idx>(*this) = get<Idx>(other);
        }
    }

    template<>
    void copy_assign_same_types<sizeof...(Types)>(const Variant&) {}

    template<size_t Idx>
    void copy_assign(const Variant& other) {
        if (Idx != other.type_idx_) {
            copy_assign<Idx + 1>(other);
        } else {
            using T = typename TypeAtIdx<Idx, Types...>::type;
            if constexpr (std::is_nothrow_copy_constructible_v<T> || !std::is_nothrow_move_constructible_v<T>) {
                new(storage_.data()) T(get<Idx>(other));
            } else {
                new(storage_.data()) T(get<Idx>(Variant(other)));
            }
        }
    }

    template<>
    void copy_assign<sizeof...(Types)>(const Variant&) {}

    template<size_t Idx>
    void move_assign_same_types(Variant&& other) {
        if (Idx != other.type_idx_) {
            move_assign_same_types<Idx + 1>(std::move(other));
        } else {
            get<Idx>(*this) = get<Idx>(std::move(other));
        }
    }

    template<>
    void move_assign_same_types<sizeof...(Types)>(Variant&&) {}

    template<size_t Idx, typename T>
    void convert_assign(T&& t) {
        if (Idx != type_idx_) {
            convert_assign<Idx + 1>(std::forward<T>(t));
        } else {
            using To = ChooseConvertibleT<T, Types...>;
            if constexpr (Idx == IndexOf<To, Types...>::value) {
                get<Idx>(*this) = std::forward<T>(t);
            } else if (std::is_nothrow_constructible_v<To, T> || !std::is_nothrow_move_constructible_v<To>) {
                destroy<0>();
                new(storage_.data()) To(std::forward<T>(t));
                type_idx_ = IndexOf<To, Types...>::value;
            } else {
                destroy<0>();
                auto to = To(std::forward<T>(t));
                new(storage_.data()) To(std::move(to));
                type_idx_ = IndexOf<To, Types...>::value;
            }
        }
    }

    template<size_t Idx, typename T>
    void convert_assign(T&& t)
    requires(Idx == sizeof...(Types)) {}

    template<size_t Idx>
    void swap_same_types(Variant& other) {
        if (Idx != type_idx_) {
            swap_same_types<Idx + 1>(other);
        } else {
            using std::swap;
            swap(get<Idx>(*this), get<Idx>(other));
            swap(type_idx_, other.type_idx_);
        }
    }

    template<>
    void swap_same_types<sizeof...(Types)>(Variant& other) {}

    size_t type_idx_;
    alignas(Types...) std::array<std::byte, std::max({sizeof(Types)...})> storage_;
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

template<typename... Types>
void swap(Variant<Types...>& left, Variant<Types...>& right) noexcept(noexcept(left.swap(right)))
requires((std::is_move_constructible_v<Types> && std::is_swappable_v<Types>) && ...) {
    left.swap(right);
}

} // namespace variant_ii