#### Building and running
In order to build you will need docker.  From the Dockerfile provided with the source the requiredimage can be created.  This resulting image can be used for building as well as running.  Use the following steps to build and run
1. Clone from git
2. Build docker image from the provided Dockerfile in the source tree (Provided at top of the tree)
```
docker build -t memcache:dev .
```
3. Create container with name memcachedev from image memcache:dev.  Local source directory is mapped into the container at path /home/memcache. Finally container will run and you will be at bash prompt.
```
docker run -it -v <srcdir>:/home/memcache --name memcachedev memcache:dev /bin/bash
```
4.  Build the source 
```
cd /home/memcache
mkdir build
cd build
cmake ..
```
5. To run, from the build directory
```
./src/memcache-main
```
6. To run the tests from the build directory
```
make test
```
7.  On the container exits, to start back the container
```
docker start -ia memcachedev
```

#### Parameters that can be configured
* cachesz (Max number of cache entries) type: int32 default: 65536
* maxclients (Max number of clients) type: int32 default: 16
* port (server port) type: int32 default: 8080
* shards (Number of shards) type: int32 default: 4
* shardthreads (Server socket IO threads) type: int32 default: 2

To run memcache-main with at port 8000 with 8 shards
```
./src/memcache-main --port=8000 --shards=8
```
NOTE: At the moment support for maxclients does not exist.

#### Third party dependencies
* wangle
* folly
* gtest
* gflags
* glog
All the dependent libraries are checked into source tree under artifacts folder.
