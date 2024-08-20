# Compiler Flag Benchmarks for Linux

Do `AVX2` instructions have benefits to execution speed?

=> It actually makes things **worse**. Best to compile without `-march`  flags. 

=> This is strange, but repeatable in all tests. Architecture and `-mtune`  is set correctly.



Is `-O3` superior to `-O2`?

=> **Yes** `-O3` increases performance by 5-20%



Is there is significant difference between `gcc`  and `clang`?

=> Not really. It depends.

=> `gcc`  is up to 10% **faster** than `clang` in some tests

=> `gcc`  is up to 5% **slower** than `clang` in some tests



Does **position independent code** decrease performance?

=> **No**, within margins of errors.



Does **link time optimization** increase performance?

=> **Yes** significantly, by up to 70%



## System

* AMD Ryzen 7 5800H
* White Power Profile



## Kubuntu 24.04 - VMWare

* Units in M/s

* Release Build
  * g++ XXX
  * clang XXX
  * Qt 6.4
  
* No LTO

| Compiler |                        | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elemens |
| -------- | ---------------------- | ------------ | ------------- | ------------- | ------------ |
| clang-18 | -O3                    | 9.7          | 10.8          | 7.5           |              |
| clang-18 | -O3 <br/>-march=znver3 | 9.6          | 11.5          | 7.3           |              |
| gcc-13   | -O3                    | 9.6          | 11.7          | 8.2           |              |
| gcc-13   | -O3<br/>-march=znver3  | 8.6          | 11.0          | 7.0           |              |

## Ubuntu 22.04 - VMWare

* Units in M/s

* Release Build

  * g++ 13.1.0

  * clang 18.1.8

  * Qt 6.2 with commenting out 3 lines

* No LTO

| Compiler |                       | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elements |
| -------- | --------------------- | ------------ | ------------- | ------------- | ------------- |
| clang-18 | -O3                   | 10.3         | 11.3          | 7.9           |               |
| clang-18 | -O3<br/>-march=native | 9.7          | 11.6          | 7.8           |               |
| gcc-13   | -O3                   | 9.8          | 12.2          | 8.9           |               |



## Ubuntu 22.04 - Laptop

* Units in M/s

* Release Build

  * g++ 13.1.0

  * clang 18.1.8

* No LTO
* With position independent code

| Compiler |                       | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elements | Uninsert / Insert    |
| -------- | --------------------- | ------------ | ------------- | ------------- | ------------- | -------------------- |
| clang-18 | -O3                   | 10.8         | 11.6          | 8.5           | 1173 ms       | 1995 ms<br />2318 ms |
| clang-18 | -O3<br/>-march=native | 10.8         | 12.1          | 8.5           | 1156 ms       | 2105 ms<br />2411 ms |
| gcc-13   | -O3                   | 10.2         | 12.4          | 9.4           | 1290 ms       | 2098 ms<br />2605 ms |
| gcc-13   | -O3<br/>-march=native | 9.9          | 11.9          | 8.0           | 1283 ms       | 2290 ms<br />2703 ms |

* With LTO
* With position independent code

| Compiler     |                       | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elements | Uninsert / Insert        |
| ------------ | --------------------- | ------------ | ------------- | ------------- | ------------- | ------------------------ |
| **clang-18** | **-O3**               | **16.44**    | **18.15**     | **13.93**     | **1065 ms**   | **1854 ms<br />2175 ms** |
| clang-18     | -O3<br/>-march=native | 15.46        | 17.88         | 13.80         | 1048 ms       | 2002 ms<br />2201 ms     |
| **gcc-13**   | **-O3**               | **17.68**    | **20.16**     | **13.25**     | **1107 ms**   | **1845 ms<br />2207**    |
| gcc-13       | -O3<br/>-march=native | 17.49        | 20.07         | 14.36         | 1048 ms       | 1880 ms<br />2353 ms     |

* With LTO
* With position independent code

| Compiler |                       | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elements | Uninsert / Insert    |
| -------- | --------------------- | ------------ | ------------- | ------------- | ------------- | -------------------- |
| clang-18 | -O2                   | 15.93        | 17.19         | 12.06         | 1066 ms       | 1865 ms<br />2166 ms |
| clang-18 | -O2<br/>-march=native | 16.02        | 17.83         | 12.26         | 1048 ms       | 1957 ms<br />2188 ms |
| gcc-13   | -O2                   | 14.77        | 17.28         | 13.02         | 1194 ms       | 1953 ms<br />2323 ms |
| gcc-13   | -O2<br/>-march=native | 14.21        | 17.07         | 13.86         | 1120 ms       | 1975 ms<br />2408 ms |

* With LTO
* No position indepenent code

| Compiler |                       | Simulation_0 | Inverter_Loop | RenderScene_0 | Load Elements | Uninsert / Insert    |
| -------- | --------------------- | ------------ | ------------- | ------------- | ------------- | -------------------- |
| clang-18 | -O3                   | 16.53        | 18.15         | 13.65         | 1067 ms       | 1868 ms<br />2161 ms |
| clang-18 | -O3<br/>-march=native | 15.14        | 17.82         | 13.79         | 1038 ms       | 1994 ms<br />2210 ms |
| gcc-13   | -O3                   | 17.75        | 20.06         | 13.36         | 1095 ms       | 1853 ms<br />2194 ms |
| gcc-13   | -O3<br/>-march=native | 17.12        | 19.65         | 13.53         | 1071 ms       | 1890 ms<br />2429 ms |

=> Position independent code has no significant impact on performance
