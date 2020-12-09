# DFS

## 编译安装

仅支持Linux环境

项目目录下 `make` 即可，得到master、slave、test三个可执行程序

先运行master，可添加-d选项以守护进程的模式运行，需要退出时手动kill，参考命令如下，请 **确保** 其他进程名不包含 **master** 等信息


```
ps -ef | grep master | grep -v grep | awk '{print $2}' | xargs kill -9
```

再运行slave，如果需要在其他设备上运行slave，将include/master.h 中的 `#define MASTER_NET "127.0.0.1"` ip改为master所在设备的ip


现包含两个版本

- sfs版本仅仅使用操作系统提供文件系统，block块直接放在disk目录下

- master版本使用bfs（bare file system）文件系统, data node在 disk/disk文件中，其中disk可以是文件，如`dd if=/dev/zero of=disk bs=1M count=10000`创建，也可以是硬盘，直接`ln -s /dev/sdX disk`创建符号链接就可以使用


### master

master管理name node，监听TCP端口 38888、58888

38888端口用来与slave通信

58888端口用来管理维护master

### slave

slave管理data node，监听TCP端口48888，用来与master通信

### test

单元测试文件，测试代码请写在此处

## 相关功能

### 查看slave信息

```
echo status | nc 127.0.0.1 58888
```

### 查看目录

```
echo ls path | nc 127.0.0.1 58888
```

### 查看所有name node

```
echo NameNodeInfo | nc 127.0.0.1 58888
```

### 创建目录

```
echo mkdir *path* | nc 127.0.0.1 58888
```

### 存文件

```
echo mvFromLocal spath dpath | nc 127.0.0.1 58888
```

其中dpath必须写全，不能省略尾部的文件名

###

```
echo mvToLocal spath dpath | nc 127.0.0.1 58888
```

## 实现机制

epoll事件驱动 + 网络通信 + 匿名管道IPC

master多线程IO加速

[未完成]：slave多线程读写磁盘

slave拥有mem缓存系统，基于LRU调度策略

bfs自定义文件系统，伙伴系统磁盘分配算法

