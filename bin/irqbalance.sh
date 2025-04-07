#!/bin/bash
## 网卡名称
#ifname=$1
#
## 获取CPU核心数
#cpu_logic_core_count=`cat /proc/cpuinfo |grep "processor"|wc -l`
#
## 停止irqbalance服务
#service irqbalance stop &> /dev/zero
#
## 获取网卡最大队列数
#max_queues=`ethtool -l $ifname 2> /dev/zero | grep "Combined" | head -n 1 | cut -d: -f2 | sed 's, *,,'`
#if [ -z "$max_queues" ]; then
#    max_queues=1
#fi
#
## 计算激活网卡队列数 min(CPU核心数，网卡最大队列数)
#num_queues=$cpu_logic_core_count
#if [ $cpu_logic_core_count -gt $max_queues ]; then
#    num_queues=$max_queues
#fi
#
#ifconfig $ifname txqueuelen 10000
#ifconfig $ifname
#
## 设置网卡激活队列数
#ethtool -L $ifname combined $num_queues &> /dev/zero
#ethtool -l $ifname
#
## 绑定网卡中断到CPU核心
#irq=`grep -m 1 $ifname'-TxRx-' /proc/interrupts | cut -d: -f1 | sed 's, *,,'`
#if [ -n "$irq" ]; then
#    for((i=0;i<num_queues;++i))
#    do  
#        x=$[irq+i]
#        `echo $[i+1] > /proc/irq/$x/smp_affinity`
#        echo "irq" $x "->" "cpu" `cat /proc/irq/$x/smp_affinity`
#    done
#fi

# 网卡名称
ifname=eth0

# 获取网卡最大ring数量,该值越大，latency越大，throughput越大，丢包越少，反之亦然 
max_rx_ring=`ethtool -g $ifname 2> /dev/zero | grep "RX" | head -n 1 | cut -d: -f2 | sed 's, *,,'`
max_tx_ring=`ethtool -g $ifname 2> /dev/zero | grep "TX" | head -n 1 | cut -d: -f2 | sed 's, *,,'`

# 限制ring数量最大值为4096
limit_ring=4096
num_rx_ring=$max_rx_ring
num_tx_ring=$max_tx_ring
if [ $num_rx_ring -gt $limit_ring ]; then
    num_rx_ring=$limit_ring
fi
if [ $num_tx_ring -gt $limit_ring ]; then
    num_tx_ring=$limit_ring
fi

# 设置网卡激活的ring数量
ethtool -G $ifname rx $num_rx_ring 1> /dev/zero 2> /dev/zero
ethtool -G $ifname tx $num_tx_ring 1> /dev/zero 2> /dev/zero
ethtool -g $ifname

# 设置tcp/ip协议栈参数
echo "Sysctl parameters:"
sysctl net.core.somaxconn="4096"
sysctl net.core.netdev_max_backlog="4096"
sysctl net.ipv4.tcp_max_syn_backlog="4096"
sysctl net.ipv4.tcp_syncookies="1"
sysctl net.ipv4.tcp_reordering="6"
sysctl net.ipv4.tcp_congestion_control="bbr"
sysctl net.ipv4.tcp_keepalive_time="300"
sysctl net.ipv4.tcp_keepalive_intvl="30"
sysctl net.ipv4.tcp_keepalive_probes="3"
sysctl net.ipv4.tcp_tw_reuse="1"
sysctl net.ipv4.tcp_fin_timeout="30"
sysctl net.ipv4.ip_local_port_range="1024 65000"
sysctl net.ipv4.tcp_syn_retries="2"
sysctl net.ipv4.tcp_synack_retries="2"
sysctl net.ipv4.tcp_retries1="3"
sysctl net.ipv4.tcp_retries2="5"
