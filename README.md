# xruntime
- Compiler and XRuntime (OCC: generator, lexer, parser, model, semantic analysis) alignment
- apply makefile learning curve to xruntime

## Phase 1: Implement a web server

#### The major parts of this section are:

- [x] An http request handler
- [x] An http response builder
- [x] A caching mechanism (LRU)
- [x] Concurrency manager (thread pool)
- [x] Code comments and documentation
- [x] Cookie management
- [x] Session management
- [x] Router implementation

#### Compile using GCC

```bash
cd http
gcc src/server.c src/request.c src/cookie.c src/response.c src/session.c src/router.c common/cache.c common/hashtable.c common/linked_list.c common/mime.c common/net.c common/file.c common/thread_pool.c -o server -lpthread -w
```

#### Run the server

```bash
# Run with default 4 worker threads
./server

# Run with custom number of worker threads
./server 8
```

#### Test HTTP server

- curl http://localhost:3490/
- curl http://localhost:3490/data
  <!-- - curl http://localhost:3490/number -->
  <!-- - curl http://localhost:3490/date -->

#### Post data:

- curl -XPOST -H'Content-Type: text/plain' -D'Hello, sample data!' http://localhost:3490/save

#### Resources:

- [HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)

## Phase 2: Add a dependency manager to the system

- [x] Init & configure
- [x] Install, Update and upgrade
- [x] Build process
- [x] DepSolver (SAT)
- [x] CI/CD

#### Design principles

- The system must support all different ways of using dependencies (language specific source colators, language specific package manager, third party managers, vendoring, system dependencies)
- The dependency system must prevent people from getting the same dependency via multiple ways
- Maintenance costs should be borne mostly by those who get the benefit
- A project must support being either a dependency provider or consumer transparently
- Build definitions must not dictate how or from where a dependency should be obtained
- Subprojects must be configured and build in isolated sandboxes with narrow and explicit interfaces between them
- Try to support for simple cases of mixing build system (general case challenge)
- Provide a centralised dependency downloader, but do not mandate its use

#### Compile using GCC

```bash
cd cpm
gcc src/cpm.c libs/asprintf.c libs/case.c libs/commander.c libs/console-colors.c libs/copy.c libs/debug.c libs/fs.c libs/hash.c libs/http-get.c libs/list.c libs/list_iterator.c libs/list_node.c libs/mkdirp.c libs/parse-repo.c libs/parson.c libs/path-join.c libs/path-normalize.c libs/rimraf.c libs/strdup.c libs/str-ends-with.c libs/str-flatten.c libs/str-starts-with.c libs/substr.c libs/trim.c libs/which.c libs/tempdir.c libs/wiki-registry.c libs/wildcardcmp.c libs/gumbo-parser/attribute.c libs/gumbo-parser/char_ref.c libs/gumbo-parser/error.c libs/gumbo-parser/get-element-by-id.c libs/gumbo-parser/get-elements-by-tag-name.c libs/gumbo-parser/gumbo-text-content.c libs/gumbo-parser/parser.c libs/gumbo-parser/string_buffer.c libs/gumbo-parser/string_piece.c libs/gumbo-parser/tag.c libs/gumbo-parser/tokenizer.c libs/gumbo-parser/utf8.c libs/gumbo-parser/util.c libs/gumbo-parser/vector.c common/cache.c common/package.c -lcurl -lpthread -o cpm -w
```

#### Test CPM (C package manager)

- cpm -V
- cpm help

## Phase 3: Object-Oriented and Functional C Extensions

