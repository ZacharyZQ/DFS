# DFS

## 编译安装

仅支持Linux环境

项目目录下 `make` 即可，得到master、slave、test三个可执行程序

先运行master，可添加-d选项以守护进程的模式运行，需要退出时手动kill，参考命令如下，请确保其他进程名不包含master等信息

```
ps -ef | grep master | grep -v grep | awk '{print $2}' | xargs kill -9
```

再运行slave，如果需要在其他设备上运行slave，将include/master.h 中的 `#define MASTER_NET "127.0.0.1"` ip改为master所在设备的ip

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

epoll事件驱动

master多线程IO加速

