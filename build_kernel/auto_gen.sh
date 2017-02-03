export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
#ARCH=arm64 scripts/kconfig/merge_config.sh -m arch/arm64/configs/defconfig android/configs/android-base.cfg android/configs/android-recommended.cfg
make android_defconfig && make -j16 Image.gz
make hisilicon/hi3660-hikey960.dtb
./mkbootimg  --kernel arch/arm64/boot/Image.gz --ramdisk ramdisk.img --cmdline "root=/dev/ram rdinit=/linuxrc loglevel=15 androidboot.hardware=hikey960 androidboot.selinux=permissive firmware_class.path=/system/etc/firmware" --base 0x0 --tags-addr 0x07A00000 --kernel_offset 0x00080000 --ramdisk_offset 0x07c00000 --os_version 7.0 --os_patch_level 2016-08-05  --output boot.img
./dtbTool -o dt.img -s 2048 -p scripts/dtc/ arch/arm64/boot/dts/hisilicon/
