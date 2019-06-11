# OSProject1

Code for user-defined scheduling FIFO, RR, SJF, PSJF.

```
env: linux ubuntu 16.04 gcc 5.4.0
cmd: cd kernel_files && make && cd .. && gcc main.c -o main -std=c99
run: ./main               (stdin)
     ./main < <inputfile> (filestream)
You need to have root priviledge.
The directory 'origin' is neglectable.
```

Message

```
stdout: process name + process id
dmesg : process id + start time + end time
```