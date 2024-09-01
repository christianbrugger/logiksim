|                        | win-msvc-debug | win-msvc-release | win-clang-debug | win-clang-release |
| ---------------------- | -------------- | ---------------- | --------------- | ----------------- |
| ccache first<br />PCH  | 2:01<br />1:56 | 5:56             | 2:13            | 2:35              |
| ccache second<br />PCH | 0:12<br />0:10 | 4:29             | 0:15            | 0:39              |
| Misses on second       | 0/803          | 0/803            | 1/803           | 1/803             |



Kubuntu 24.04 VMWare

|                        | linux-gcc-debug | linux-gcc-release | linux-clang-debug | linux-clang-release |
| ---------------------- | --------------- | ----------------- | ----------------- | ------------------- |
| Cmake Configure        | 0:11            | 0.11              | 0:15              | 0:16                |
| ccache first<br />PCH  | 4:52            | 5:00              | 2:58              | 3:47                |
| ccache second<br />PCH | 0:13            | 1:34              | 0:09              | 0:52                |
| Misses on second       | 0/800           | 0/800             | 0/800             | 0/800               |
| Cache Size             |                 | 172 MB            |                   | 132 MB              |





### Github Actions

First: https://github.com/christianbrugger/logiksim/actions/runs/10554706697

Second: https://github.com/christianbrugger/logiksim/actions/runs/10554900384
Third: https://github.com/christianbrugger/logiksim/actions/runs/10555779471

| Debug                  | win-msvc-debug | win-clang-debug | linux-gcc-debug | linux-clang-debug |
| ---------------------- | -------------- | --------------- | --------------- | ----------------- |
| Cmake Configure        | 0:44           | 1:08<br />0:43  | 0:09            | 0:13              |
| ccache first<br />PCH  | 6:02           | 6:38            | 8:12            | 6:22              |
| ccache second<br />PCH | 0:22<br />0:20 | 0:19<br />0:20  | 0:15<br />0:16  | 0:11<br />0:11    |
| Misses on second       | 0/803          | 0/803           | 0/800           | 0/801             |
| Cache Overhead D/U     | 0:16<br />0:13 | 0:13<br />0:08  | 0:08<br />0:06  | 0:09<br />0:09    |
| Cache Size             | 229 MB         | 136 MB          | 224 MB          | 163 MB            |

First: https://github.com/christianbrugger/logiksim/actions/runs/10555779471

Second: https://github.com/christianbrugger/logiksim/actions/runs/10556031842

| Release                | win-msvc-release | win-clang-release | linux-gcc-release | linux-clang-release |
| ---------------------- | ---------------- | ----------------- | ----------------- | ------------------- |
| Cmake Configure        | 0:40             | 1:04              | 0:09              | 0:13<br />0:13      |
| ccache first<br />PCH  | 15:10            | 7:42              | 9:57              | 8:04                |
| ccache second<br />PCH | 10:19            | 1:25              | 3:13              | 1:36                |
| Misses on second       | 0/803            | 0/803             | 0/801             | 0/800               |
| Cache Overhead D/U     | 0:16             | 0:05              | 0:06              | 0:04                |
| Cache Size             | 462 MB           | 89 MB             | 180 MB            | 122 MB              |

First: https://github.com/christianbrugger/logiksim/actions/runs/10644687883

Second: https://github.com/christianbrugger/logiksim/actions/runs/10644759613

| Release                | win-msvc-release-no-lto | linux-gcc-release-no-lto |
| ---------------------- | ----------------------- | ------------------------ |
| Cmake Configure        | 0:28<br />0:31          | 0:08<br />0:08           |
| ccache first<br />PCH  | 9:05                    | 8:34                     |
| ccache second<br />PCH | 0:14                    | 0:11                     |
| Misses on second       | 0/803                   | 0/801                    |
| Cache Overhead D/U     | 0:10                    | 0:03                     |
| Cache Size             | 136 MB                  | 98 MB                    |





Linux Sanitizers

First: https://github.com/christianbrugger/logiksim/actions/runs/10652376302/job/29526125757

First Thread: https://github.com/christianbrugger/logiksim/actions/runs/10653013996/job/29527610298























