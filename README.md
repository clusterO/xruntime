# xruntime

## The first goal of this project is to implement a web server

#### The major parts of this section are:

- [x] An http request handler
- [x] An http response builder
- [x] A caching mechanism (LRU)
- [_] Concurency manager
- [_] See code comments

#### Compile using GCC

```bash
cd http
gcc server.c llist.c hashtable.c cache.c file.c mime.c net.c helpers/request.c -o server -w
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

## The second phase is to add a dependency manager to the system

- [x] Init & configure
- [x] Install, Update and upgrade
- [x] Build process
- [x] DepSolver (SAT)
- [_] CI/CD

#### Design principles

- The system must support all different ways of using dependencies (language specific source colators, language specific package manager, third party managers, vendoring, system dependencies)
- The dependency system must prevent people from getting the same dependency via multiple ways
- Maintenance costs should be borne mostly by those who get the benefit
- A project must support being either a dependency provider or consumer transparently
- Build definitions must not dictate how or from where a dependency should be obtained
- Subprojects must be configured and build in solated sandboxes with narrow and explicit interfaces between them
- Try to support for simple cases of mixing build system (general case challenge)
- Provide a centralised dependency downloader, but do not mandate its use

#### Compile using GCC

```bash
cd cpm
gcc cpm.c libs/asprintf.c libs/case.c libs/commander.c libs/console-colors.c libs/copy.c libs/debug.c libs/fs.c libs/hash.c libs/http-get.c libs/list.c libs/list_iterator.c libs/list_node.c libs/mkdirp.c libs/parse-repo.c libs/parson.c libs/path-join.c libs/path-normalize.c libs/rimraf.c libs/strdup.c libs/str-ends-with.c libs/str-flatten.c libs/str-starts-with.c libs/substr.c libs/trim.c libs/which.c libs/tempdir.c libs/wiki-registry.c libs/wildcardcmp.c common/cache.c common/package.c gumbo-parser/attribute.c gumbo-parser/char_ref.c gumbo-parser/error.c gumbo-parser/get-element-by-id.c gumbo-parser/get-elements-by-tag-name.c gumbo-parser/gumbo-text-content.c gumbo-parser/parser.c gumbo-parser/string_buffer.c gumbo-parser/string_piece.c gumbo-parser/tag.c gumbo-parser/tokenizer.c gumbo-parser/utf8.c gumbo-parser/util.c gumbo-parser/vector.c -lcurl -lpthread -o cpm -w
```

#### Test CPM (C package manager)

- cpm -V
- cpm help

## The third phase is to incorporate object oriented and functional programming styles

- [x] OOC [OOC](https://ipfs.io/ipfs/bafykbzacebdwpzy4hih6puwm2vl746vponefnskbolmiuxmcbfjfe2gvnqrie?filename=Axel%20Schreiner%20-%20Object-Oriented%20Programming%20with%20ANSI-C%20%281993%29.pdf)
- [_] FC [Functional C](https://ipfs.io/ipfs/bafykbzacedosuw6brb6mpmlwy7pex4kjtzcftrdhjk2thj4ppxtifcbjqckgy?filename=%28International%20Computer%20Science%20Series%29%20P.%20Hartel%2C%20F.%20Muller%20-%20Functional%20C-Addison-Wesley%20%281997%29.pdf)
- [_] Fix Wall Wextra
- [_] Functional tests
- [_] Refactor
- [_] What's next

## The fourth phase is V8/TSC Binding
