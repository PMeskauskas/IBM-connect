include $(TOPDIR)/rules.mk

PKG_NAME:=IBM_connect
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/IBM_connect
	CATEGORY:=Base system
	TITLE:=IBM_connect
	DEPENDS:=+libIBMWatson +libubox +libblobmsg-json +libubus +luci-app-IBM_connect
endef

define Package/IBM_connect/description
	This is an example app package which uses an example library package
endef

define Package/IBM_connect/install
	$(INSTALL_DIR) $(1)/usr/bin  
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/IBM_connect $(1)/usr/bin
	$(INSTALL_BIN) ./files/IBM_device.init $(1)/etc/init.d/IBM_device
	$(INSTALL_CONF) ./files/IBM_device.config $(1)/etc/config/IBM_device
endef

$(eval $(call BuildPackage,IBM_connect))
