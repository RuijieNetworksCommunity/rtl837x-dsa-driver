# SPDX-License-Identifier: GPL-2.0-or-later

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtl8372n_dsa
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=StarField Xu (air_jinkela@163.com)

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
  SUBMENU:=Network Devices
  TITLE:=Realtek RTL8372N DSA switch driver
  FILES:=$(PKG_BUILD_DIR)/rtl8372n_dsa.ko
  AUTOLOAD:=$(call AutoLoad,42,rtl8372n_dsa)
  KCONFIG:= \
    CONFIG_NET_DSA_TAG_RTL8_4=y \
    CONFIG_NET_DSA_TAG_MXL_862XX_8021Q=y
endef

define Build/Compile
	+$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(PKG_BUILD_DIR)" \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
		CONFIG_RTL8372N_DSA=m \
		CONFIG_RTL837X_PHY_PATCH=y \
		CONFIG_RTL8372N_DSA_DEBUG=n \
		modules
endef

$(eval $(call KernelPackage,rtl8372n_dsa))
