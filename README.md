# <img alt="image" src="https://github.com/user-attachments/assets/6f96adf9-1867-4ca6-b627-1bd0ab7ee494" />

## Introduction

YLA (Yggdrasil LuA) is a Lua scripting engine for [Yggdrasilcore](https://github.com/YggdrasilWotLK/yggdrasilcore), based on AzerothCore's fork of ElunaLuaEngine (mod-yla). It lets server administrators and developers create custom gameplay features, events and mechanics without modifying the core server code.

[![Lua](https://img.shields.io/badge/Lua-5.2-2C2D72?style=for-the-badge&logo=lua&logoColor=white)](http://www.lua.org/manual/5.2/)
[![Lua](https://img.shields.io/badge/Lua-jit-2C2D72?style=for-the-badge&logo=lua&logoColor=white)](http://www.lua.org/manual/jit/)

Read more about Yggdrasil WoW and see Yggdrasilcore in action [here](https://yggdrasilwow.com/).

## Compatibility

> [!IMPORTANT]
> Yggdrasil LuA is NOT compatible with AzerothCore, ALE or Eluna. It includes significant performance improvements, including multistate support and thread safety alignments with the Yggdrasilcore core, that are incompatible with stock AzerothCore scripts. Scripts written for stock ALE or Eluna will not work with YLA, and scripts written for YLA will not work with them.

YLA has diverged from mod-yla and from the original Eluna project. Its API, state handling and threading model differ from upstream, so scripts are not interchangeable in either direction. Always refer to YLA specific behavior when developing scripts, and do not expect upstream documentation to be accurate for it.

If you need stock ALE or Eluna compatibility, use the upstream projects instead: [mod-yla](https://github.com/azerothcore/mod-ale) for AzerothCore's ALE or [ElunaTrinityWotlk](https://github.com/ElunaLuaEngine/ElunaTrinityWotlk) for the original Eluna API.

## Installation

### Prerequisites
- [Yggdrasilcore](https://github.com/YggdrasilWotLK/yggdrasilcore) server installation
- Git
- CMake

### Installation Steps

```bash
# Navigate to your Yggdrasilcore modules directory
cd <yggdrasilcore-path>/modules

# Clone the mod-yla repository
git clone https://github.com/YggdrasilWotLK/mod-yla.git

# Configure build with your preferred Lua version
cd <yggdrasilcore-build-directory>
cmake ../ -DLUA_VERSION=luajit  # Options: luajit, lua52, lua53, lua54

# Default: If no version is specified, Lua 5.2 will be used

# Rebuild your Yggdrasilcore server
make -j$(nproc)
```

### Supported Lua Versions
- LuaJIT (recommended for performance)
- Lua 5.2 (default)
- Lua 5.3
- Lua 5.4

## Documentation

- [Installation Guide](https://github.com/YggdrasilWotLK/mod-yla/tree/master/docs/USAGE.md)
- [Implementation Details](https://github.com/YggdrasilWotLK/mod-yla/tree/master/docs/IMPL_DETAILS.md)
- [Hooks Documentation](https://github.com/YggdrasilWotLK/mod-yla/blob/master/src/LuaEngine/Hooks.h)
- [Lua 5.2 Reference](http://www.lua.org/manual/5.2/)

The upstream [mod-yla API Documentation](https://www.azerothcore.org/eluna/) is a reference for the shared base only and may not match YLA.

## Support

YLA is a downstream fork and we do not provide public support for it. Upstream channels do not support YLA specific behavior. However, if you do decide to use YLA in your server and find any issues, feel free to report bugs and/or open PRs here. You may also find help in the AzerothCore Discord community's mod-yla channel due to the similar nature of these code bases.

- [YLA GitHub Issues](https://github.com/YggdrasilWotLK/mod-yla/issues)
- [Discord Community](https://discord.com/invite/bx3y5Qmy)

## Contributing

YLA is a privately managed repository. We do not recommend emulation enthusiasts to follow it. Instead, we recommend that enthusiasts contribute to the upstream mod-yla project at [azerothcore/mod-ale](https://github.com/azerothcore/mod-ale). See their information on how to contribute [here](https://www.azerothcore.org/wiki/contribute).

## Acknowledgements

YLA is built upon mod-yla, which in turn is built upon the original [Eluna](https://github.com/ElunaLuaEngine/Eluna) project. We thank the original Eluna team for their pioneering work in Lua scripting for World of Warcraft server emulators.

- [Original Eluna Repository](https://github.com/ElunaLuaEngine/Eluna)
- [Eluna Discord Community](https://discord.gg/bjkCVWqqfX)

## License

- Source: Contrary to some of the source documentation and previous information distributed by AzerothCore, all Yggdrasilcore source components are licensed under GNU GPL v2.
- Intellectual property: YLA is not related to Blizzard Entertainment. Yggdrasil WoW and its derivative projects lay no claim to Blizzard Entertainment's copyrights and intellectual property, and operate this project solely as an avenue for exploring the functionality of the abandonware WotLK 3.3.5a client in an educational capacity.
