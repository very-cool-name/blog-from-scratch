# Intro
Once I decided to write a standard-like variant from scratch. It turned out to be much deeper rabbit hole, than I expected. And I had to look into variant implementation for clues **WHICH is not a pleasant experience**. So I decided to write follow up in case someone wants to try this and will also need help.
I'd advise you to get familiar with variadic templates **!!!Link** if you are not, because variant uses them, and text has a bit brief explanation.
I think the best way to work with this text is to start building variant yourself. And look for answers here in case of trouble. That's why text is structured to tackle one function of variant at time. And every block ends with plan on what we'll build next, so reader may stop and try this.

# What is variant?

## Union
You've probably heard about `union` in C and C++. If you've not - `union` is a way to reuse same storage for different types. It holds several types on stack and is only as large as the maximum of said types. Here is the example.

```cpp
union S
{
    std::int32_t big_int;     // occupies 4 bytes
    std::uint8_t small_int;   // occupies 1 byte
};                            // the whole union occupies 4 bytes

int main() {
    S s;                      // uninitialized union
    s.big_int = 1;            // assign big_int
    std::cout << s.big_int;   // prints 1
    s.small_int = 13;         // reassign small_int
    std::cout << s.small_int; // prints 13
}
```

**!!!Picture with memory**

Unfortunately `union` is very easy to misuse:

```cpp
S s;                        // uninitialized union
s.big_int = 1;              // assign big_int
std::cout << s.small_int;   // OOPS - undefined behaviour
```

And as we all know UB is very bad - it happens only in runtime and very hard to catch.
[note] There are also bigger problems, like a need to explicitly call destructor for more complex types.

You might be wondering why would anyone ever want to share space between types. And I'll link you to [this Stackoverflow](https://stackoverflow.com/questions/4788965/when-would-anyone-use-a-union-is-it-a-remnant-from-the-c-only-days) answer for lengthy explanation. But in short:
1. Performance optimization
2. Bit trickery
3. Different way to handle polymorphism

**!!!Example with json, maybe**

The danger of using union actually makes 3-rd point much underappreciated. And variant makes this stuff easy.

## Variant
`std::variant` **!!!is same**  is different because it knows the type it holds. If user tries to get wrong type, variant throws `std::bad_variant_access` exception.

**!!!How does variant deecides between two integers?**
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

**!!!format**
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
So we'll use constructor template, accepting any type `U`.

```cpp
template<typename U>
Variant(const U& var) {
}
```

