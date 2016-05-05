# xruntime

## The first goal of this project is to implement a web server
The major parts of this section are:

[x] An http request handler
[x] An http response builder
[x] A caching mechanism (LRU)
[x] Concurency manager

### Test C server *
 * curl -D - http://localhost:3490/
 * curl -D - http://localhost:3490/number
 * curl -D - http://localhost:3490/date

##### Posting Data:
 * curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save

## The second phase is to add a dependency manager to the system

[x] Init & configure
[x] Install, Update and upgrade
[x] Build process
[_] DepSolver (SAT)
[_] CI/CD

## The third phase is to incorporate object oriented and functional programming styles

[_] OOC
[_] FC

## V8/TSC Binding