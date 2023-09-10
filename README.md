Basic c++ binary, "script" for spilling old zram block into backing device.

setup.sh: sets up the zram device and backing store.

zramtracker: is small c "script" which tracks usage and start spilling old blocks to backing store when needed. I chose c instead of bash as during memory pressure I would like the script to have as little footprint as possible to guarantee that is remains functional. It also locks it memory in ram using mlockall() call.

zraminfo: is basic status script to understand the current situation with zram.
