#!/bin/bash

# Partition to spill the idle pages from zram.
PARTITION=/dev/disk/by-id/nvme-Samsung_SSD_980_PRO_*-part2

modprobe zram
echo 1 >/sys/block/zram0/reset
echo $PARTITION >/sys/block/zram0/backing_dev

# Maximum rate to write the data to the backing partition.
echo 72000000 >/sys/block/zram0/writeback_limit

# Compression algorithm, fastest.
echo lz4 >/sys/block/zram0/comp_algorithm

# Maximum amount of space for data stored in zram before compression.
echo 64G >/sys/block/zram0/disksize

# Limit maximum usage of ram. We would try to spill to disk way before we hit this.
echo 16G  >/sys/block/zram0/mem_limit
mkswap /dev/zram0
swapon -p 10 /dev/zram0

# Compression is pretty cheap. This can free some space for ZFS ARC.
echo 100 >/proc/sys/vm/swappiness

# Invoke tracker, which will start spilling to backing store if more than 1/4 of zram memory used.
zramtracker
