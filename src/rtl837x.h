#ifndef __RTL8372_COMMON_H__
#define __RTL8372_COMMON_H__

#include <linux/switch.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <net/dsa.h>

#include <rtl8373_asicdrv.h>
#include "./rtk-api/dal/rtl8373/rtl8373_smi.h"
#include "./rtk-api/dal/rtl8373/dal_rtl8373_drv.h"
#include "./rtk-api/rtk_error.h"
#include "./rtk-api/rtk_types.h"
#include "./rtk-api/rtk_switch.h"
#include "./rtk-api/phy.h"
#include "./rtk-api/port.h"
#include "./rtk-api/vlan.h"
#include "./rtk-api/chip.h"
#include "./rtk-api/eee.h"
#include "./rtk-api/rma.h"
#include "./rtk-api/cpuTag.h"
#include "./rtk-api/mib.h"
#include "./rtk-api/isolation.h"

#include <linux/printk.h>

struct rtl837x_mib_counter {
	uint16_t	base;
	const char	*name;
};

struct rtl837x_sdsmode_map {
	rtk_sds_mode_t mode;
	const char *name;
};

struct rtl837x_priv {
 	struct device *dev;
	struct gpio_desc	*reset;
 	struct mii_bus *bus;
	struct regmap		*map;
	struct regmap		*map_nolock;
	struct mutex		map_lock;
	int			mdio_addr;

	const char *chip_name;
	switch_chip_t chip_id;
	const uint8_t *port_map;

	unsigned int cpu_port;
	unsigned int num_ports;

	rtk_sds_mode_t sds0mode;
	rtk_sds_mode_t sds1mode;

	struct dsa_switch	*ds;

    struct rtl837x_mib_counter *mib_counters;
	unsigned int num_mib_counters;

	const struct rtl837x_ops *ops;
	int			(*write_reg_noack)(void *ctx, u32 addr, u32 data);

	int			vlan_enabled;
	int			vlan4k_enabled;

	char			buf[4096];
	void			*chip_data; /* Per-chip extra variant data */
};

struct rtl837x_vlan_4k {
	u16	vid;
	u16	untag;
	u16	member;
	u8	fid;
};

struct rtl837x_variant {
	const struct dsa_switch_ops *ds_ops_mdio;
	const struct rtl837x_ops *ops;
	size_t chip_data_sz;
};

struct rtl837x_ops {
	int	(*detect)(struct rtl837x_priv *priv);
	int	(*reset_chip)(struct rtl837x_priv *priv);

	int	(*get_mib_counter)(struct rtl837x_priv *priv,
					int port,
					struct rtl837x_mib_counter *mib,
					u64 *mibvalue);

	int	(*get_vlan_4k)(struct rtl837x_priv *priv, u32 vid,
			       struct rtl837x_vlan_4k *vlan4k);
	int	(*set_vlan_4k)(struct rtl837x_priv *priv,
			       const struct rtl837x_vlan_4k *vlan4k);
	bool	(*is_vlan_valid)(struct rtl837x_priv *priv, unsigned int vlan);
	int	(*enable_vlan)(struct rtl837x_priv *priv, bool enable);
	
	int	(*phy_read_c45)(struct rtl837x_priv *priv, int phy, int devad, int regnum);
	int	(*phy_write_c45)(struct rtl837x_priv *priv, int phy, int devad, int regnum,
				u16 val);
};

char* chipid_to_chip_name(switch_chip_t id);

extern const struct rtl837x_variant rtl8372n_variant;
// extern int rtl837x_phy_module_init(struct module *owner, const void *driver_data);
// extern void rtl837x_phy_module_exit(void);

#endif