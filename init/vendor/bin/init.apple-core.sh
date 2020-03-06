#!/vendor/bin/sh

# Unbind fbcon
echo 0 > /sys/devices/virtual/vtconsole/vtcon1/bind

#init cannot access ro.kernel.android.bootanim,
#so do a translation into vendor.qemu space
bootanim=`getprop ro.kernel.android.bootanim`
case "$bootanim" in
    "")
    ;;
    *) setprop vendor.qemu.android.bootanim 0
    ;;
esac

# take the wake lock
allowsuspend=`getprop ro.kernel.qemu.allowsuspend`
case "$allowsuspend" in
    "") echo "emulator_wake_lock" > /sys/power/wake_lock
    ;;
    1) echo "emulator_wake_lock" > /sys/power/wake_unlock
    ;;
    *) echo "emulator_wake_lock" > /sys/power/wake_lock
    ;;
esac

setprop ro.serialno `/vendor/bin/syscfg /dev/block/nvme0n3 SrNm z`
setprop ro.boot.hardware.revision `syscfg /dev/block/nvme0n3 'Mod#' z`
