#ifndef __RTL8372_COMMON_H__
#define __RTL8372_COMMON_H__

#include <linux/switch.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <net/dsa.h>
#include <linux/dsa/8021q.h>

#include "./rtk-api/rtk_error.h"
#include "./rtk-api/rtk_types.h"
#include "./rtk-api/rtk_switch.h"
#include "./rtk-api/phy.h"
#include "./rtk-api/port.h"
#include "./rtk-api/vlan.h"
#include "./rtk-api/svlan.h"
#include "./rtk-api/chip.h"
#include "./rtk-api/eee.h"
#include "./rtk-api/rma.h"
#include "./rtk-api/cpuTag.h"
#include "./rtk-api/mib.h"
#include "./rtk-api/isolation.h"
#include "./rtk-api/l2.h"
#include "./rtk-api/mirror.h"
#include "./rtk-api/dal/rtl8373/rtl8373_asicdrv.h"
#include "./rtk-api/dal/rtl8373/dal_rtl8373_mapper.h"
#include "./rtk-api/dal/rtl8373/dal_rtl8373_switch.h"

#include <linux/printk.h>

#define MDC_MDIO_CTRL_REG           21
#define MDC_MDIO_ADDR_REG           22
#define MDC_MDIO_DATA_LOW           23
#define MDC_MDIO_DATA_HIGH          24
#define MDC_MDIO_READ_CMD           0x1B
#define MDC_MDIO_WRITE_CMD          0x19

struct rtl837x_mib_counter {
	unsigned int	offset;
	unsigned int	length;
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
	enum dsa_tag_protocol tag_proto;

	struct dentry *debugfs_parent;

	const char *chip_name;
	switch_chip_t chip_id;

	unsigned int num_ports;

	dal_mapper_t *pMapper;

	struct dsa_switch	*ds;

    struct rtl837x_mib_counter *mib_counters;
	unsigned int num_mib_counters;

	const struct rtl837x_ops *ops;
	int			(*write_reg_noack)(void *ctx, u32 addr, u32 data);

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
	enum dsa_tag_protocol def_tag_proto;
	const struct phylink_mac_ops *pl_mac_ops;
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

extern rtk_api_ret_t rtk_hal_init(void);
extern int rtl837x_gpiochip_init(struct rtl837x_priv *priv);
extern rtk_sds_mode_t phy_interface_to_rtk_sds_mode(phy_interface_t interface);

extern int rtl837x_debug_proc_init(struct rtl837x_priv *priv);
extern int rtl837x_debug_proc_deinit(struct rtl837x_priv *priv);

extern int rtl837x_phy_read_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum);
extern int rtl837x_phys_write_c45(struct rtl837x_priv *priv, u16 phy_mask, int devad, int regnum, u16 val);
extern int rtl837x_phy_write_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 val);

extern int rtl837x_sds_reg_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 *pdata);
extern int rtl837x_sds_reg_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 data);
extern int rtl837x_sds_reg_bits_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 data);
extern int rtl837x_sds_reg_bits_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 *pdata);

extern const struct rtl837x_variant rtl8372n_variant;

#endif