- [x] COOP: Object-Oriented Programming in C ([Axel T. Schreiner - Object-Oriented Programming with ANSI-C](https://www.mclibre.org/descargar/docs/libros/ooc-ats.pdf))
- [x] CFP: Functional Programming in C ([Pieter H. Hartel - Functional C](https://archive.org/details/functionalc0000hart))
- [x] C-Star Preprocessor: Automated Transpilation for OOP and FP
- [x] Functional and Integration Tests
- [x] Refactor and Runtime Integration

## Phase 4: Frontend UI from C
**Compile to WebAssembly via Emscripten**

Architecture Principle
Don't just compile C to WASM and call it a day. Build a UI framework in your extended-C syntax that gets transpiled to C, then compiled to WASM. The developer writes:

```c
// Your extended-C syntax
component Counter {
    state int count = 0;
    
    render {
        button(onclick: increment) {
            text("Count: %d", count);
        }
    }
    
    fn increment() {
        this->count++;
    }
}
```

Your preprocessor transpiles this to C structs and function pointers (Schreiner-style), Emscripten compiles it to WASM, and a thin JS runtime mounts it to the DOM.

Key Decisions

Decision Recommendation 
DOM access Use Emscripten's `EM_JS` macros or `embind` to create a minimal bridge. Don't expose raw JS interop to the developer—hide it behind your component abstraction. 
State management Leverage your FP primitives (Hartel) for immutable state updates. A Redux-like store in C is actually elegant with function pointers. 
Reactivity Implement a virtual DOM diff in C. It's slower than JS frameworks for huge lists, but for typical UI it's fine—and you control the memory. 
Bundle size Emscripten bloats easily. Use `-Os`, `-s FILESYSTEM=0`, `-s EXPORTED_RUNTIME_METHODS=[]` aggressively. Target under 200KB for a hello-world. 
Threading Start single-threaded. WASM threads (`SharedArrayBuffer`) are complex and limit deployment (COOP/COEP headers). 

Deliverable for this phase
A single-page app (e.g., a todo list or a dashboard) written entirely in your extended-C syntax, compiled to WASM, running in a browser. This proves the frontend target works end-to-end.

## Phase 5: Test

Immediate next steps

1. Write a non-trivial demo app in your extended-C syntax (e.g., a REST API with DB access). This will reveal whether Schreiner's OOP and Hartel's FP hold up under real pressure or if they're too verbose for production.
2. Audit the preprocessor: Can it handle itself? If the preprocessor is written in C, can it preprocess its own source? (Dogfooding reveals architectural flaws.)
3. Define the memory model: C doesn't have GC. Your OOP objects need a lifecycle story. Without this, the framework is unusable.
4. Decide on async: Modern frameworks are async-first. Can your FP primitives express `async/await` or promises? This is where Hartel's work becomes crucial.

## Phase 6: The expansion

Path 1: The "Spring for C" Backend Framework (Recommended)
Your HTTP server and package manager are already here. Double down:
- Async runtime: Implement an event loop (epoll/kqueue/IOCP) with your FP primitives (Hartel's closures map beautifully to callbacks/async).
- Memory management: Schreiner's OOP needs a deterministic memory story. Add arena allocators or reference counting via your preprocessor macros.
- Module ecosystem: Your package manager needs a standard library (HTTP routing, JSON, DB drivers) written in your extended-C syntax to prove the paradigms work.
- Hot reloading: Use your preprocessor to enable runtime code injection.

Why this fits: C's killer advantage is systems/backend programming. Don't abandon it to chase frontend trends.

Path 2: The "Better C" Language Project
If the preprocessor is the real innovation:
- Stabilize the OOP/FP transpilation with rigorous testing (edge cases in macro expansion are brutal).
- Add modules/namespaces (C lacks these; your preprocessor can fake them).
- Add generics/templates via preprocessor code generation.
- Build an LSP/language server so developers get IDE support.
- Target WASM compilation as your "frontend" story—let developers write C-like code that runs in browsers via WASM, not via JS interop.

**How it works**

Path 2 (Better C) Feeds Path 1 (Backend Framework)
Your preprocessor isn't just a transpiler—it's the language definition. Path 2 should add:

1. Module system (critical for a package manager)
   
```c
   module http.router;
   import http.server;
   import std.collections;
   ```

2. Generics/Templates via preprocessor
   
```c
   list<int> numbers;
   list<user_t> users;
   ```

   Transpiles to C macro hacks or code generation.

3. Error handling
   
```c
   result<response_t> res = http_get(url);
   if (res.is_err) { ... }
   ```

   Maps to `union { ok; err }` in C.

4. Async/await syntax
   
```c
   async response_t fetch_data() {
       return await http_get(url);
   }
   ```

   Transpiles to state machines using your event loop (Path 1).

Path 1 (Backend Framework) Validates Path 2
The backend framework is the reference implementation written in your extended-C syntax. Every feature in Path 2 must be dogfooded here:

- HTTP server → written with your `async/await` syntax
- Router → written with your OOP `class`/`interface` keywords
- Middleware pipeline → written with your FP `map`/`filter`/`compose` primitives
- JSON parser → written with your generics (`result<json_t>`)

If you can't write a clean JSON parser in your extended-C syntax, your language isn't ready.

---

Suggested Architecture: The "Isomorphic C" Model

Since you have both frontend (WASM) and backend (native), you can offer something most frameworks can't: shared code between client and server.

```
xruntime/
├── core/ # Path 2: Language runtime (memory, types, async primitives)
│ ├── oop.h # Schreiner vtables + your preprocessor keywords
│ ├── fp.h # Hartel closures + your FP keywords
│ ├── async.h # Event loop, promises, await state machine
│ └── result.h # Error handling union
├── backend/ # Path 1: Native server framework
│ ├── http/ # HTTP server (uses core/async)
│ ├── router/ # URL routing (uses core/oop)
│ └── db/ # Database drivers
├── frontend/ # WASM UI framework
│ ├── dom/ # Virtual DOM (uses core/oop + core/fp)
│ ├── component/ # Component lifecycle
│ └── state/ # Reactive store (uses core/fp)
└── shared/ # Isomorphic code (runs in both native and WASM)
    ├── types/ # User-defined structs (serialized the same way)
    ├── validation/ # Input validation (run on client AND server)
    └── protocol/ # Binary/JSON RPC schema
```

The RPC Bridge
Because both sides are compiled from the same language, you can generate type-safe RPC:

```c
// shared/api.xr (your syntax)
service UserAPI {
    rpc get_user(int id) -> user_t;
    rpc create_user(create_req_t req) -> result<user_t>;
}
```

Your preprocessor generates:
- Server: HTTP route handlers + deserialization
- Client (WASM): Fetch wrappers + type-safe stubs

This is your killer feature. JavaScript frameworks need GraphQL/Protobuf to achieve this. You get it for free because you own the language.

---

Concrete Roadmap

Now → 2 months: WASM Proof of Concept
- Write a minimal virtual DOM in C (no preprocessor yet, pure C)
- Compile it to WASM with Emscripten, render to DOM
- Port it to your extended-C syntax, make the preprocessor handle it
- Build one demo app (todo list or real-time chat)

2 → 4 months: Language Hardening (Path 2)
- Implement module system in preprocessor
- Add `result<T>` and `option<T>` generics
- Add `async`/`await` syntax that transpiles to state machines
- Write a memory allocator/arena that integrates with Schreiner's OOP

4 → 6 months: Backend Framework (Path 1)
- Rewrite your HTTP server using `async/await` syntax
- Build a router using OOP interfaces
- Implement middleware pipeline using FP composition
- Dogfood the package manager to install xruntime modules

6 → 8 months: The Bridge
- Define the shared/isomorphic module structure
- Build the RPC generator from service definitions
- Create a full-stack demo (e.g., a collaborative editor or admin dashboard)

---

Pitfalls to Watch For

1. Emscripten dependency hell: Pin your Emscripten version in your package manager. EMSDK changes fast and breaks builds.
2. String handling in WASM: C strings are painful in browserland. Decide early: null-terminated? Length-prefixed? UTF-8 with validation? Your preprocessor should enforce one model.
3. OOP memory leaks in WASM: Schreiner's manual `new`/`delete` is fine for native, but WASM has no valgrind. Add a debug allocator that tracks allocations and reports leaks to the browser console.
4. Preprocessor recursion: If your preprocessor generates C that includes headers that trigger the preprocessor again, you can get infinite loops. Use file guards or a two-pass design.
5. Async in C is hard: `async/await` transpilation to C state machines is well-trodden (C++ coroutines, protothreads, simulators), but debugging the generated C is miserable. Generate readable C with comments indicating the original source line.

---

One Final Thought

The most successful "C with modern features" projects (Zig, Nim, Odin) all made one critical decision early: they prioritized a great build system and package manager over language features. You already have the package manager. Make sure it can:

- Fetch Emscripten automatically (or verify it's installed)
- Cross-compile with one flag: `xrt build --target wasm`
- Cache compiled objects aggressively (C compile times are brutal)

If `xrt build` Just Works for both `native` and `wasm`, developers will tolerate your preprocessor's rough edges while you polish them.
