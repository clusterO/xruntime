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

## Phase 4: V8/TSC Binding

- [_] Define phase
- [_] CMake for build process
- [_] Tests
- [_] What's Next
