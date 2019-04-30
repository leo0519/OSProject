# OSProject1
Sample code for user-defined scheduling.

```
env: linux ubuntu 16.04 gcc 5.4.0
cmd: cd kernel_files && make && cd .. && gcc main.c -o main -std=c99
run: ./main               (stdin)
     ./main < <inputfile> (filestream)
You need to have root priviledge.
```

Message
```
stdout: process name + process id
dmesg : process id + start time + end time
```
