# CS2 Flashlight

![Build](https://github.com/XBDJ504764827/mm_cs2flashlight/actions/workflows/ci.yml/badge.svg)
![License](https://img.shields.io/github/license/XBDJ504764827/mm_cs2flashlight)

一个基于 **Metamod:Source** 的 Counter-Strike 2 手电筒插件。玩家在游戏中按下 F 键即可切换手电筒，适合需要在地图中恢复或提供手电筒功能的社区服务器。

## 功能

- 识别 CS2 的 `impulse 100` 手电筒命令。
- 兼容常见的 `+lookatweapon` 绑定，因此默认 F 键配置通常可以直接使用。
- 只处理按下命令，忽略 `-lookatweapon` 释放命令，避免一次按键触发两次切换。
- 通过 Source2 的 `IServerGameClients::ClientCommand` 接口实现。
- 不依赖私有实体偏移、签名扫描或第三方运行时框架，游戏更新时维护成本更低。
- 自动生成 Metamod VDF 文件并打包 Linux CS2 插件。

插件不会强制修改玩家的键位配置。如果服务器需要明确要求 F 键绑定手电筒，可以让玩家在 CS2 控制台执行：

```text
bind f "impulse 100"
```

## 安装

### 使用构建包

1. 从构建产物或 Release 下载插件包。
2. 将包内的内容复制到 CS2 服务端根目录，通常是 `game/csgo/`。
3. 保持目录结构不变。Linux 64 位插件的目标路径为：

   ```text
   addons/cs2_flashlight/bin/linuxsteamrt64/cs2_flashlight.so
   ```

4. VDF 文件位于：

   ```text
   addons/metamod/cs2_flashlight.vdf
   ```

5. 重启服务器后使用 `meta list` 检查插件是否加载。

## 服务器部署

下面的路径以 CS2 服务端的 `game/csgo/` 目录为基准。不要把插件文件放到客户端的 CS2 安装目录，也不要把 `.so` 或 `.dll` 直接放到 `addons/metamod/` 根目录。

### Linux 服务端

假设服务器根目录为 `/srv/cs2`，完整目录应当类似：

```text
/srv/cs2/
└── game/csgo/
    └── addons/
        ├── metamod/
        │   └── cs2_flashlight.vdf
        └── cs2_flashlight/
            └── bin/
                └── linuxsteamrt64/
                    └── cs2_flashlight.so
```

将 `build/package/` 下的内容复制到 `/srv/cs2/game/csgo/`：

```sh
cp -a build/package/cs2/. /srv/cs2/game/csgo/
```

如果你的服务器使用其他目录，只需要将命令中的 `/srv/cs2/game/csgo/` 替换为实际的 `game/csgo/` 路径。

### Windows 服务端

Windows 构建包使用相同的目录结构，但二进制文件位于 Windows 平台目录：

```text
game/csgo/addons/metamod/cs2_flashlight.vdf
game/csgo/addons/cs2_flashlight/bin/win64/cs2_flashlight.dll
```

当前 GitHub Actions 默认构建 Linux x86_64 包。如果需要 Windows DLL，应在安装了 MSVC/Visual Studio Build Tools 的环境中使用 AMBuild 的 Windows 目标重新构建。

### VDF 文件说明

`addons/metamod/cs2_flashlight.vdf` 是 Metamod 自动加载配置，内容由 `PackageScript` 生成，不需要手工修改：

```text
"Metamod Plugin"
{
    "alias" "flashlight"
    "file" "addons/cs2_flashlight/bin/linuxsteamrt64/cs2_flashlight"
}
```

如果服务器没有自动加载 VDF，也可以把插件路径加入 `addons/metamod/metaplugins.ini`，但通常不需要同时使用两种加载方式，以免重复加载。

### 部署后检查

1. 重启 CS2 服务端，或在确认服务器支持热加载时执行 `meta unload flashlight` 后再 `meta load addons/metamod/cs2_flashlight.vdf`。
2. 在服务器控制台执行：

   ```text
   meta list
   ```

3. 确认列表中出现 `CS2 Flashlight`，状态为 `RUN` 或 `Running`。
4. 玩家进入服务器后按 F；如果没有反应，在客户端执行 `bind f "impulse 100"`。
5. 如果插件加载失败，优先检查 `addons/metamod/`、插件二进制目录和当前平台是否匹配，并查看服务器控制台中的 Metamod 错误信息。

### 更新和卸载

更新插件时，先停止服务器，再用新构建包覆盖：

```text
game/csgo/addons/metamod/cs2_flashlight.vdf
game/csgo/addons/cs2_flashlight/bin/<platform>/cs2_flashlight.<so|dll>
```

卸载插件时停止服务器并删除 `cs2_flashlight/` 目录和 `metamod/cs2_flashlight.vdf`。不要删除整个 `addons/metamod/` 目录，因为其中还包含其他 Metamod 插件。

### 从源码构建

要求：

- Git，并支持递归 submodule
- Python 3.8 或更高版本
- AMBuild 2.2 或更高版本
- 支持 C++17 的 GCC、Clang 或 MSVC
- Metamod:Source 的 Source2 开发版本

```sh
git clone --recurse-submodules https://github.com/XBDJ504764827/mm_cs2flashlight.git
cd mm_cs2flashlight
mkdir build
cd build
python3 ../configure.py --sdks cs2 --targets x86_64 --enable-optimize
ambuild
```

构建完成后，完整安装包位于：

```text
build/package/
```

如果依赖放在其他目录，可以通过以下参数指定路径：

```text
--mms_path
--hl2sdk-manifests
--hl2sdk-root
```

## 工作原理

插件在服务端注册 `IServerGameClients::ClientCommand` 钩子：

1. 玩家按下 F 键后，CS2 通常会发送 `+lookatweapon`，或者直接发送 `impulse 100`。
2. 插件识别这两个命令，并阻止原始命令继续执行。
3. 插件通过 `IVEngineServer::ClientCommand` 向同一玩家发送 `impulse 100`。
4. 客户端执行原生手电筒切换逻辑。

这种方式不需要访问 CS2 私有实体结构，也不需要维护游戏二进制偏移。

## 项目结构

```text
.
├── src/                    # 插件 C++ 源码
├── AMBuildScript           # AMBuild 主配置和插件版本生成逻辑
├── AMBuilder               # C++ 目标定义
├── PackageScript           # VDF 和发布包定义
├── configure.py            # 构建参数入口
├── plugin-metadata.json    # 插件名称、作者和版本模板
├── scripts/
│   ├── bump_version.py     # 自动递增 patch 版本
│   └── test_bump_version.py
├── .github/workflows/ci.yml # CI、构建和自动版本更新
├── metamod-source/         # Metamod:Source submodule
└── hl2sdk-cs2/             # CS2 HL2SDK submodule
```

## CI 和版本管理

`.github/workflows/ci.yml` 会在 Pull Request 和分支推送时执行：

- Python 单元测试
- `plugin-metadata.json` JSON 校验
- 使用真实 Metamod/CS2 SDK 的 Linux x86_64 AMBuild 构建
- 上传 `build/package` 构建产物

当代码成功推送或合并到 `main` 后，CI 会自动执行 `scripts/bump_version.py`，将插件的 patch 版本递增。例如：

```text
1.0.0.{{git-shorthash}} -> 1.0.1.{{git-shorthash}}
```

自动提交使用 `[skip ci]`，不会造成版本更新工作流循环。编译版本还会包含 Git short hash，便于定位具体构建。

## 直接依赖

| 依赖 | 用途 | GitHub |
| --- | --- | --- |
| Metamod:Source | 插件加载、SourceHook 和 Metamod API | [alliedmodders/metamod-source](https://github.com/alliedmodders/metamod-source) |
| HL2SDK CS2 | CS2 Source2 接口、引擎头文件和库 | [alliedmodders/hl2sdk](https://github.com/alliedmodders/hl2sdk/tree/cs2) |
| AMBuild | C++ 编译、链接和插件打包 | [alliedmodders/ambuild](https://github.com/alliedmodders/ambuild) |

Metamod:Source 自身还会递归使用 `hl2sdk-manifests` 和 `amtl` 等依赖，均通过 submodule 自动获取。

## 参考项目

本项目沿用了以下项目的接口基础、目录组织或 CI 思路：

- [zer0k-z/mm_misc_plugins](https://github.com/zer0k-z/mm_misc_plugins) - Source2 Metamod 最小插件基础
- [FemboyKZ/mm-cs2rockthevote](https://github.com/FemboyKZ/mm-cs2rockthevote) - CS2 Metamod 插件目录、构建和 GitHub Actions 参考
- [Metamod:Source S2 sample](https://github.com/alliedmodders/metamod-source/tree/master/samples/s2_sample_mm) - 官方 Source2 插件示例
- [SourceHook](https://github.com/alliedmodders/metamod-source/tree/master/core/sourcehook) - Metamod 的 C++ 函数钩子系统

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
