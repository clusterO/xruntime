# xruntime

## The first goal of this project is to implement a web server
The major parts of this section are:

1. An http request handler
2. An http response builder
3. A caching mechanism (LRU)
4. Concurency manager

### Test C server * 
 * curl -D - http://localhost:3490/
 * curl -D - http://localhost:3490/number
 * curl -D - http://localhost:3490/date
 
##### Posting Data:
 * curl -D - -X POST -H 'Content-Type: text/plain' -d 'Hello, sample data!' http://localhost:3490/save

## The second phase is to add a dependency manager to the system
## The third phase is to incorporate object oriented and functional programming styles
