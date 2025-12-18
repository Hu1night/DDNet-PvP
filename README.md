**DDNet-PvP** ![GitHub Actions](https://github.com/NewTeeworldsCN/DDNet-PVP/workflows/Build/badge.svg)
===
[中文](README_zh.md)

**DDNet-PvP** is a teeworlds mod based on DDNet 15.4, which implements a room system for PvP modes.

This repo is a fork of the original [**DDNet-PvP**](https://github.com/TeeworldsCN/ddnet-pvp), including some new features that were added to DDNet after version 15.4.

If you don't want to use the room system, you can disable it, or use [**ddnet-insta**](https://github.com/ddnet-insta/ddnet-insta) instead. 

# New Features (Compared to the original ddnet-pvp)
- Http server register
- DDNet HUD
- 128 Players support (without backcompat)

# Dependencies on Linux
You can install all required dependencies on Debian or Ubuntu like this:
```
sudo apt install build-essential cmake libcurl4-openssl-dev libpnglite-dev libsqlite3-dev ninja-build python-is-python3 python3 zlib1g-dev -y
```

# Build
See also [README.md (ddnet/ddnet)](https://github.com/ddnet/ddnet/blob/master/README.md)
