# PERF ATTACHER

Demonstrace PTRACE attaching to follow syscalls on a target program with focus on IO

## Warning

**Tested only on Linux x86_64 for now.**

## Usage

```
perf [-h] [-j] [-o FILE] [-r FD] [-i] [-p PID]

-p PID  : attach to PID
-o FILE : store output in FILE (not compatible with -r)
-r FD   : redirect output to FD (not compatible with -o)
-i      : only show 'io' events
-j      : use a JSON event output
-h      : show this help
```

## Examples

### Attach to PID

```
perf -p XX -o out.log
````

Sample output:

```
[15375] [RET (int) 3] [openat](dfd=-100, path=/usr/local/go , flag=591872, mode=0)  [1625836826.78631591796875 for 1.40667e-05]
[15375] [RET (int) 0] [fstat](fd=3 , buff=0x7ffe2ef3d690)  [1625836826.7863409519195556640625 for 1.21593e-05]
[15375] [RET (int) 0] [fstat](fd=3 , buff=0x7ffe2ef3d760)  [1625836826.786364078521728515625 for 1.19209e-05]
[15375] [RET (int) 1] [write](fd=1 , buff=0x5598892e5510, size=1)  [1625836826.786386966705322265625 for 1.3113e-05]
[15375] [RET (int) 15] [write](fd=1 , buff=0x5598892e5510, size=15)  [1625836826.7864110469818115234375 for 1.28746e-05]
```

## Run a Progam

```
perf -j -o out.log -- cat /proc/self/cmdline 
```

Output (note each dict is NULL separated) :

```json
{ "start" : 1625837000.710058 , "duration" : 2.5987625122070312e-05 , "syscall" : { "name" : "access" , "args" : {"path":"/etc/ld.so.preload" , "mode":4} , "ret": null } }{ "start" : 1625837000.710135 , "duration" : 2.5033950805664062e-05 , "syscall" : { "name" : "openat" , "args" : {"dfd":-100, "path":"/etc/ld.so.cache" , "flag":524288, "mode":0} ,  "ret" : 4 } }{ "start" : 1625837000.7101779 , "duration" : 1.71661376953125e-05 , "syscall" : { "name" : "fstat" , "args" : {"fd":4 , "buff":"0x7ffce7dc04c0"} ,  "ret" : 0 } }{ "start" : 1625837000.710242 , "duration" : 1.5020370483398438e-05 , "syscall" : { "name" : "close" , "args":{"int":4} ,  "ret" : 0 } }{ "start" : 1625837000.7102859 , "duration" : 2.3126602172851562e-05 , "syscall" : { "name" : "openat" , "args" : {"dfd":-100, "path":"/lib/x86_64-linux-gnu/libc.so.6" , "flag":524288, "mode":0} ,  "ret" : 4 } }{ "start" : 1625837000.710324 , "duration" : 1.6927719116210938e-05 , "syscall" : { "name" : "read" , "args" : {"fd":4 , "buff":"0x7ffce7dc0688", "size":832} ,  "ret": 832 } }{ "start" : 1625837000.710356 , "duration" : 1.4066696166992188e-05 , "syscall" : { "name" : "fstat" , "args" : {"fd":4 , "buff":"0x7ffce7dc0520"} ,  "ret" : 0 } }{ "start" : 1625837000.7106521 , "duration" : 1.9788742065429688e-05 , "syscall" : { "name" : "close" , "args":{"int":4} ,  "ret" : 0 } }{ "start" : 1625837000.7111001 , "duration" : 2.47955322265625e-05 , "syscall" : { "name" : "openat" , "args" : {"dfd":-100, "path":"/usr/lib/locale/locale-archive" , "flag":524288, "mode":0} ,  "ret" : 4 } }{ "start" : 1625837000.7111411 , "duration" : 1.6927719116210938e-05 , "syscall" : { "name" : "fstat" , "args" : {"fd":4 , "buff":"0x7f78da36cac0"} ,  "ret" : 0 } }{ "start" : 1625837000.7112131 , "duration" : 1.5974044799804688e-05 , "syscall" : { "name" : "close" , "args":{"int":4} ,  "ret" : 0 } }{ "start" : 1625837000.7112801 , "duration" : 1.6927719116210938e-05 , "syscall" : { "name" : "fstat" , "args" : {"fd":1 , "buff":"0x7ffce7dc11e0"} ,  "ret" : 0 } }{ "start" : 1625837000.711309 , "duration" : 3.3140182495117188e-05 , "syscall" : { "name" : "openat" , "args" : {"dfd":-100, "path":"/proc/self/cmdline" , "flag":0, "mode":0} ,  "ret" : 4 } }{ "start" : 1625837000.7113571 , "duration" : 1.5974044799804688e-05 , "syscall" : { "name" : "fstat" , "args" : {"fd":4 , "buff":"0x7ffce7dc11e0"} ,  "ret" : 0 } }{ "start" : 1625837000.711458 , "duration" : 2.09808349609375e-05 , "syscall" : { "name" : "read" , "args" : {"fd":4 , "buff":"0x7f78da38a000", "size":131072} ,  "ret": 23 } }{ "start" : 1625837000.711493 , "duration" : 2.002716064453125e-05 , "syscall" : { "name" : "write" , "args" : {"fd":1 , "buff":"0x7f78da38a000", "size":23} ,  "ret": 23 } }{ "start" : 1625837000.7115331 , "duration" : 2.5987625122070312e-05 , "syscall" : { "name" : "read" , "args" : {"fd":4 , "buff":"0x7f78da38a000", "size":131072} ,  "ret": 0 } }{ "start" : 1625837000.711643 , "duration" : 2.9087066650390625e-05 , "syscall" : { "name" : "close" , "args":{"int":4} ,  "ret" : 0 } }{ "start" : 1625837000.7117009 , "duration" : 1.5974044799804688e-05 , "syscall" : { "name" : "close" , "args":{"int":1} ,  "ret" : 0 } }{ "start" : 1625837000.711731 , "duration" : 1.6927719116210938e-05 , "syscall" : { "name" : "close" , "args":{"int":2} ,  "ret" : 0 } }
```

## Sample Tool (using monitor FD)

```
./run.py cat /proc/self/cmdline
```

Outputs:

```
cat/proc/self/cmdline
[stdout]
    READ  : 0 operations for 0 bytes
    WRITE : 0 operations for 0 bytes
[stderr]
    READ  : 0 operations for 0 bytes
    WRITE : 0 operations for 0 bytes
[stdin]
    READ  : 0 operations for 0 bytes
    WRITE : 0 operations for 0 bytes
[/etc/ld.so.cache]
    READ  : 0 operations for 0 bytes
    WRITE : 0 operations for 0 bytes
[/lib/x86_64-linux-gnu/libc.so.6]
    READ  : 1 operations for 832 bytes
    WRITE : 0 operations for 0 bytes
[/usr/lib/locale/locale-archive]
    READ  : 0 operations for 0 bytes
    WRITE : 0 operations for 0 bytes
[/proc/self/cmdline]
    READ  : 2 operations for 23 bytes
    WRITE : 0 operations for 0 bytes
```