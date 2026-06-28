#ifndef __RTL8372_COMMON_H__
#define __RTL8372_COMMON_H__

#include <linux/of_mdio.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/dsa/8021q.h>
#include <net/dsa.h>

#include "./rtk-api/dal/rtl8373/rtl8373_asicdrv.h"
#include "./rtk-api/dal/rtl8373/dal_rtl8373_mapper.h"
#include "./rtk-api/dal/rtl8373/dal_rtl8373_switch.h"

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
	struct mutex		map_lock;
	struct regmap		*map_8224;
	struct mutex		map_8224_lock;
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

struct rtl837x_vlan_data {
	u16 vid;
	union {
		struct {
			u32 mbr  : 10;
			u32 untag: 10;
			u32 fid  : 4 ;
			u32 svlan_chk_ivl_svl: 1;
			u32 ivl_svl: 1;
			u32 resv : 6;
		};
		u32 val;
	};
};

struct rtl837x_variant {
	const struct dsa_switch_ops *ds_ops_mdio;
	const struct rtl837x_ops *ops;
	enum dsa_tag_protocol def_tag_proto;
	const struct phylink_mac_ops *pl_mac_ops;
	const bool have_8224;
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
	
	int	(*phy_read_c45)(struct rtl837x_priv *priv, int phy, int devad, int regnum,
				u16 *pval);
	int	(*phy_write_c45)(struct rtl837x_priv *priv, int phy, int devad, int regnum,
				u16 val);
};

char* chipid_to_chip_name(switch_chip_t id);

extern rtk_api_ret_t rtk_hal_init(void);

#define rtl837x_reg_read(priv, reg, pval) regmap_read(priv->map, reg, pval)
#define rtl837x_reg_write(priv, reg, val) regmap_write(priv->map, reg, val)

extern int rtl837x_reg_bits_read(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 *pval);
extern int rtl837x_reg_bits_write(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 val);

extern int rtl837x_gpiochip_init(struct rtl837x_priv *priv);
extern rtk_sds_mode_t phy_interface_to_rtk_sds_mode(phy_interface_t interface);

extern int rtl837x_debug_proc_init(struct rtl837x_priv *priv);
extern int rtl837x_debug_proc_deinit(struct rtl837x_priv *priv);

extern int rtl837x_phy_read_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 *pval);
extern int rtl837x_phys_write_c45(struct rtl837x_priv *priv, u16 phy_mask, int devad, int regnum, u16 val);
extern int rtl837x_phy_write_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 val);

extern int rtl837x_sds_reg_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 *pdata);
extern int rtl837x_sds_reg_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 data);
extern int rtl837x_sds_reg_bits_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 data);
extern int rtl837x_sds_reg_bits_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 *pdata);

#define rtl837x_rtl8224_reg_read(priv, reg, pval) (priv->map_8224 ? regmap_read(priv->map_8224, reg, pval) : -ENODEV)
#define rtl837x_rtl8224_reg_write(priv, reg, val) (priv->map_8224 ? regmap_write(priv->map_8224, reg, val) : -ENODEV)
extern int rtl837x_rtl8224_reg_bits_read(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 *pval);
extern int rtl837x_rlt8224_reg_bits_write(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 val);
extern int rtl837x_rtl8224_sds_reg_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 *pdata);
extern int rtl837x_rtl8224_sds_reg_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 data);

extern int rtl837x_vlan_set(struct rtl837x_priv *priv, struct rtl837x_vlan_data *vlan);
extern int rtl837x_vlan_get(struct rtl837x_priv *priv, struct rtl837x_vlan_data *vlan);

extern const struct rtl837x_variant rtl8372n_variant;

#endif