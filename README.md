# Memory allocation monitoring & leak detection
### Memory Alllocation Monitor

#### Compile

```BASH
g++ -o Monitor.cpp -o demo -lncurses
```

#### execute

- Direct execution `./demo`：Real-time monitoring of all processes in the system.
- `./demo -t`：Real-time monitoring of all threads running in the system.
- `./demo -p [pid]`：Real-time monitoring of all threads running under a process and the allocation and release of file descriptors in that process.

### Leak Detection Tool

#### Compile

**run.sh**

```shell
# C version Launch
gcc $1.c -O0 -o "./Exec/$1"; 
gcc -D_GNU_SOURCE -shared -fPIC -o "./Lib/libMemTracker.so" MemTracker_c.c MemTracker_c.h  -ldl -rdynamic;
export LD_PRELOAD="./Lib/libMemTracker.so";
./Exec/$1 2>"./Report/$1.report";

# C++ version Launch
g++ $1.cpp -O0 -o $1 -lncurses -rdynamic;
g++ -D_GNU_SOURCE -shared -fPIC -o "./Lib/libMemTracker.so" MemTracker_cpp.cpp MemTracker_cpp.h  -ldl -rdynamic -DONLINE;
export LD_PRELOAD="./Lib/libMemTracker.so";
./$1;
```

#### execute

```shell
# C verison
cd ~/cVer
./run.sh test1

# C++ version
cd ~/cppVer
./run.sh test2
# create another  shell 
cd ~/cppVer/Exec
./Analysis test2
```

