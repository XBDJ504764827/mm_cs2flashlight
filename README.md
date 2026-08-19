# CS2 Flashlight

![Build](https://github.com/XBDJ504764827/mm_cs2flashlight/actions/workflows/ci.yml/badge.svg)
![License](https://img.shields.io/github/license/XBDJ504764827/mm_cs2flashlight)

一个基于 **Metamod:Source** 的 Counter-Strike 2 手电筒插件。玩家在游戏中按下 F 键即可切换手电筒，适合需要在地图中恢复或提供手电筒功能的社区服务器。

## 功能

- 读取 CS2 原生 `IN_LOOK_AT_WEAPON` 输入位，支持玩家默认的 F 检视键。
- 同时检查“当前按下”和“本帧变化”状态，一次按键只切换一次手电筒。
- 保留原版武器检视动作，不拦截 F，也不修改玩家的客户端按键绑定。
- 在服务端创建带官方 flashlight cookie 的 `light_barn`，并附着到玩家视角方向。
- 玩家死亡、断开、换图或插件卸载时自动清理灯光实体。
- 实体字段通过 Source 2 Schema 动态解析；少量引擎内部函数通过随包发布的 gamedata 定位。
- 自动生成 Metamod VDF 文件并打包 Linux CS2 插件。

默认情况下，按一次 F 会同时播放原版武器检视并打开或关闭手电筒。这两个行为可以共存，不需要将 F 重新绑定到 `impulse 100`。

## 安装

### 使用构建包

1. 从 GitHub Releases 下载最新的 `cs2-flashlight-X.Y.Z-linux-x86_64.zip`。
2. 将压缩包内的内容复制到 CS2 服务端的 `game/csgo/` 目录。
3. 保持目录结构不变。Linux 64 位插件的目标路径为：

   ```text
   addons/cs2_flashlight/bin/linuxsteamrt64/cs2_flashlight.so
   ```

4. VDF 文件位于：

   ```text
   addons/metamod/cs2_flashlight.vdf
   ```

5. 确认签名配置文件也已安装：

   ```text
   addons/cs2_flashlight/gamedata/cs2_flashlight.games.txt
   ```

6. 重启服务器后使用 `meta list` 检查插件是否加载。

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
            ├── bin/
            │   └── linuxsteamrt64/
            │       └── cs2_flashlight.so
            └── gamedata/
                └── cs2_flashlight.games.txt
```

将 Release 压缩包解压到 `/srv/cs2/game/csgo/`，或者将本地构建包复制到该目录：

```sh
unzip cs2-flashlight-X.Y.Z-linux-x86_64.zip -d /srv/cs2/game/csgo/
# 本地源码构建包也可以这样部署：
cp -a build/package/cs2/. /srv/cs2/game/csgo/
```

如果你的服务器使用其他目录，只需要将命令中的 `/srv/cs2/game/csgo/` 替换为实际的 `game/csgo/` 路径。

### Windows 服务端

Windows 构建包使用相同的目录结构，但二进制文件位于 Windows 平台目录：

```text
game/csgo/addons/metamod/cs2_flashlight.vdf
game/csgo/addons/cs2_flashlight/bin/win64/cs2_flashlight.dll
game/csgo/addons/cs2_flashlight/gamedata/cs2_flashlight.games.txt
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
4. 玩家存活时按 F，确认武器检视和手电筒切换同时发生。
5. 如果插件加载失败，检查二进制、VDF 和 gamedata 三个文件是否都在正确位置，并查看服务器控制台中的 Metamod 错误信息。
6. 如果日志提示 `Unable to resolve engine function`，通常表示 CS2 更新后签名已变化，需要更新 `cs2_flashlight.games.txt` 或安装新的 Release；重新绑定 F 无法解决该错误。

### 更新和卸载

更新插件时，先停止服务器，再用新构建包覆盖：

```text
game/csgo/addons/metamod/cs2_flashlight.vdf
game/csgo/addons/cs2_flashlight/bin/<platform>/cs2_flashlight.<so|dll>
game/csgo/addons/cs2_flashlight/gamedata/cs2_flashlight.games.txt
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

插件在服务端注册 `IServerGameDLL::GameFrame` 前置钩子：

1. 每帧通过玩家 `CPlayer_MovementServices::m_nButtons` 读取输入状态。
2. 当 `IN_LOOK_AT_WEAPON` 同时出现在按下状态和变化状态中时，确认这是一次新的 F 按键。
3. 首次按键创建 `light_barn`，配置亮度、范围、阴影和官方 `flashlight.vtex` light cookie，再附着到玩家的 `clip_limit`。
4. 后续按键向灯光实体发送 `Enable` 或 `Disable` 输入。
5. 插件不消费原始输入，因此 CS2 原版武器检视继续正常执行。

Schema 字段偏移在运行时从 `ISchemaSystem` 获取，不写死到代码中。`CreateEntityByName`、`DispatchSpawn` 和 `CEntityInstance_AcceptInput` 属于未公开的游戏函数，其平台签名保存在 `gamedata/cs2_flashlight.games.txt`；CS2 大型更新后可能需要更新该文件。

## 项目结构

```text
.
├── src/                    # 插件 C++ 源码
├── gamedata/               # Linux/Windows 引擎函数签名
├── AMBuildScript           # AMBuild 主配置和插件版本生成逻辑
├── AMBuilder               # C++ 目标定义
├── PackageScript           # VDF 和发布包定义
├── configure.py            # 构建参数入口
├── plugin-metadata.json    # 插件名称、作者和版本模板
├── scripts/
│   ├── release_version.py  # 根据 Git 标签计算并写入发布版本
│   ├── test_release_version.py
│   ├── bump_version.py     # 手动递增版本的兼容脚本
│   ├── test_bump_version.py
│   └── test_gamedata.py    # gamedata 完整性检查
├── .github/workflows/ci.yml # CI、构建和自动版本更新
├── metamod-source/         # Metamod:Source submodule
└── hl2sdk-cs2/             # CS2 HL2SDK submodule
```

## CI 和版本管理

`.github/workflows/ci.yml` 会在 Pull Request 和分支推送时执行：

- Python 单元测试
- `plugin-metadata.json` JSON 校验
- 使用真实 Metamod/CS2 SDK 的 Linux x86_64 AMBuild 构建
- 校验二进制、VDF 和 gamedata 的安装目录结构
- 上传 `build/package` 构建产物

当 `develop` 合并到 `main`（也就是产生一次 `main` push），CI 还会：

- 将 `build/package/cs2/` 下的 `addons/` 目录打包为可直接安装的 ZIP。
- 根据已有的 `vX.Y.Z` 标签计算下一个 patch 版本，例如已有 `v1.0.0` 时发布 `v1.0.1`。
- 在构建前把本次版本写入插件元数据，因此二进制中的版本号与 Release 一致。
- 创建 GitHub Release，例如 `CS2 Flashlight v1.0.1`，并上传 `cs2-flashlight-1.0.1-linux-x86_64.zip`。
- 将标签明确指向本次 `main` 合并提交，并自动生成 Release Notes。
- 发布成功后把相同版本同步回 `main` 的 `plugin-metadata.json`。

Release 压缩包的根目录是 `addons/`，解压目标就是 CS2 服务端的 `game/csgo/`，不需要再次移动目录。

同一个 `main` 提交重复运行工作流时，如果标签已经指向该提交，CI 会复用该版本并更新同一个 Release，不会额外递增版本。例如：

```text
v1.0.1 -> v1.0.1
```

新的 `main` 合并提交才会递增版本；`develop` 分支和 Pull Request 只执行检查、构建和构建产物上传，不创建 Release。版本同步提交使用 `[skip ci]`，且由 `GITHUB_TOKEN` 推送，不会造成工作流循环。编译版本还会包含 Git short hash，便于定位具体构建。

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
- [Source2ZE/CS2Fixes](https://github.com/Source2ZE/CS2Fixes) - CS2 输入状态、`light_barn` 行为和签名维护参考；不是运行依赖
- [Metamod:Source S2 sample](https://github.com/alliedmodders/metamod-source/tree/master/samples/s2_sample_mm) - 官方 Source2 插件示例
- [SourceHook](https://github.com/alliedmodders/metamod-source/tree/master/core/sourcehook) - Metamod 的 C++ 函数钩子系统

## 许可证

本项目使用 MIT License，详见 [LICENSE](LICENSE)。
