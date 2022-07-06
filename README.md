# Intro
Please don't ask me why, but I decided to write an `std::variant` from scratch. It turned out to be what I think is the most interesting container to implement. It requires a deep dig into the guts of C++, including:
1. `const` correctness
2. Exception safety
3. Placement new
4. Variadic templates
5. Combining templates in run time and compile time

And even more.

I didn't know a lot of stuff required to build proper `std::variant`, so I had to look for clues in the code of available variant implementations. And they are not built for clarity of reading. That's why I decided to write a clear follow up to my endeavour and I hope it might help anyone reading to learn more about C++.

I think the best way to work with this text is to start building `std::variant` yourself. And look for answers here in case of trouble. That's why text is structured to tackle one function of variant at time. And every block ends with plan on what we'll build next, so you may stop and try before seeing my solution. If you are familiar with what `std::variant` is you may jump straight to the part [Building our own Variant](building-our-own-variant). But in case you are not - what is `std::variant`?

# What is variant?

## Union
**TODO wording**
You've probably heard about `union` in C and C++. If you've not - `union` is a way to reuse same storage for different types. It holds several types on stack. The size of stack space used is only as large as the maximum of said types.

```cpp
union S
{
    int32_t i; // occupies 32 bits
    char8_t c; // occupies 8 bits
};             // the whole union occupies 4 bytes

int main() {
    S s;                    // uninitialized union
    std::cout << sizeof(s); // prints 4 (on a system where 1 byte == 8 bit)
    s.i = 1;                // assign big_int
    std::cout << s.i;       // prints '1'
    s.c = 'a';              // reassign small_int
    std::cout << s.c;       // prints 'a'
}
```

![union memory layout](img/variant_union.png)

Unfortunately `union` is very easy to misuse:

```cpp
S s;              // uninitialized union
s.i = 1;          // assign int32_t
std::cout << s.c; // OOPS - undefined behaviour
```

And as we all know UB is very bad - it happens only in runtime and very hard to catch. There are much more subtle problems with unioin, like the fact that you need to explicitly call proper destructors for more complex types.

You might be wondering why would anyone ever want to share space between types. And I'll link you to [this Stackoverflow](https://stackoverflow.com/questions/4788965/when-would-anyone-use-a-union-is-it-a-remnant-from-the-c-only-days) answer for lengthy explanation. But in short:
1. Performance optimization
2. Bit trickery
3. Different way to handle polymorphism

**TODO Example with json, maybe**

The danger of using union actually makes 3-rd point much underappreciated. And all the danger boils down to the fact, that `union` make the user do all type bookkeeping. That's exactly what `std::variant` saves user from.

## Variant
`std::variant` uses some extra space to hold type information, so it prevents illegal access by throwing `std::bad_varaint_access`.

**TODO How does variant deecides between two integers?**
```cpp
int main() {
    std::variant<std::int32_t, std::uint8_t> v{1}; // variant now holds std::int32_t{1}
    std::cout << std::get<std::uint8_t>(v);        // std::bad_variant_access
}
```

If the type is not from expected list of types, error is actually compile time!

```cpp
int main() {
    std::variant<std::int32_t, std::uint8_t> v{1}; // variant now holds std::int32_t{1}
    std::cout << std::get<char>(v);                // compile time error
}
```

Safety has a cost and `variant` is slightyly bigger than union, because it has to store type information somewhere.

```cpp
std::cout << sizeof(S) << "\n" << sizeof(std::variant<std::int32_t, std::uint8_t>) << std::endl;
```

# Building our own Variant
```cpp
template<typename... Types>
class Variant {
};
```

We can't actually just put `union` inside our variant class, so we'll have to improvise. How do we allocate enough storage to hold all our types?

## Storage
First of all we need **stack** storage with size equals to `sizeof` of a maximum of sizes of all `Types...`. We can do this using `std::array`:

```cpp
template<typename... Types>
class Variant {
    std::array<std::byte, MaxSizeof<Types...>::value> storage_;
};
```

`MaxSizeof` will be implemented with recursive approach to unrolling variadic templates:

```cpp
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
```

I think the best way to understand this recursion is try to understand what compiler has to generate for specific example.

