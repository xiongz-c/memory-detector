gcc $1.c -O0 -o "./Exec/$1";
gcc -D_GNU_SOURCE -shared -fPIC -o "./Lib/libMemTracker.so" MemTracker_c.c MemTracker_c.h  -ldl -rdynamic;
export LD_PRELOAD="./Lib/libMemTracker.so";
./Exec/$1 2>"./Report/$1.report";


