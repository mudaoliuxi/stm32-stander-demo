# 42_spi_slave — STM32H750 SPI2 从机(Slave) Demo

正点原子 **M100Z-M7 最小系统板(STM32H750VBT6)** 的 SPI2 从机实验工程。

---

## 1. 功能

| 项目 | 说明 |
|------|------|
| 外设 | SPI2，**从机模式** |
| 引脚 | `PB12=NSS` `PB13=SCK` `PB14=MISO` `PB15=MOSI`（AF5） |
| 时序 | **Mode0**（CPOL=0, CPHA=0），MSB 先发，8bit |
| Master 时钟 | 支持 **5 / 10 / 15 / 20 MHz**（SPI2 内核时钟 PCLK1=120MHz） |
| Master 写 | NSS 上升沿（一帧结束）后，从机把本帧数据以十六进制从 **串口1 @115200** 打印 |
| Master 读 | 从机 MISO 恒定返回 16 字节应答 `30 31 32 ... 3F` |
| 单帧上限 | **512 字节** |
| 帧间间隔 | NSS 高电平建议 ≥ 50us |

---

## 2. 接线

### SPI 四线（必须全部接，且必须共地）

| STM32 (M100Z-M7) | 方向 | Master (RK3576 等) |
|---|---|---|
| `PB12` NSS | ← | CS / SS |
| `PB13` SCK | ← | SCLK |
| `PB14` MISO | → | MISO |
| `PB15` MOSI | ← | MOSI |
| `GND` | — | GND（**必接**） |

> **NSS 一定要接。** SPI 协议本身没有"帧"的概念，从机完全靠 NSS 上升沿判断一帧结束。
> 不接 NSS 就不会打印，数据也会粘连。

### 串口（看打印）

板上没有 USB 转串口芯片，`PA9(TX)/PA10(RX)` 引在 **P3 调试排针**（4x2，丝印 `SWD & USART1`）。

| STM32 | Mini HS-DAP / USB-TTL |
|---|---|
| `PA9` (TX) | RXD |
| `PA10` (RX) | TXD |
| `GND` | GND |

串口参数：**115200-8-N-1**，用 XCOM / SSCOM / MobaXterm 打开对应 COM 口。

---

## 3. 工作原理（V1.3 环形 DMA 常驻架构）

SPI 使能一次就**不再关闭**，收发 DMA 都配成 **CIRCULAR（环形）模式常驻运行**，
只用 `NSS(PB12)` 的上升沿中断来"切割"帧：

```
        ┌────────── 一直使能，永不停 ──────────┐
SPI2 ───┴──────────────────────────────────────┴───
   RX DMA (DMA1_Stream0, 环形512字节)  ← 连续写入环形缓冲
   TX DMA (DMA1_Stream1, 环形16字节)   → 应答pattern常驻FIFO

NSS 上升沿 ──► EXTI ──► 读DMA剩余计数NDTR
                       本帧长度 = (上次NDTR − 本次NDTR) & 511
                       本帧数据 = 环形缓冲对应区间（含回绕处理）
                       ──► Cache失效 ──► 拷到影子缓冲 ──► 置打印标志
主循环      ──► 检测标志 ──► 十六进制打印（不在中断里做慢速打印）
```

### 为什么这么设计

V1.0~V1.2 用的是"每帧停下 SPI/DMA → 处理 → 重新武装"的模型，在 STM32H7 上会踩三个坑：

| # | 坑 | 后果 |
|---|---|---|
| 1 | H7 的 SPI 从机必须"**先武装好，再来时钟**" | 每帧重新武装的窗口里如果 Master 已开始打时钟，**首字节丢失/错位** |
| 2 | H7 的 SPI **TX FIFO 会被 DMA 预填充**，而 `SPE=0` 并不保证清空 FIFO | 反复"停/启"会让 Master 读到**上一次残留在 FIFO 里的旧字节** |
| 3 | HAL 的 `HAL_SPI_TransmitReceive_DMA()` 会顺带开 OVR/UDR/FRE/MODF 中断并维护内部 State 机 | 一旦 DMA 报错就把流停掉、State 置 ERROR，之后数据全乱、重新武装还直接返回 `HAL_BUSY` |

V1.3 的环形常驻模型把这三个坑一次性绕开：**从机在任何时刻都是"武装好"的状态**，
不存在重新武装窗口，也不存在 FIFO 残留。

### 几个关键设计点

- **软件 NSS**：`SSI=0`，从机常选中（MISO 常驱动），只适用于**单从机**总线。
- **TX 缓冲长度 = 应答 pattern 长度 = 16 字节**。因为 DMA 环形长度正好等于 pattern 长度，
  FIFO 里预装的永远是 `30 31 ... 3F` 的连续片段，所以 Master 每次读 16 字节
  **必然**拿到完整的 `30 31 32 ... 3F`，不受帧起点漂移影响。
- **不启用任何 DMA 中断**：全片唯一中断源是 `NSS` 的 EXTI。
  另外 `HAL_DMA_Start_IT()` 会无条件打开 DMA 的 TC/TE/DME 中断，代码里显式关掉了。