```cpp
// For following definition
MaxSizeof<int8_t, int32_t, int16_t>::value

// Compiler will generate this code
template<int8_t, int32_t, int16_t>
struct MaxSizeof {
    private:
        constexpr static auto maxSizeof = MaxSizeof<int32_t, int16_t>::value;  // dependency
    public:
        constexpr static auto value = sizeof(int8_t) > maxSizeof ? sizeof(int8_t) : maxSizeof;
}

// Now compiler sees dependency on MaxSizeof<int32_t, int16_t>::value and will have to generate
template<int32_t, int16_t>
struct MaxSizeof {
    private:
        constexpr static auto maxSizeof = MaxSizeof<int16_t>::value; // dependency
    public:
        constexpr static auto value = sizeof(int32_t) > maxSizeof ? sizeof(int32_t) : maxSizeof;
}

// Now compiler sees dependency on MaxSizeof<int16_t>::value
// It also sees specialization for a single type and will have to use that for generation
template<int16_t>
struct MaxSizeof {
    constexpr static auto value = sizeof(int16_t);
}

// This leads to following values:
MaxSizeof<int16_t>::value == 2;
MaxSizeof<int32_t, int16_t>::maxSizeof == MaxSizeof<int16_t>::value == 2;
MaxSizeof<int32_t, int16_t>::value == (4 > 2) ? 4 : 2 == 4
MaxSizeof<int8_t, int32_t, int16_t>::maxSizeof == MaxSizeof<int32_t, int16_t>::value == 4;
MaxSizeof<int8_t, int32_t, int16_t>::value == (1 > 4) ? 1 : 4 == 4
```

Now `Variant` has exact size of maximum type it holds.

```cpp
std::cout << sizeof(Variant<std::int32_t, std::uint8_t>); // prints 4
```

How do we place someting in `Variant`'s storage?

## Construction
We want to make this code work:
```cpp
Variant<int8_t, int32_t> v{1};
```

It's a constructor, and `Variant` needs a family of constructors with every type from `Types...` as a parameter:
```cpp
template<typename... Types>
class Variant {
    Variant(const int8_t& x);
    Variant(const int32_t& x);
};
```
So we'll use constructor template, accepting any type `T`.

```cpp
template<typename T>
Variant(const T& var) {
}
```