[placement new](https://en.cppreference.com/w/cpp/language/new) can be used to copy construct value inside `variant`'s storage . Placement new does not allocate memory as ordinary new does. Instead it uses provided chunk of memory and calls constructor of object to initialize it.
**!!!Picture of placement new vs new**

```cpp
template<typename U>
Variant(const U& var) {
    new(storage_.data()) U(var);
}
```

You may already see the problem, that right now we can place any type `U` inside our storage, which is not the only problem. But in order to understand and fix them, let's continue with the interface and change constructor on thee way.
How do we get the value back from `variant`?

## Get
Get has two versions typed and indexed.
```cpp
std::variant<int, char> v;
std::get<int>(v); // tries to get variable of type int
std::get<0>(v);   // tries to get first type, which is also int
```
As you may see get is not a member function in the standard. It adds a bit of complexity with declaring this function `friend`, so let's start with implementing `get` as member function. It's a bit easier to implement typed version of get, so let's focus on that one. We might be tempted to just cast our buffer space to pointer to desired type `U*`. There are two ways to do this `reinterpret_cast` or two `static_cast`s, first to `void*` second to `U*`.

```cpp
template<typename U>
U& get() {
    return *static_cast<U*>(static_cast<void*>(storage_.data()));
}
```
Unfortunately it's the same undefined behaviour as with union if we guessed wrong. We'll need to store some type info and check whether the call is right or not.
But the function itself is useful to us, so we'll call it `any_cast`, make it private and return `U*`.
```cpp
template<typename U>
U* any_cast() {
    return static_cast<U*>(static_cast<void*>(storage_.data()));
}
```

### Saving type info
Let's go back to the constructor and use very simple type info - index of the type in the sequence. `size_t` is used for index type, because length of pack `sizeof...(Types..)` has this type, so we might as well use it for indexing.
```cpp
template<typename... Types>
class Variant {
public:
    template<typename U>
    Variant(const U& var)
    : type_idx_{IndexOf<U, Types...>::value} {
        new(storage_.data()) U(var);
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
**Explanation in terms of recursion unroll**
**That actually made a compile time error for wrong types**

### get<T>
Now that we have `IndexOf` implemented and type index stored, `get<T>` becomes easy to implement.
```cpp
template<typename U>
U& get() {
    if (IndexOf<U, Types...>::value == type_idx_) {
        return *any_cast<U>();
    } else {
        throw std::runtime_error("Type is not right"); // bad_variant_access is in <variant> header
    }
}
```
Return type for template functions is not an easy thing to get right. For example, what if we have `Variant<int&>`? Then `U&` actually becomes `int&&` and that's not what we want. Luckily for us we don't need to think about this, because `std::varaint` forbids references, array types and `void*`.
Now we can turn this `get` to free function, that accepts `Variant<Types...>` as a parameter.
```cpp
template<typename U, typename... Args>
U& get(Variant<Args...>& variant) {
    if (IndexOf<U, Types...>::value == variant.type_idx_) {
        return *variant. template any_cast<U>();
    } else {
        throw std::runtime_error("Type is not right"); // bad_variant_access is in <variant> header
    }
}
```
The only odd thing here is **<variant. template>**.

`get` needs to access `Variant`'s private section, so we'll need to declare it as friend. This is actually pretty straightforward if you don't try to overthink it and just copy to class:

```cpp
template<typename... Types>
class Variant {
    template<typename U, typename... Args>
    friend U& get(Variant<Args...>& variant);
};
```
We can't actually use `Types...` from Variant declaration. **<Explain linker errro>**

The documentation has 4 overloads of function get:
```cpp
template<typename U, typename... Args>
U& get(Variant<Args...>& variant);
U&& get(Variant<Args...>&& variant);
const U& get(const Variant<Args...>& variant);
const U&& get(const Variant<Args...>&& variant);
```
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

```cpp
int x = 0;
const int& clref_x = x;
int& lref_x = const_cast<int&>(clref_x); // perfectly legal because is orginally non-const

const int y = 0;
const int& clref_y = y;
int& lref_y = const_cast<int&>(clref_y); // OOPS undefined because y is originally const
```
Returning to our example with `lref` function calling `clref`. If we are sure, that `clref` operates only on object x, that we passed - it's perfectly legal to cast away `const`.

```cpp
const int& clref(const int& x) {
    return x;
}

int& lref(int& x) {
    return const_cast<int&>(clref(x));
}
```
And that's exactly our case with `get` function. So that means we can only implement const reference version and call it from every other. How do we do it?

### get<T>(const Variant&)

**<explain>**
```
any_cast const
mutable
```
`mutable` is totally okay here, example with `unique_ptr`.

How do we do `get<index>`?

### get<index>
We can build `get<index>` from `get<U>` if could only get `U` from index.

```cpp
template<size_t Idx, typename... Args>
const ???& get(const Variant<Args...>& variant) {
    return get<???>(variant);
}
```

Let's find a way to get type from the sequnce of `Args...`. We'll call it `TypeAtIdx`.

```cpp
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
```

Now we can fill in gaps and don't forget to prefix every `TypeAtIdx` with `typename`, so compiler understands.
```cpp
template<size_t Idx>
const typename TypeAtIdx<Idx, Types...>::type& get(const Variant<Args...>& variant) {
    using U = typename TypeAtIdx<Idx, Types...>::type;
    return get<U>(variant);
}
```
That kinda leaks our implementation detail `TypeAtIdx` to the public interface of `Variant`.

**<VariantAlternative>**
Declare before, release after.

## Destruction
Let's address another problem, that `Variant's` constructor introduced. Placement new can't be cleaned up with simple delete, code must explictly call appropiate destructor.
```cpp
std::array<std::byte, sizeof(T)> storage; // allocate storage
T* tptr = new(storage.data()) T;
tptr->~T();
```
In our case it's a bit worse beacuse we can't actually save a pointer of type `T`. And even though we have `TypeAtIdx` it's not actually usable in runtime.
```cpp
~Variant() {
    using T = TypeAtIdx<type_idx_, Types...>;  // that does not work
    type_cast<T>()->~T();
}
```
Destrcutor can't be called recursively to iterate `Types...` , so we'll delegate this to a helper function. It will iterate through sequence of indexes `0..sizeof...(Types)` and when hitting appropriate one - use `TypeAtIdx` to call proper destructor.

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
Compiler can't predict, that this function never actually hits the line `destroy<1>();`, so it will try to generate `destroy<1>`. And there is an error in this generation.

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

The solution is to write explicit stop for recursion and specialize the function:
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
### GetIf
### Default constructor
Okay, so now we actually have a pretty good `Variant`. It's constructed and destructed properly.

### Visit
### Swap

### Part II
### operator=(T), emplace & ValuelessByException
### ExceptionSafety
### DefaultConstructible & monostate
### Hash
### Variant<int, int, int>`