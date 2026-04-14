# firsth7

基于 **STM32H750** 的 LVGL 嵌入式 GUI 演示项目。

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 功能

| 模块 | 说明 |
|------|------|
| 主菜单 | 带图标的深色风格列表菜单，包含显示、网络、音频、灯控、计算器、时钟、关于等入口 |
| 计算器 | 支持四则运算、退格、正负号切换的全功能计算器 |
| 时钟 | 包含时钟显示、正计时（计时器）、倒计时（秒表）三种模式 |
| 灯控 | 通过 LVGL 开关控制板载 RGB LED（PC0/PC1/PC2） |
| 图片显示 | 通过 QSPI Flash 加载图片资源（SFUD 驱动） |

## 效果预览

| 主菜单 | 计算器 | 时钟 |
|--------|--------|------|
| ![主菜单](docs/preview_menu.png) | ![计算器](docs/preview_calc.png) | ![时钟](docs/preview_clock.png) |

> 预览图来自 LVGL PC Simulator，实机效果以开发板为准。

## 硬件

| 项目 | 说明 |
|------|------|
| MCU | STM32H750VBTx（Cortex-M7 @ 480 MHz，128 KB Flash + 1 MB RAM） |
| 显示屏 | 3.5 寸 ST7789 SPI TFT（320×480） |
| 触摸 | FT6336 I²C 电容触摸 |
| SPI Flash | QSPI NOR Flash（SFUD 驱动，用于存储图片资源） |
| LED | RGB LED × 1（PC0 / PC1 / PC2，低电平点亮） |

### 引脚定义

| 信号 | 引脚 |
|------|------|
| LCD CS | PE2 |
| LCD DC | PE5 |
| LCD RST | PE6 |
| LCD BL | PE0 |
| TP CS | PA0 |
| TP IRQ | PA1 |
| LED R/G/B | PC0 / PC1 / PC2 |

## 软件依赖

| 库 | 版本 | 许可证 |
|----|------|--------|
| [LVGL](https://github.com/lvgl/lvgl) | v8.x | MIT |
| STM32H7xx HAL Driver | 随 STM32CubeMX 生成 | BSD-3-Clause |
| CMSIS | 随 STM32CubeMX 生成 | Apache-2.0 |
| [SFUD](https://github.com/armink/SFUD) | — | MIT |

## 构建

### 工具链要求

- CMake ≥ 3.22
- `arm-none-eabi-gcc` 工具链（推荐通过 [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) 安装）
- `ninja`（可选，推荐）

### 编译步骤

```bash
# 配置（Debug）
cmake -B build/Debug -DCMAKE_BUILD_TYPE=Debug \
      --preset Debug   # 或使用 CMakePresets.json 内置预设

# 构建
cmake --build build/Debug --parallel

# 产物
# build/Debug/firsth7.elf
# build/Debug/firsth7.bin
```

### 烧录

```bash
# 使用 OpenOCD（ST-Link）
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
        -c "program build/Debug/firsth7.elf verify reset exit"

# 或使用 STM32CubeProgrammer CLI
STM32_Programmer_CLI -c port=SWD -d build/Debug/firsth7.bin 0x08000000 -rst
```

## 目录结构

```
firsth7/
├── BSP/LCD_ST7789/   # ST7789 驱动 + LVGL 显示/触摸移植层
├── calculator/       # 计算器应用
├── clock/            # 时钟/计时/秒表应用
├── cmake/            # CMake 工具链与 STM32CubeMX 生成配置
├── Core/             # STM32CubeMX 生成的主程序与外设初始化
├── Drivers/          # CMSIS + STM32H7xx HAL Driver
├── lvgl/             # LVGL 图形库（子模块）
├── picture/          # 图片 QSPI 读取模块
├── sfud/             # Serial Flash Universal Driver
├── ui/               # LVGL UI 主菜单与页面
├── lv_conf.h         # LVGL 配置
├── STM32H750XX_FLASH.ld  # 链接脚本
└── CMakeLists.txt    # 顶层构建脚本
```

## 开发说明

- LVGL 显示缓冲区需放在 AXI SRAM / D1 RAM（不能使用 DTCMRAM），否则 DMA 无法访问。链接脚本中使用 `.dma_buffer` 段映射到 `RAM_D1`。
- 中文字体使用自定义 `lv_font_custom_cn16`，涵盖常用 CJK 汉字子集，随源码一同提供。
- 如需修改 LVGL 功能开关，编辑根目录 `lv_conf.h`；构建系统通过 `LV_CONF_PATH` 变量将其传递给 LVGL。

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

本项目源码以 [MIT License](LICENSE) 开源。第三方组件保留各自的原始许可证（详见 `LICENSE` 文件底部说明）。