[placement new](https://en.cppreference.com/w/cpp/language/new) can be used to copy construct value inside `Variant`'s storage . Placement new does not allocate memory as new does. Instead it uses provided chunk of memory and calls constructor of object to initialize it.

![placement-new-vs-new](img/variant_placement_new.png)

```cpp
template<typename T>
Variant(const T& var) {
    new(storage_.data()) T(var);
}
```

You may already see the problem - right now we can place any type `T` inside our storage. Let's continue with implementing ther rest of the interface for now. This and other problems'll become obvious and we'll fix them along the way, I promise.
Next, how do we get the value back from `Variant`?

## GetIf
The simplest value we can get value from `Variant` is a strongly typed pointer to `storage_`. It's tempting to simply cast `storage_` to a pointer of a desired type `T*` using two `static_cast` operations.
```cpp
template<typename T>
T* any_cast() {
    return *static_cast<T*>(static_cast<void*>(storage_.data()));
}
```

**TODO add_pointer_t?**

Unfortunately it's the same undefined behaviour as with union if we guessed wrong. Coincidentally this function will be useful to us, so let's leave it for now.

In order to avoid UB `get_if` can call `any_cast` only if guessed type `T` is the type of value in `storage_`. Otherwise `nullptr` is returned. That's why the standard calls this function `get_if`. To do so we'll need to store some type information to check in `get_if`.

How do we store type information?

### Saving type info
Let's go back to the constructor and use very simple type info - index of the type in the sequence. `size_t` is used for index type, because length of pack `sizeof...(Types..)` has this type. It'll be definetly enough to store our index, so let's use it.

```cpp
template<typename... Types>
class Variant {
public:
    template<typename T>
    Variant(T&& var)
    : type_idx_{IndexOf<T, Types...>::value} {
        new(storage_.data()) T(var);
    }

private:
    size_t type_idx_;
    std::array<std::byte, MaxSizeof<Types...>::value> storage_;
};
```

And we'll once again implement `IndexOf<>` recursively:

```cpp
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
```

It's unrolling recursion time!

```cpp
// For following definition
IndexOf<int, char, int, double>::value

// Compiler generates
template<
    T = int,
    Types... = char, int, double
>
struct IndexOf {
    static constexpr size_t value = IndexOfImpl<int, 0, char, int, double>::value;
};

// Now compiler sees dependency on IndexOfImpl<int, 0, char, int, double>::value and will have to generate
template<
    Search = int,
    Idx = 0,
    T = char,
    Types... = int, double
>
struct IndexOfImpl {
    static constexpr size_t value = IndexOfImpl<int, 1, int, double>::value;
};

// Now compiler sees dependency on IndexOfImpl<int, 1, int, double>::value
// And it also matches this specialization: struct IndexOfImpl<Search, Idx, Search, Types...>, so it's used for generation
template<
    Search=int,
    Idx = 1,
    Types... = double>
struct IndexOfImpl<
    Search = int,
    Idx = 1,
    Search = int,
    Types... = double
> {
    static constexpr size_t value = 1;
};

// Now recursion stopped and we got the value
IndexOf<int, char, int, double>::value == IndexOfImpl<int, 1, int, double>::value == 1
```

`IndexOf` works and it gave us bonus feature. Now constructor forbids types not present in `Types...` - `IndexOf<int, char, double>` recursion stops with compile error. That's because when `Types...` sequence ends it doesn't provide explicit `typename T` template requires.

```cpp
template<
    Search=int,
    Idx = 2,
    T = ???
>
struct IndexOfImpl;
```

**TODO `decay_t`**
**TODO Note that our variant bigger than std::variant**

Now that we have `type_idx_` stored, we can proceed with `get_if`. Actually there are two versions of it - typed and indexed.

```cpp
std::variant<int, char> v;
std::get_if<int>(&v); // tries to get variable of type int
std::get_if<0>(&v);   // tries to get first type, which is also int
```

**TODO get<index>**

Which one should we implement first? I didn't know about this, but variant allows repeats of the same type. Yes, `variant<int, int, int>` is allowed. In such case `get_if<int>` gives a compile time error because it's not clear which `int` was requested. This makes `get<index>` the basic building block, which works for every kind of variant.

How do we implement `get<index>`?

### `get_if<index>`
**TODO member vs friend**
Indexed get is a bit hard to start with, because we can't even write a proper declaration for it.

```cpp
template<size_t Idx>
T* get_if(); // what's T?
```

That's our first goal - to get type `T` at index `Idx` from list of `Types...`.

```cpp
template<size_t Idx, size_t CurIdx, typename T, typename... Types>
struct TypeAtIdxImpl {
    using type = TypeAtIdxImpl<Idx, CurIdx + 1, Types...>::type;
};

template<size_t Idx, typename T, typename... Types>
struct TypeAtIdxImpl<Idx, Idx, T, Types...> {
    using type = Ts;
};

template<size_t Idx, typename... Types>
struct TypeAtIdx {
    using type = typename TypeAtIdxImpl<Idx, 0, Types...>::type;
};

template<size_t Idx>
TypeAtIdx<Idx, Types..>::type* get_if();
```

Is it plane? Is it bird? No, it's variadic recursion.

```cpp
// For following definition
TypeAtIdx<1, char, int, double>::type

// Compiler generates
template<
    size_t Idx = 1,
    typename... Types = char, int, double>
struct TypeAtIdx {
    using type = typename TypeAtIdxImpl<1, 0, char, int, double>::type;
};

// Now compiler sees dependecy on typename TypeAtIdxImpl<1, 0, char, int, double>::type
template<
    size_t Idx = 1,
    size_t CurIdx = 0,
    typename U = char,
    typename... Types = int, double>
struct TypeAtIdxImpl {
    using type = TypeAtIdxImpl<1, 1, int, double>::type;
};

// Now compiler sees dependecy on typename TypeAtIdxImpl<1, 1, int, double>::type
// And provided specialization matches
template<
    size_t Idx = 1,
    typename T = int,
    typename... Types = double>
struct TypeAtIdxImpl<1, 1, int, double> {
    using type = int;
};

// Now recursion stopped and we got the type
TypeAtIdx<1, char, int, double>::type == TypeAtIdxImpl<1, 0, char, int, double>::type == TypeAtIdxImpl<1, 1, int, double>::type == int
```

Now that we have `TypeAtIdx`, `get_if` becomes easy to implement.

```cpp
template<size_t Idx>
TypeAtIdx<Idx, Types...>::type* get_if() {
    if(Idx != type_idx_) {
        return nullptr;
    } else {
        using T = TypeAtIdx<Idx, Variant<Args...>>::type;
        return any_cast<T>();
    }
}
```

Let's think about return type a little bit. What if variant holds a reference `Variant<int&>`? Return type of `get_if` becomes `int&*` which is illegal in C++, because it's a pointer to a reference. The better way to convert type to a pointer is `add_pointer_t`. It decays all types correctly, so we'll use it for `get_if` and `any_cast`.

```cpp
template<size_t Idx>
std::add_pointer_t<TypeAtIdx<Idx, Types...>::type> get_if();

template<typename T>
std::add_pointer_t<T> any_cast();
```

I lied to you. `std::variant` is not permitted to hold references, arrays, or the type void. **TODO add_pointer_t is still usefull because of function types**

The other thing is we kinda leaked implementation detail which is `TypeAtIdx` to the public interface of `Variant`. `std::variant` has a thing similar to our `TypeAtIdx` called `variant_alternative_t`. It's used as a return type for `std::get_if`. The difference from `TypeAtIdx` is that `variant_alternative_t` implemented only for `std::variant`, and not for any sequence of types. It's quite simple to build our own `VariantAlteernativeT` using `TypeAtIdx`.

```cpp
template<size_t Idx, typename T>
struct VariantAlternative;

template<size_t Idx, typename T>
using VariantAlternativeT = typename VariantAlternative<Idx, T>::type;

// VariantAlternative must be declared before Variant to be used internally
template<typename... Types>
class Variant {
    std::add_pointer_t<VariantAlternativeT<Idx, Variant<Types...>>> get_if();
};

// VariantAlternative specialization for Variant must be defined after Variant's definition
template<size_t Idx, typename... Types>
struct VariantAlternative<Idx, Variant<Types...>> {
    using type = typename TypeAtIdx<Idx, Types...>::type; // forward all the work to TypeAtIdx
};
```

If you're confused about ordering, don't worry I'm with you. But I think it works because when user tries to use `get_if` the compiler sees all the declarations and definitions required to compile.

Okay that was a lot. Now we have a problem with constant `Variant`.

```cpp
const Variant<int> x{1};
x.get_if<0>(); // get_if is not constant
```

How do we implement `get_if<index>() const`?

### `get_if<index>() const`
The obvious idea is to copy-paste and add `const`s. `get_if() const` requires only chage in declaration, the implementation may stay the same.

```cpp
template<size_t Idx>
std::add_pointer_t<const VariantAlternativeT<Idx, Variant<Types...>>> get_if() const;
```

It calls `any_cast`, so we'll also need a constant version of that.

```cpp
template<typename T>
const T* any_cast() const {
    return *static_cast<const T*>(static_cast<void*>(storage_.data()));
}
```

Now it works, but can we do better, without code duplication? Basically, what we want is to either call `get_if() const` from `get_if` or vice versa. Vice versa is forbidden by compilers, so let's proceed with the first option. Naive take leads to compile error.

```cpp
template<size_t Idx>
std::add_pointer_t<VariantAlternativeT<Idx, Variant<Types...>>> get_if() {
    const auto* cthis = this;
    return cthis->get_if<Idx>();  // OOPS `const T*` is not convertible to `T*`
}
```

Will `const_cast` help us with the compiler error?

```cpp
template<size_t Idx>
std::add_pointer_t<VariantAlternativeT<Idx, Variant<Types...>>> get_if() {
    using T = VariantAlternativeT<Idx, Variant<Types...>>;
    const auto* cthis = this;
    return const_cast<std::add_pointer_t<T>>(cthis->get_if<Idx>());
}
```

Yes it helped, but does it cause undefined behaviour? Answer to this question depends on the context of `const_cast` usage. It's legal to cast away constness from pointer or reference, when the originally referenced object was not `const`.

```cpp
int x = 0;
const int* cptr_x = &x;
int* ptr_x = const_cast<int*>(cptr_x); // perfectly legal because x is orginally non-const

const int y = 0;
const int* cptr_y = &y;
int* ptr_y = const_cast<int*>(cptr_y); // OOPS undefined because y is originally const
```

This makes `const_cast` legal in our case, because non-constant version of `get_if` can be only called with non-const object.

```cpp
Variant<int> x {1};
x.get_if<int>(); // calls const_cast, but original memory is non-const

const Variant<int> y {1};
y.get_if<int>(); // calls get_if() const, so no const cast

Variant<int>& ref_y = y; // compiler error
ref_y.get_if<int>();     // won't get there
```

In order to cause UB in `Variant`'s code, one must call illegal `const_cast` before `get_if`, so it's not out problem.

```cpp
const Variant<int> y {1};

Variant<int>& ref_y = const_cast<Variant<int>&>(y); // OOPS UB - casting away constness of orginally const object
ref_y.get_if<int>();                                // illegal `const_cast` here doesn't matter
```

We can also get rid of non-const `any_cast`, because now `get_if()` just calls `get_if() const`.

How do we turn `get_if` to free function, that accepts `Variant<Types...>` as a parameter?

### free

```cpp
template<size_t Idx, typename... Args>
std::add_pointer_t<const VariantAlternativeT<Idx, Variant<Args...>>> get_if(const Variant<Args...>* variant) {
    if(variant == nullptr || Idx != variant->type_idx_) {
        return nullptr;
    } else {
        return variant-> template any_cast<T>();
    }
}

template<size_t Idx, typename... Args>
std::add_pointer_t<VariantAlternativeT<Idx, Variant<Args...>>> get_if(Variant<Args...>* variant) {
    using T = VariantAlternativeT<Idx, Variant<Types...>>;
    const auto* cvariant = variant;
    return const_cast<std::add_pointer_t<T>>(get_if<Idx>(cvariant));
}
```

We had to add `nullptr` check and this quirk with `variant-> template any_cast`. From my point of view it does not make sense for compiler to require `template` here, so let me give you an example where it makes sense.

```cpp
variant->func<0>(13);
// compiler can't tell whether it's call to a function or something like
(variant->func < 0) > (13)
// so compiler requires explicit `template` keyword to be sure, that it's a call to a template function
variant-> template func<0>(13);
```

In the case of `get_if` the second option is even weirder `(variant->any_cast < T) > ()`. `T` is a type and there is no comparison operator for types as far as I know. The rest of the line also gives more questions - what the hell is `> ()`? I don't know why compiler requires `template` in our case, but we'll let it to have this one.

To finalize our work with `get_if<index>` let's make it a free function, like in the standard.

### free

`get_if` needs to access `Variant`'s private section, so it needs to be declared friend. This is actually pretty straightforward if you don't try to overthink it and just copy to class.

```cpp
template<typename... Types>
class Variant {
    template<size_t Idx, typename... Args>
    friend std::add_pointer_t<const VariantAlternativeT<Idx, Variant<Args...>>> get_if(const Variant<Args...>* variant);
};
```

`Types...` from `Variant`'s can't be used for friend declaration because linker will complain, that it can't find right function to call. And linker is right, cosider these two functions.

```cpp
template<typename... Types>
class Variant {
    friend void function1(const Variant<Types...>* variant);
};

template<typename... Args>
void function2(const Variant<Args...>* variant)
```

They have two different signatures. `functionn1` is a template function with it's own parameter pack. `function2` is a weird thing, that just uses parameter pack of class, even though it has not relation to it. So it doesn't pay off to be smart here and reuse `Varaint`'s parameter pack.

Non-const version of `get_if` doesn't need to be declaraed friend, because it just calls const `get_if`. So that's the only friend declaration we'll need. Neat.

Ok, so what's about other typed version of `get_if`?

### `get_if<T>`

Now that we have `IndexOf` implemented and type index stored, `get<T>` becomes rather easy to implement.
```cpp
template<typename T>
T* get_if() {
    if (IndexOf<T, Types...>::value != type_idx_) {
        throw std::runtime_error("Type is not right"); // bad_variant_access is in <variant> header
    } else {
        return *any_cast<T>();
    }
}
```

### get

The documentation has 4 overloads of function get:
```cpp
template<typename T, typename... Args>
T& get(Variant<Args...>& variant);
T&& get(Variant<Args...>&& variant);
const T& get(const Variant<Args...>& variant);
const T&& get(const Variant<Args...>&& variant);
```

**TODO what the hell is T&&?**
**TODO rework all text**
We could potentially just copy-paste code with some tweaks for `const`, but I hope we'd all prefer some code reuse. Ideally, to implement logic in just one function and call it in three others. Which one is the only to be called?

### `get<T>` overloads
The type, that accepts incoming `T&`, `T&&` and `const T&&` is `const T&`. So if we only implement function accepting `f(const T&)`, we can call it from every other.

```cpp
void clref(const int& x);

void lref(int& x) {
    clref(x);
}

void rref(int&& x) {
    clref(x);
}

void crref(const int&& x) {
    clref(x);
}
```

In our case we also have a return type, so we have a problem.
```cpp
const int& clref(const int& x);

int& lref(int& x) {
    return clref(x);  // OOPS const int& is not convertible to int&
}
```

We could use `const_cast`
```cpp
const int& clref(const int& x);

int& lref(int& x) {
    return const_cast<int&>(clref(x));
}
```
The trick is, that sometimes this code is perfectly legal and sometimes causes UB. It depends on what `clref` function does. How so? Standard says, that it's legal to drop constness of object, which was not const in creation, and it's illegal if it was.

Returning to our example with `lref` function calling `clref`. If we are sure, that `clref` operates only on object x that we passed - it's perfectly legal to cast away `const`.

```cpp
const int& clref(const int& x) {
    return x;
}

int& lref(int& x) {
    return const_cast<int&>(clref(x));
}
```
And that's exactly our case with `get` function. So that means we may only implement const reference version and call it from every other. How do we do it?


## Destruction
Let's address another problem, that `Variant`'s constructor introduced. Placement new can't be cleaned up with simple `delete` call. Code must explictly call appropiate destructor.
```cpp
std::array<std::byte, sizeof(T)> storage; // allocate storage
T* tptr = new(storage.data()) T;          // save type using pointer
tptr->~T();                               // delete explicitly
```

In our case it's a bit worse beacuse we can't actually save a pointer of type `T`. And even though we have `TypeAtIdx` it's not usable in runtime.
```cpp
~Variant() {
    using T = TypeAtIdx<type_idx_, Types...>;  // this does not work
    type_cast<T>()->~T();
}
```
Destructor can't be called recursively to iterate `Types...` , so a helper function is needed. It will iterate through sequence of indexes `0..sizeof...(Types)` and when hitting appropriate one - use `TypeAtIdx` to call proper destructor.

```cpp
template<size_t Idx>
void destroy() {
    if (Idx != type_idx_) {
        destroy<Idx + 1>();
    } else {
        using T = typename TypeAtIdx<Idx, Types...>::type;
        any_cast<T>()->~T();
    }
}
```

When compiled this code will give a long long error message boiling down to:
```
error: too few template arguments for class template 'TypeAtIdxImpl'
    using type = TypeAtIdxImpl<Idx, CurIdx + 1, Types...>;
```
That's because compiler doesn't know when to stop recursion. For example for `Variant` with just one element compiler will generate following function `destroy`:

```cpp
Variant<int> v;

template<0>
void destroy() {
    if (0 /*Idx*/ != type_idx_) {
        destroy<1>();
    } else {
        using T = char;
        any_cast<T>()->~T();
    }
}
```
Compiler can't predict, that this function never actually hits the line `destroy<1>();`, so compiler must generate `destroy<1>`. And this leads to error.

```cpp
template<1>
void destroy() {
    if (1 /*Idx*/ != type_idx_) {
        destroy<2>();
    } else {
        using T = typename TypeAtIdx<1, int>::type; // OOPS there is no Type with index 1
        any_cast<T>()->~T();
    }
}
```

The solution is to write explicit stop for recursion as template specialization.
```cpp
template<>
void destroy<sizeof...(Types)>() {}
```

Now compiler will generate just one function, see our specialization and stop.

```cpp
template<0>
void destroy() {
    if (0 /*Idx*/ != type_idx_) {
        destroy<1>();
    } else {
        using T = char;
        any_cast<T>()->~T();
    }
}

template<1>
void destroy() {}
```

## Other
### Index
### HoldsAlternative
### VariantSize
### Default constructor
### Swap
### Align

Okay, so now we actually have a pretty neat little `Variant`. It has proper construction and destruction. The stored value can be type-safely retrieved and modified. But there are two big missing parts. Our `Variant` can't be copied or moved. And the typed of stored value is decided on creation and can't be changed later.
**TODO connected to exception safety and that's where we'll start our part II**

## Part II
### Emplace

**TODO example with shared_ptr**
**TODO exception safety**
### Copy & Move constructor
**TODO concepts to solve problem with two constructors**
**copy and move constructors**

```cpp
    Variant& operator=(Variant&& other) {
        auto tmp = std::move(other);
        tmp.swap(*this);
        return *this;
    }
```

### default constructor
concepts

### operator=(T), emplace & ValuelessByException
**operator= with swap trick**
**operator= for same type**
**operator= and valueless_vy_exception**
**concepts for operator= and copy constructors type_traits**
### ExceptionSafety
### Comparison
### Visit
It's just too complicated signature
### `Variant<int, int, int>`
Very interesting thing, Idx must check for index.
**TODO get<int>**
### Hash
hash is connecte to type_idx_, so we'll need to explain above