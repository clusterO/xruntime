# xruntime

## The first goal of this project is to implement a web server

The major parts of this section are:

- [x] An http request handler
- [x] An http response builder
- [x] A caching mechanism (LRU)
- [x] Concurency manager

### Test C server \*

- curl -D - http://localhost:3490/
- curl -D - http://localhost:3490/number
- curl -D - http://localhost:3490/date

##### Posting Data:

- curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save

## The second phase is to add a dependency manager to the system

- [x] Init & configure
- [x] Install, Update and upgrade
- [x] Build process
- [_] DepSolver (SAT)
- [_] CI/CD

#### Compile using GCC

```bash
gcc cpm.c -o cpm -I /mnt/data/2020/C\ runtime\ env/x\ runtime/cpm/ libs/asprintf.c libs/case.c libs/commander.c libs/console-colors.c libs/copy.c libs/debug.c libs/fs.c libs/hash.c libs/http-get.c libs/list.c libs/list_iterator.c libs/list_node.c libs/mkdirp.c libs/parse-repo.c libs/parson.c libs/path-join.c libs/path-normalize.c libs/rimraf.c libs/strdup.c libs/str-ends-with.c libs/str-flatten.c libs/str-starts-with.c libs/substr.c libs/trim.c libs/which.c libs/tempdir.c libs/wiki-registry.c libs/wildcardcmp.c common/cache.c common/package.c gumbo-parser/attribute.c gumbo-parser/char_ref.c gumbo-parser/error.c gumbo-parser/get-element-by-id.c gumbo-parser/get-elements-by-tag-name.c gumbo-parser/gumbo-text-content.c gumbo-parser/parser.c gumbo-parser/string_buffer.c gumbo-parser/string_piece.c gumbo-parser/tag.c gumbo-parser/tokenizer.c gumbo-parser/utf8.c gumbo-parser/util.c gumbo-parser/vector.c -lcurl -lpthread
```

## The third phase is to incorporate object oriented and functional programming styles

- [ ] OOC [OOC](https://ipfs.io/ipfs/bafykbzacebdwpzy4hih6puwm2vl746vponefnskbolmiuxmcbfjfe2gvnqrie?filename=Axel%20Schreiner%20-%20Object-Oriented%20Programming%20with%20ANSI-C%20%281993%29.pdf)
- [ ] FC [Functional C](https://ipfs.io/ipfs/bafykbzacedosuw6brb6mpmlwy7pex4kjtzcftrdhjk2thj4ppxtifcbjqckgy?filename=%28International%20Computer%20Science%20Series%29%20P.%20Hartel%2C%20F.%20Muller%20-%20Functional%20C-Addison-Wesley%20%281997%29.pdf)
- [ ] Rename functions with PascalCase
- [ ] Fix Wall Wextra
- [ ] Functional tests

## V8/TSC Binding
