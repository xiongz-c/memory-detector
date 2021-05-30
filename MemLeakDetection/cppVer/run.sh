g++ $1.cpp -O0 -o $1 -lncurses -rdynamic;
g++ -D_GNU_SOURCE -shared -fPIC -o "./Lib/libMemTracker.so" MemTracker_cpp.cpp MemTracker_cpp.h  -ldl -rdynamic -DONLINE;
export LD_PRELOAD="./Lib/libMemTracker.so";
./$1 2>tmp;