- **关掉了 DMA 中断服务函数的内容**（`DMA1_Stream0/1_IRQHandler` 留空占位）。
  这样链接器能丢掉 `HAL_DMA_IRQHandler`/`HAL_DMA_Abort_IT` 约 **4KB** 代码 —— 
  见下面第 5 节的 32KB 限制说明。
- **Cache 一致性**：缓冲区 32 字节对齐、位于 AXI SRAM（`0x24000000`）。
  D-Cache 强制透写；RX 方向 DMA 写内存后 CPU 读前做 `SCB_InvalidateDCache_by_Addr`。
- **毛刺帧过滤**：Master 板上电/复位瞬间 SCK 抖动会被从机当成时钟、MOSI 悬空为高
  → 收到满屏 `FF`。本工程**静默丢弃全 0xFF 帧**不打印。
  （若业务上确实要传全 0xFF 的数据帧，删掉 `main.c` 中标注的那一段即可。）

---

## 4. 上电自检与诊断输出

### 4.1 信号线自检（每次上电打印一次）

保持 Master 上电，从机用**内部上拉/下拉对比**判断每条线是否真被外部驱动：

```
---- 信号线自检(Master保持上电, 用内部上下拉对比, 只作参考) ----
  SCK  (PB13): 外部驱动为低
  MOSI (PB15): 外部驱动为高
  MISO (PB14): 悬空(正常, 由STM32驱动)
  NSS  (PB12): 外部驱动为高(空闲)
```

出现 `悬空/未接 !!` → 这根线没接到 Master，先查线。

### 4.2 寄存器快照（每次上电打印一次）

```
---- 寄存器快照 ----
 SPI2  : CR1=... CFG1=... CFG2=... SR=...
 DMA_S0: CR=... NDTR=512 PAR=... M0AR=... (RX, 环形512字节)
 DMA_S1: CR=... NDTR=16  PAR=... M0AR=... (TX, 环形16字节)
```

`CR` 的 bit10(`MINC`) 与 bit8(`CIRC`) 应都为 1；`NDTR` 应等于缓冲大小。

### 4.3 运行中诊断（每 3 秒一次，只在有变化时打印）

```
[诊断] NSS边沿=12  毛刺帧丢弃=0  SPI错误=0  FIFO排空超时=0
```

| 字段 | 含义 | 正常值 |
|---|---|---|
| `NSS边沿` | 收到多少次 NSS 上升沿 | 与 Master 发起次数一致 |
| `毛刺帧丢弃` | 被过滤掉的全 0xFF 假帧数 | Master 上电时会涨几次，之后不动 |
| `SPI错误` | OVR/UDR/FRE/MODF 次数 | **恒为 0** |
| `FIFO排空超时` | RX FIFO 迟迟不空（DMA 可能卡住） | **恒为 0** |

---

## 5. ⚠ Keil 版本限制：32KB

本工程用 **armcc V5.06u7 (AC5)** 编译，镜像大小约 **30568 字节**
（`Code=29320 RO=1192 RW=56`，另有 `ZI=5464` 不占镜像）。

如果 Keil 是 **Lite / 未注册版**，链接器有 **32KB 上限**，超了会报：

```
L6047U: The size of this image (xxxxx bytes) exceeds the maximum allowed for this version of the linker
```

当前余量约 2.2KB。若后续继续加代码触发此错误，说明确实顶到 Lite 上限了 ——
要么注册 MDK，要么换 AC6/GCC 编译。

---

## 6. 编译

安装有 Keil MDK + ARM Compiler 5 的情况下，命令行编译：

```bat
"C:\Keil_v5\UV4\UV4.exe" -b Projects\MDK-ARM\atk_h750.uvprojx -j0 -o build_log.txt
```

或在 Keil 里打开 `Projects/MDK-ARM/atk_h750.uvprojx` 直接 Build。

**编译结果**：0 Error / 0 Warning，`Code=29320 RO=1192 RW=56 ZI=5464`。

---

## 7. 版本记录

| 版本 | 变更 |
|------|------|
| V1.0 | 初版：SPI2 Slave + 软件 NSS + EXTI 帧边界 + DMA 收发 |
| V1.1 | 增加**全 0xFF 毛刺帧过滤**（解决 Master 板上电瞬间疯狂打印假帧） |
| V1.2 | 修复帧长累加 Bug：`HAL_SPI_DMAStop()` 为空壳函数导致 HAL 状态机卡在 BUSY、DMA 计数不重装 |
| V1.3 | **改为环形 DMA 常驻架构**：SPI 不再反复关停，彻底解决"首字节错位/丢字节"和"TX FIFO 残留导致 Master 读到旧字节"；不再使用 HAL 的 SPI DMA 状态机与 DMA 中断；新增信号线自检 + 寄存器快照 + 运行诊断计数；空壳化 DMA 中断函数释放约 4KB 以适配 MDK-Lite 32KB 上限 |
