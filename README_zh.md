**DDNet-PvP** ![GitHub Actions](https://github.com/NewTeeworldsCN/DDNet-PVP/workflows/Build/badge.svg)
===
[English](README.md)

**DDNet-PvP**是基于[**DDNet**](https://github.com/DDNet/DDNet) 15.4的Teeworlds mod，实现了用于PVP模式的房间系统.

该仓库是原[**DDNet-PvP**](https://github.com/TeeworldsCN/ddnet-pvp)的分支，包含了一些在DDNet 15.4之后加入的新特性。

如果你不想使用房间系统，你可以禁用它，或者改用[**ddnet-insta**](https://github.com/ddnet-insta/ddnet-insta)。 

# 新特性 (相较于原DDNet-PVP)
- Http服务器注册
- DDNet HUD
- 128玩家支持 (没有向后兼容)

# Linux的依赖库
在Debian或Ubuntu上对于需要的依赖, 你可以用以下指令安装:
```
sudo apt install build-essential cmake libcurl4-openssl-dev libpnglite-dev libsqlite3-dev ninja-build python-is-python3 python3 zlib1g-dev -y
```

# 构建
另见[README.md (ddnet/ddnet)](https://github.com/ddnet/ddnet/blob/master/README.md)
