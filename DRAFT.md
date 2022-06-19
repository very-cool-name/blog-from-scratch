## Motivating example
Imagine geometry library with rectangles and squares. Let the task be simple and artificial - generatee 5000000 random shapes and calculte their total area without accounting itersections. How can we design such a library?

## `virtual` polymorphism
One thing we usually use is standard c++ polymorphism.
```cpp
struct Shape {              // base class for all shapes
    virtual int area() = 0;
    virtual ~Shape() {}
};

struct Rectangle: Shape {
    int x, y, w, h;        // center, width and height of rectangle

    int area() override {
        return w * h;
    }
};

struct Circle: Shape {
    int x, y, r;           // center and radius of circle

    int area() override {
        return r * r * 3;  // Pi is 3, whatever
    }
};
```

Generation:
```cpp
const int NUM_SHAPES = 5'120'000;
std::vector<std::unique_ptr<Shape>> shapes;
shapes.reserve(NUM_SHAPES);
for(int i = 0; i < NUM_SHAPES; ++i) {
    // shape(rng) generates 0 or 1
    if(shape(rng)) {
        shapes.emplace_back(
            // coords(rng) generates integer in range [0, 10)
            std::make_unique<Rectangle>(coords(rng), coords(rng), coords(rng), coords(rng))
        );
    } else {
        shapes.emplace_back(
            std::make_unique<Circle>(coords(rng), coords(rng), coords(rng))
        );
    }
}
```

Area calculation:
```cpp
int area = 0;
for(const auto& s: shapes) {
    area += s->area();
}
```


```
Run on (8 X 2800 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB
  L1 Instruction 32 KiB
  L2 Unified 256 KiB (x4)
  L3 Unified 6144 KiB
Load Average: 2.62, 3.18, 3.12
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
BM_Union                22.5 ms         22.1 ms           30
BM_Union                20.9 ms         20.6 ms           30
BM_Union                27.8 ms         25.2 ms           30
BM_Union                22.9 ms         22.2 ms           30
BM_Union                25.5 ms         23.7 ms           30
BM_Union                24.8 ms         24.0 ms           30
BM_Union                25.1 ms         24.4 ms           30
BM_Union                20.0 ms         19.9 ms           30
BM_Union                19.9 ms         19.9 ms           30
BM_Union                20.1 ms         20.0 ms           30
BM_Union_mean           23.0 ms         22.2 ms           10
BM_Union_median         22.7 ms         22.2 ms           10
BM_Union_stddev         2.77 ms         2.02 ms           10
BM_Union_cv            12.05 %          9.12 %            10
BM_Poly                 39.8 ms         39.4 ms           17
BM_Poly                 54.6 ms         51.3 ms           17
BM_Poly                 37.6 ms         37.2 ms           17
BM_Poly                 37.8 ms         37.6 ms           17
BM_Poly                 33.8 ms         33.7 ms           17
BM_Poly                 34.9 ms         34.8 ms           17
BM_Poly                 43.7 ms         43.3 ms           17
BM_Poly                 44.4 ms         43.7 ms           17
BM_Poly                 35.4 ms         35.2 ms           17
BM_Poly                 42.8 ms         41.9 ms           17
BM_Poly_mean            40.5 ms         39.8 ms           10
BM_Poly_median          38.8 ms         38.5 ms           10
BM_Poly_stddev          6.22 ms         5.35 ms           10
BM_Poly_cv             15.37 %         13.44 %            10
BM_Variant              28.8 ms         28.8 ms           24
BM_Variant              29.3 ms         29.1 ms           24
BM_Variant              31.4 ms         31.3 ms           24
BM_Variant              34.3 ms         33.6 ms           24
BM_Variant              36.2 ms         35.6 ms           24
BM_Variant              35.8 ms         30.6 ms           24
BM_Variant              33.7 ms         31.9 ms           24
BM_Variant              32.1 ms         31.7 ms           24
BM_Variant              31.7 ms         31.4 ms           24
BM_Variant              32.9 ms         30.9 ms           24
BM_Variant_mean         32.6 ms         31.5 ms           10
BM_Variant_median       32.5 ms         31.3 ms           10
BM_Variant_stddev       2.48 ms         1.99 ms           10
BM_Variant_cv           7.60 %          6.33 %            10
```