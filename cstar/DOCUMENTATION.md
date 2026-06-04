# C-Star: Functional & Object-Oriented Programming in C

C-Star extends C with modern paradigm capabilities—Object-Oriented (coop) and Functional (cfp)—without sacrificing the power, performance, or transparency of native C. It accomplishes this through a **transpiler** approach, converting idiomatic C-Star syntax into standard C, which is then compiled using your preferred C compiler (e.g., `gcc`, `clang`).

## 1. Object-Oriented C (`coop`)

`coop` provides structured Object-Oriented programming based on the principles of dynamic linkage, inheritance, and metaclasses.

### Syntax
Define classes using the `class` keyword. Inherit from a superclass using the colon (`:`) operator.

```cstar
class Point {
    int x;
    int y;
}

class Circle : Point {
    int rad;
}
```

### Key Principles
*   **Encapsulation:** Implemented via opaque descriptors (`struct Class`).
*   **Polymorphism:** Achieved through dynamic linkage (selector functions dispatched at runtime).
*   **Inheritance:** "Structure Lengthening" ensures subclass objects can be treated as superclass objects (up-casting).
*   **Runtime Type Identification (RTTI):** Safe type checking with `isA`, `isOf`, and `cast`.

---

## 2. Functional C (`cfp`)

`cfp` introduces functional abstractions to C, emphasizing pure functions, recursive data types, and pattern matching.

### Syntax
Define sum types (Algebraic Data Types) using `variant`.

```cstar
variant MaybeInt {
    None, Just
}
```

### Key Principles
*   **Pure Functions:** Predictable, side-effect-free functions.
*   **Tail Recursion:** Recursive logic transformed into efficient imperative `while`/`for` loops.
*   **Algebraic Data Types (ADTs):** Sum and Product types using `struct` and `union` with tag-based pattern matching via generated `match_` macros.
*   **Functional Primitives:** Higher-order functions, list abstractions (`cons`, `map`, `filter`).

---

## 3. The C-Star Transpiler

The `cstar` preprocessor reads `.cstar` files and generates the corresponding idiomatic C code (`.h`, `.r`, `.c`).

### Build Workflow

1.  **Transpile:**
    ```bash
    # Run the preprocessor to generate C code from .cstar files
    ./cstar_bin source.cstar
    ```
    This generates the necessary C boilerplate in `cstar/coop/src/` and `cstar/cfp/src/`.

2.  **Compile:**
    Compile the generated C code along with the existing `coop` and `cfp` runtimes:
    ```bash
    gcc -o my_app source.c \
        cstar/coop/src/new.c cstar/coop/src/Object.c \
        cstar/coop/src/Point.c cstar/coop/src/Circle.c \
        cstar/cfp/src/list.c \
        -I cstar/coop/src -I cstar/cfp/src
    ```

### Dependencies
The runtime requires standard C library headers. The preprocessor itself is built from `cstar/src`.

---

## 4. Best Practices

*   **Stick to the paradigm:** Use `coop` for managing complex stateful object lifecycles and `cfp` for data transformation and logic pipelines.
*   **Pure Functions:** Keep your functional transformations pure whenever possible to ensure predictability.
*   **Memory Management:** C-Star retains manual memory management. Use `delete()` for OO objects and `free_list()` for functional list structures to prevent leaks.
*   **Debugging:** Because `cstar` transpiles to idiomatic C, use your standard debugger (like `gdb`) on the generated `.c` files to inspect the runtime state directly.
