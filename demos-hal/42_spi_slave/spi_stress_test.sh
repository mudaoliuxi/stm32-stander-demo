#!/bin/sh
# ============================================================
# SPI 回环压力测试 (RK3576 -> STM32H750 SPI2 从机 V2.7)
# 用法: ./spi_stress_test.sh [速率Hz] [轮数] [每帧字节数]
#   例: ./spi_stress_test.sh 1000000 512 16
# 默认: 1MHz, 256 轮, 每帧 16 字节 (上限 32, 受从机 TX 缓冲限制)
#
# 协议 (V2.7 取反回环):
#   从机第 N 帧回发 = 第 N-1 帧收到数据的按位取反; 上电默认回发 16 个 A5。
#   因此每一轮: 期望RX = 上一轮TX的逐字节取反 (第 1 轮期望全 A5)。
#   任何一轮对不上 => 从机收错或发错, 立即打印详情并退出。
#
# 退出码: 0=全部通过  1=失败  130=手动中断
# ============================================================

DEV=${DEV:-/dev/spidev2.0}
SPEED=${1:-1000000}
ROUNDS=${2:-256}
LEN=${3:-16}

if [ "$LEN" -gt 32 ]; then
    echo "LEN 上限 32 (从机 TX 缓冲 32 字节)"
    exit 1
fi
if [ "$LEN" -ne 16 ]; then
    echo "[!] LEN=$LEN: 第 1 轮从机固定回发 16 字节 A5, LEN≠16 时第 1 轮会假失败, 建议用 16"
fi

echo "设备=$DEV 速率=$SPEED Hz 轮数=$ROUNDS 每帧=$LEN 字节"
echo "协议: 期望RX = 上一轮TX的按位取反 (第1轮期望全A5)"
echo "--------------------------------------------"

ok=0
total_bytes=0
start=$(date +%s)

# --- 生成 \xAA 形式的 TX 串 (字面量, spidev_test 自己解析 \x) ---
gen_tx() {
    st=$(( ($1 % 256 + 256) % 256 ))    # 防御: 归一化到 0..255, 禁止负数进入 printf
    i=0
    s=""
    while [ $i -lt $LEN ]; do
        s="$s\\x$(printf '%02x' $(( (st + i) % 256 )))"
        i=$((i + 1))
    done
    printf '%s' "$s"
}

# --- 生成期望 RX: 上一轮 TX 的逐字节取反, 空格分隔大写 hex ---
gen_exp_not() {
    st=$(( ($1 % 256 + 256) % 256 ))    # 同上, 负数/超界一律归一化
    i=0
    s=""
    while [ $i -lt $LEN ]; do
        s="$s$(printf '%02X ' $(( 255 - (st + i) % 256 )))"
        i=$((i + 1))
    done
    printf '%s' "$s"
}

# --- 生成全 A5 期望 (第 1 轮) ---
gen_exp_a5() {
    i=0
    s=""
    while [ $i -lt $LEN ]; do
        s="${s}A5 "
        i=$((i + 1))
    done
    printf '%s' "$s"
}

trap 'echo; echo "手动中断: 完成 $ok 轮 ($total_bytes 字节) 无错"; exit 130' INT TERM

r=0
last_st=-1
while [ $r -lt $ROUNDS ]; do
    r=$((r + 1))

    # 期望 = 上一轮 TX 的取反; 第 1 轮 = 全 A5 (从机上电默认)
    if [ "$last_st" -lt 0 ]; then
        exp=$(gen_exp_a5)
    else
        exp=$(gen_exp_not $last_st)
    fi

    # 本轮 TX: 每 4 轮换 pattern (全A5/全5A/滚动递增/反向递增)
    # [!] 全部保持十进制: bash 的 [ 不认 0xA5 这种 hex 字符串(-lt 报
    #     "integer expression expected"); 取模必须归一化成 0..255,
    #     否则 (255-r)%256 在 r>255 时得负数, printf '%02x' 负数会输出
    #     16 个 hex 字符, spidev_test 把剩余字符当 ASCII 发出 => 帧长暴涨+数据变文本
    case $((r % 4)) in
        0) st=$(( r % 256 ));;
        1) st=$((0xA5));;
        2) st=$((0x5A));;
        3) st=$(( ((255 - r) % 256 + 256) % 256 ));;
    esac
    tx=$(gen_tx $st)

    out=$(spidev_test -D "$DEV" -s "$SPEED" -v -p "$tx" 2>&1)
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "============================================"
        echo "第 $r 轮: spidev_test 异常退出 (码 $rc):"
        echo "$out"
        echo "已完成 $ok 轮 ($total_bytes 字节) 无错"
        exit 1
    fi

    # 从 "RX | AA BB ... __ __ |..." 行提取有效字节(遇到 __ 停)
    rx=$(printf '%s\n' "$out" | awk '/^RX \|/ {for(i=3;i<=NF;i++){if($i ~ /^__/) break; printf "%s ", $i} exit}')

    if [ "$rx" != "$exp" ]; then
        echo "============================================"
        echo "第 $r 轮失败! (速率 $SPEED Hz, 本轮TX起点 0x$(printf '%02X' $st), 期望校验的是上一轮TX)"
        echo "期望: $exp"
        echo "实际: $rx"
        echo "--- spidev_test 原始输出 ---"
        echo "$out"
        echo "--------------------------------------------"
        echo "已完成 $ok 轮 ($total_bytes 字节) 无错"
        exit 1
    fi

    ok=$((ok + 1))
    total_bytes=$((total_bytes + LEN))
    last_st=$st

    if [ $((ok % 25)) -eq 0 ]; then
        now=$(date +%s)
        echo "进度: $ok/$ROUNDS 轮 ($total_bytes 字节) 无错  [已跑 $((now - start)) s]"
    fi
done

end=$(date +%s)
echo "============================================"
echo "全部通过: $ok 轮 / $total_bytes 字节, 耗时 $((end - start)) s, 速率 $SPEED Hz"
echo "回环校验说明: 每轮都验证了『从机上一帧收到的数据』, 收发双向闭环"
exit 0
