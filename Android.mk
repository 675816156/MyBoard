#
# Copyright (C) 2009-2011 The Android-x86 Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
KERNEL_DEFCONFIG := defconfig
KERNEL_DIR := $(call my-dir)
ROOTDIR := $(abspath $(TOP))
KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/arch/arm64/boot/Image.gz
TARGET_PREBUILT_DTB := $(KERNEL_OUT)/arch/arm64/boot/dts/hisilicon/hi3660-hikey960.dtb
TARGET_KERNEL_CONFIG := $(KERNEL_OUT)/.config
KERNEL_HEADERS_INSTALL := $(KERNEL_OUT)/usr
INSTALLED_KERNEL_TARGET := $(PRODUCT_OUT)/kernel
INSTALLED_DTB_TARGET := $(PRODUCT_OUT)/hi3660-hikey960.dtb
KERNEL_CROSS_COMPILE := ${ANDROID_TOOLCHAIN}/aarch64-linux-android-
$(KERNEL_OUT):
	mkdir -p $@
.PHONY: kernel kernel-defconfig kernel-menuconfig clean-kernel
kernel-menuconfig: | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) menuconfig
kernel-savedefconfig: | $(KERNEL_OUT)
	cp $(TARGET_KERNEL_CONFIG) $(KERNEL_DIR)/arch/arm64/configs/$(KERNEL_DEFCONFIG)
$(TARGET_PREBUILT_KERNEL): kernel
$(TARGET_KERNEL_CONFIG) kernel-defconfig: $(KERNEL_DIR)/arch/arm64/configs/$(KERNEL_DEFCONFIG) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) $(KERNEL_DEFCONFIG)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) oldconfig
$(KERNEL_HEADERS_INSTALL): $(TARGET_KERNEL_CONFIG) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) headers_install
kernel: $(TARGET_KERNEL_CONFIG) $(KERNEL_HEADERS_INSTALL) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE)
$(TARGET_PREBUILT_DTB): hi3660-hikey960.dtb
hi3660-hikey960.dtb:
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm64 CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) hisilicon/$@
$(INSTALLED_KERNEL_TARGET): kernel
$(INSTALLED_KERNEL_TARGET): $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(call copy-file-to-target)
$(INSTALLED_DTB_TARGET): hi3660-hikey960.dtb
$(INSTALLED_DTB_TARGET): $(TARGET_PREBUILT_DTB) | $(ACP)
	$(call copy-file-to-target)
clean-kernel:
	@rm -rf $(KERNEL_OUT)
