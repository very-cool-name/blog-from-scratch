# Intro
Let's start with a recap we have following `Variant`.
```cpp
template<typename... Types>
class Variant {
public:
    Variant(); // default constructor, that tries to default construt first T in Types...

    template<typename T>
    Variant(T&& var); // constructor from value

    ~Variant(); // calls appropriate type's destructor

    // type_idx_ accessors
    size_t index();
    size_t index() const;

    void swap(Variant& other);
};

template<size_t Idx, typename... Types>
struct VariantAlternativeT<Idx, Variant<Types...>>;  // holds type of the element in sequence Types... at index Idx

// get_if<0>(Variant*) index based pointer accessors, return nullptr on wrong type
template<size_t Idx, typename... Args>
const T* get_if(const Variant<Args...>* variant);
T* get_if(Variant<Args...>* variant);

// get_if<int>(Variant*) type based pointer accessors, return nullptr on wrong type
template<typename T, typename... Args>
const T* get_if(const Variant<Args...>* variant);
T* get_if(Variant<Args...>* variant);

// get<0>(Variant&) index based accessors, throw on wrong type
template<size_t Idx, typename... Args>
const T& get(const Variant<Args...>& variant);
T& get(Variant<Args...>& variant);
const T&& get(const Variant<Args...>&& variant);
T&& get(Variant<Args...>&& variant);

// get<int>(Variant&) type based accessors, throw on wrong type
template<typename T, typename... Args>
const T& get(const Variant<Args...>& variant);
T& get(Variant<Args...>& variant);
const T&& get(const Variant<Args...>&& variant);
T&& get(Variant<Args...>&& variant);

template<typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& v); // checks that Variant holds type T

template<typename... Types>
struct VariantSize<Variant<Types...>>; // holds size of sequence Types...
```

Let's try to add `emplace` here. It will lead us to a lengthy discussion on exception safety.
### Emplace
There are two versions of `emplace` - typed and indexed. It seems to me that it doesn't matter from which to start, so let's start with indexed one. What's tricky about emplace? Its' behavious depends on the combination of type held by `Variant` and type trying to be emplaced.

**TODO understand when variant calls assignment**

As I promised it lead to start with exception safety of functions above. Which ones can be marked `noexcept`? There are several functions where nothing much happens, which can be easily marked noexcept. These are `index`, `swap`, `holds_alternative` and `get_if`. There is no place in these functions where exception might arise. Destructors are considered `noexcept` by default. The obvious functions which are not `noexcept` are all `get` versions, because they throw exception on wrong access.

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