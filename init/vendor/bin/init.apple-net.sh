#!/vendor/bin/sh

OTP_CHIP=`cat /sys/module/brcmfmac/parameters/otp_chip_id`
OTP_NVRAM=`cat /sys/module/brcmfmac/parameters/otp_nvram_id`
CHIP=`cat /sys/module/brcmfmac/parameters/otp_chip_id | tr '_' '\n' | grep '^C-' | cut -c3-`

MODULE='unknown'
BTMODULE=''
if [ -d /sys/bus/pci/devices/0000:00:02.0 ]; then
	if [ -d /sys/bus/pci/devices/0000:00:02.0/0000:* ]; then
		if [ `cat /sys/bus/pci/devices/0000:00:02.0/0000:*/vendor` == '0x14e4' ]; then
			if [ -f /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/elsa.trx ]; then
				MODULE='elsa'
			fi
			if [ -f /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/olaf.trx ]; then
				MODULE='olaf'
			fi
		fi
	fi
fi
if [ -d /sys/bus/pci/devices/0000:00:03.0 ]; then
	if [ -d /sys/bus/pci/devices/0000:00:03.0/0000:* ]; then
		if [ `cat /sys/bus/pci/devices/0000:00:03.0/0000:*/vendor` == '0x14e4' ]; then
			if [ -f /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/kristoff.trx ]; then
				MODULE='kristoff'
			fi
			if [ -f /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/sven.trx ]; then
				MODULE='sven'
			fi
			if [ -f /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/baccus-2.trx ]; then
				MODULE='baccus-2'
				BTMODULE='baccus2'
			fi
		fi
	fi
fi

if [ $MODULE == 'unknown' ]; then
	echo WLAN PCI device not found.
	exit 1
fi
if [ x$BTMODULE == x ]; then
	BTMODULE=$MODULE
fi

mkdir -p /data/vendor/firmware/brcm
ln -sf /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/${MODULE}.trx /data/vendor/firmware/brcm/brcmfmac${CHIP}-pcie.bin
ln -sf /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/${MODULE}.clmb /data/vendor/firmware/brcm/brcmfmac${CHIP}-pcie.clm_blob
ln -sf /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/${MODULE}.txcb /data/vendor/firmware/brcm/brcmfmac${CHIP}-pcie.txcb
ln -sf /hostfs/usr/share/firmware/wifi/${OTP_CHIP}/P-${MODULE}_${OTP_NVRAM}.txt /data/vendor/firmware/brcm/brcmfmac${CHIP}-pcie.hx,h9p.txt

BTFW_NAME=`/vendor/bin/hcdpack /hostfs/usr/sbin/BlueTool ${OTP_CHIP} ${BTMODULE} ${OTP_NVRAM} /data/vendor/firmware/brcm/BCM.hcd`
echo serial0-0 > /sys/bus/serial/drivers/hci_uart_bcm/unbind
if [ -f /data/vendor/firmware/brcm/BCM.hcd ]; then
    mv /data/vendor/firmware/brcm/BCM.hcd /data/vendor/firmware/brcm/${BTFW_NAME}
    /vendor/bin/syscfg /dev/block/nvme0n3 BMac x:6 > /sys/module/btbcm/parameters/mac_addr
    echo -n /data/vendor/firmware/ > /sys/module/btbcm/parameters/alternative_fw_path
fi

/vendor/bin/syscfg /dev/block/nvme0n3 WMac x:6 > /sys/module/brcmfmac/parameters/nvram_mac_addr
echo -n /data/vendor/firmware > /sys/module/brcmfmac/parameters/alternative_fw_path

for RETRY in `seq 1 3`; do
    echo 1 > /sys/bus/pci/devices/0000:02:00.0/remove
    sleep 1
    echo 1 > /sys/bus/pci/devices/0000:00:02.0/dev_rescan
    echo 1 > /sys/bus/pci/devices/0000:00:03.0/dev_rescan
    for DELAY in `seq 1 6`; do
        if [ -e /sys/class/net/wlan0 ]; then
            ifconfig wlan0 up
            if [ -f /data/vendor/firmware/brcm/${BTFW_NAME} ]; then
                echo serial0-0 > /sys/bus/serial/drivers/hci_uart_bcm/bind
            fi
            exit 0
        fi
        sleep 1
    done
done

echo WLAN start up failed.
