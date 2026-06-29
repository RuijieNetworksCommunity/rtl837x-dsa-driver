#include <linux/bitops.h>
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>

#include "./rtl837x.h"
#include "./rtk-api/dal/dal_mgmt.h"
#include "./rtk-api/identify.h"

char* chipid_to_chip_name(switch_chip_t id)
{
    switch (id)
    {
    case CHIP_RTL8373:
        return "RTL8373";
    case CHIP_RTL8372:
        return "RTL8372";
    case CHIP_RTL8224:
        return "RTL8224";
    case CHIP_RTL8373N:
        return "RTL8373N";
    case CHIP_RTL8372N:
        return "RTL8372N";
    case CHIP_RTL8224N:
        return "RTL8224N";
    case CHIP_RTL8366U:
        return "RTL8366U";
    default:
        return "Unknow";
    }
}

rtk_sds_mode_t phy_interface_to_rtk_sds_mode(phy_interface_t interface)
{
	switch (interface)
	{
	case PHY_INTERFACE_MODE_USXGMII:
		return SERDES_10GUSXG;
	case PHY_INTERFACE_MODE_1000BASEX:
		return SERDES_1000BASEX;
	case PHY_INTERFACE_MODE_SGMII:
		return SERDES_SG;
	case PHY_INTERFACE_MODE_2500BASEX:
		return SERDES_2500BASEX;
	case PHY_INTERFACE_MODE_10GBASER:
	case PHY_INTERFACE_MODE_10GKR:
	default:
		return SERDES_10GR;
	}
}

int rtl837x_reg_bits_read(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 *pval)
{
	int ret;
	u32 tmp;

    ret = rtl837x_reg_read(priv, reg, &tmp);
	if (ret)
		return ret;

    *pval = (tmp & mask) >> __ffs(mask);
	return 0;
}

int rtl837x_reg_bits_write(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 val)
{
    return regmap_update_bits(priv->map, reg, mask, (val << __ffs(mask)) & mask);
}

int rtl837x_phy_read_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 *pval)
{
	int ret;
    u32 tmp;

	ret = regmap_update_bits(priv->map,
			  RTL8373_SMI_ACCESS_PHY_CTRL_3_ADDR,
			  RTL8373_SMI_ACCESS_PHY_CTRL_3_INDATA_15_0_MASK,
			  FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_3_INDATA_15_0_MASK, phy));
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_MMD_DEVAD_4_0_MASK, devad) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_MMD_REG_15_0_MASK, regnum) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_RWOP_MASK, 0) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_TYPE_MASK, 1) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_CMD_MASK, 1);

	ret = regmap_write(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_1_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_1_ADDR, tmp, 
		((tmp & (RTL8373_SMI_ACCESS_PHY_CTRL_1_CMD_MASK | RTL8373_SMI_ACCESS_PHY_CTRL_1_FAIL_MASK))==0),
		0, 1000);
	if (ret)
		return ret;

	ret = regmap_read(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_2_ADDR, &tmp);
	if (ret)
		return ret;

	*pval = (tmp & RTL8373_SMI_ACCESS_PHY_CTRL_2_DATA_15_0_MASK) >> __ffs(RTL8373_SMI_ACCESS_PHY_CTRL_2_DATA_15_0_MASK); 
	return 0;
}

int rtl837x_phys_write_c45(struct rtl837x_priv *priv, u16 phy_mask, int devad, int regnum, u16 val)
{
	int ret;
    u32 tmp;

	ret = regmap_write(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_0_ADDR, phy_mask);
	if (ret)
		return ret;

	ret = regmap_update_bits(priv->map, 
			  RTL8373_SMI_ACCESS_PHY_CTRL_3_ADDR,
			  RTL8373_SMI_ACCESS_PHY_CTRL_3_INDATA_15_0_MASK,
			  FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_3_INDATA_15_0_MASK, val));
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_MMD_DEVAD_4_0_MASK, devad) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_MMD_REG_15_0_MASK, regnum) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_RWOP_MASK, 1) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_TYPE_MASK, 1) |
		FIELD_PREP(RTL8373_SMI_ACCESS_PHY_CTRL_1_CMD_MASK, 1);

	ret = regmap_write(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_1_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_1_ADDR, tmp, 
		((tmp & (RTL8373_SMI_ACCESS_PHY_CTRL_1_CMD_MASK | RTL8373_SMI_ACCESS_PHY_CTRL_1_FAIL_MASK))==0),
		0, 1000);
	if (ret)
		return ret;
    return 0;
}

int rtl837x_phy_write_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 val)
{
	return rtl837x_phys_write_c45(priv, BIT(phy), devad, regnum, val);
}

int rtl837x_sds_reg_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 *pdata)
{
	int ret;
	u32 val, tmp;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_INDEX_MASK, sds_index) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_PAGE_MASK, sds_page) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_REGAD_MASK, sds_reg) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_RWOP_MASK, 0) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK, 1);

	ret = regmap_write(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	ret = regmap_read(priv->map, RTL8373_SDS_INDACS_RD_ADDR, &val);
	if (ret)
		return ret;

	*pdata = val & 0xFFFF;
	return 0;
}

int rtl837x_sds_reg_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 data)
{
	int ret;
	u32 tmp;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
			  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
			  0, 1000);
	if (ret)
		return ret;

	ret = regmap_write(priv->map, RTL8373_SDS_INDACS_WD_ADDR, data);

	tmp = FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_INDEX_MASK, sds_index) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_PAGE_MASK, sds_page) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_REGAD_MASK, sds_reg) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_RWOP_MASK, 1) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK, 1);

	ret = regmap_write(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	return 0;
}

int rtl837x_sds_reg_bits_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 *pdata)
{
	int ret;
	u16 val;

	ret = rtl837x_sds_reg_read(priv, sds_index, sds_page, sds_reg, &val);
	if (ret)
		return ret;

	*pdata = (val & mask) >> __ffs(mask);
	return 0;
}

int rtl837x_sds_reg_bits_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 data)
{
	int ret;
	u16 val;

	ret = rtl837x_sds_reg_read(priv, sds_index, sds_page, sds_reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= (data << __ffs(mask)) & mask;

	return rtl837x_sds_reg_write(priv, sds_index, sds_page, sds_reg, val);
}

int rtl837x_rtl8224_reg_bits_read(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 *pval)
{
	int ret;
	u32 tmp;

	ret = rtl837x_rtl8224_reg_read(priv, reg, &tmp);
	if (ret)
		return ret;

	*pval = (tmp & mask) >> __ffs(mask);
	return 0;
}

int rtl837x_rlt8224_reg_bits_write(struct rtl837x_priv *priv, u32 reg, u32 mask, u32 val)
{
    return regmap_update_bits(priv->map_8224, reg, mask, (val << __ffs(mask)) & mask);
}

int rtl837x_rtl8224_sds_reg_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 *pdata)
{
	int ret;
	u32 val, tmp;

	ret = regmap_read_poll_timeout(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_INDEX_MASK, sds_index) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_PAGE_MASK, sds_page) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_REGAD_MASK, sds_reg) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_RWOP_MASK, 0) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK, 1);

	ret = regmap_write(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	ret = regmap_read(priv->map_8224, RTL8373_SDS_INDACS_RD_ADDR, &val);
	if (ret)
		return ret;

	*pdata = val & 0xFFFF;
	return 0;
}

int rtl837x_rtl8224_sds_reg_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 data)
{
	int ret;
	u32 tmp;

	ret = regmap_read_poll_timeout(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
			  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
			  0, 1000);
	if (ret)
		return ret;

	ret = regmap_write(priv->map_8224, RTL8373_SDS_INDACS_WD_ADDR, data);

	tmp = FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_INDEX_MASK, sds_index) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_PAGE_MASK, sds_page) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_REGAD_MASK, sds_reg) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_RWOP_MASK, 1) |
				FIELD_PREP(RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK, 1);

	ret = regmap_write(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map_8224, RTL8373_SDS_INDACS_CMD_ADDR, tmp,
		  ((tmp & RTL8373_SDS_INDACS_CMD_SDS_CMD_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	return 0;
}

int rtl837x_rtl8224_sds_reg_bits_read(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 *pdata)
{
	int ret;
	u16 val;

	ret = rtl837x_rtl8224_sds_reg_read(priv, sds_index, sds_page, sds_reg, &val);
	if (ret)
		return ret;

	*pdata = (val & mask) >> __ffs(mask);
	return 0;
}

int rtl837x_rtl8224_sds_reg_bits_write(struct rtl837x_priv *priv, u8 sds_index, u16 sds_page, u16 sds_reg, u16 mask, u16 data)
{
	int ret;
	u16 val;

	ret = rtl837x_rtl8224_sds_reg_read(priv, sds_index, sds_page, sds_reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= (data << __ffs(mask)) & mask;

	return rtl837x_rtl8224_sds_reg_write(priv, sds_index, sds_page, sds_reg, val);
}

int rtl837x_vlan_set(struct rtl837x_priv *priv, struct rtl837x_vlan_data *vlan)
{
	int ret;
	u32 tmp;

	ret = regmap_write(priv->map, RTL8373_ITA_WRITE_DATA0_ADDR(0), vlan->val);
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_ITA_CTRL0_TBL_ADDR_MASK, vlan->vid) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_TYPE_MASK, TB_TARGET_CVLAN) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_ACT_MASK, TB_OP_WRITE) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_EXECUTE_MASK, TB_EXECUTE);

	ret = regmap_write(priv->map, RTL8373_ITA_CTRL0_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_ITA_CTRL0_ADDR, tmp,
		  ((tmp & RTL8373_ITA_CTRL0_TLB_EXECUTE_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;
	return 0;
}

int rtl837x_vlan_get(struct rtl837x_priv *priv, struct rtl837x_vlan_data *vlan)
{
	int ret;
	u32 tmp;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_ITA_CTRL0_ADDR, tmp,
		  ((tmp & RTL8373_ITA_CTRL0_TLB_EXECUTE_MASK) == 0),
		  0, 1000);

	ret = regmap_write(priv->map, RTL8373_ITA_WRITE_DATA0_ADDR(0), vlan->val);
	if (ret)
		return ret;

	tmp = FIELD_PREP(RTL8373_ITA_CTRL0_TBL_ADDR_MASK, vlan->vid) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_TYPE_MASK, TB_TARGET_CVLAN) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_ACT_MASK, TB_OP_READ) |
		  FIELD_PREP(RTL8373_ITA_CTRL0_TLB_EXECUTE_MASK, TB_EXECUTE);

	ret = regmap_write(priv->map, RTL8373_ITA_CTRL0_ADDR, tmp);
	if (ret)
		return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_ITA_CTRL0_ADDR, tmp,
		  ((tmp & RTL8373_ITA_CTRL0_TLB_EXECUTE_MASK) == 0),
		  0, 1000);
	if (ret)
		return ret;

	ret = regmap_read(priv->map, RTL8373_ITA_READ_DATA0_ADDR(0), &tmp);
	if (ret)
		return ret;
	vlan->val = tmp;

	return 0;
}

#define _reg_read(is_8224, priv, reg, pval) \
	  (is_8224 ? rtl837x_rtl8224_reg_read(priv, reg, pval): \
	             rtl837x_reg_read(priv, reg, pval))
#define _reg_write(is_8224, priv, reg, val) \
	  (is_8224 ? rtl837x_rtl8224_reg_write(priv, reg, val): \
	             rtl837x_reg_write(priv, reg, val))
#define _reg_bits_read(is_8224, priv, reg, mask, pval) \
	  (is_8224 ? rtl837x_rtl8224_reg_bits_read(priv, reg, mask, pval): \
	             rtl837x_reg_bits_read(priv, reg, mask, pval))
#define _reg_bits_write(is_8224, priv, reg, mask, val) \
	  (is_8224 ? rtl837x_rtl8224_reg_bits_write(priv, reg, mask, val): \
	             rtl837x_reg_bits_write(priv, reg, mask, val))

#define _sds_reg_read(is_8224, priv, sds_idx, page, reg, pval) \
	  (is_8224 ? rtl837x_rtl8224_sds_reg_read(priv, sds_idx, page, reg, pval): \
	             rtl837x_sds_reg_read(priv, sds_idx, page, reg, pval))
#define _sds_reg_write(is_8224, priv, sds_idx, page, reg, val) \
	  (is_8224 ? rtl837x_rtl8224_sds_reg_write(priv, sds_idx, page, reg, val): \
	             rtl837x_sds_reg_write(priv, sds_idx, page, reg, val))
#define _sds_reg_bits_read(is_8224, priv, sds_idx, page, reg, mask, pval) \
	  (is_8224 ? rtl837x_rtl8224_sds_reg_bits_read(priv, sds_idx, page, reg, mask, pval): \
	             rtl837x_sds_reg_bits_read(priv, sds_idx, page, reg, mask, pval))
#define _sds_reg_bits_write(is_8224, priv, sds_idx, page, reg, mask, val) \
	  (is_8224 ? rtl837x_rtl8224_sds_reg_bits_write(priv, sds_idx, page, reg, mask, val): \
	             rtl837x_sds_reg_bits_write(priv, sds_idx, page, reg, mask, val))

// 10G reset
static int _rtl837x_sds_reset_R(bool is_8224, struct rtl837x_priv *priv, u16 sds_idx)
{
	int ret;
	u16 rx_sts, tmp;
	bool rx_idle, nsq, sync_ok, link_ok, hi_ber; // nsq:(Noise Squelch) hi_ber:(High bit error rate)

	ret = _sds_reg_bits_read(is_8224, priv, sds_idx, 0x20, 0, 0x30, &rx_sts);
	if (ret)
		return ret;

	// check serdes rx status
	// 0,2,3: rx is enabled
	// 1    : rx is disabled
	if (rx_sts == 1) // do nothing when rx is disabled
		return 0;

	// enable rx test
	// switch debug port (What is this?)
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x21, 0x00, BIT(2), 1); // RX_TEST_EN
	if (ret)
		return ret;
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x36, 0x05, 0xf<<11, 8); // REG0_DEBUG_SEL
	if (ret)
		return ret;
	ret = _sds_reg_write(is_8224, priv, sds_idx, 0x1f, 0x02, 0x1f); // switch debug port
	if (ret)
		return ret;

	// Get RX status
	ret = _sds_reg_read(is_8224, priv, sds_idx, 0x1f, 0x15, &tmp);
	if (ret)
		return ret;
	rx_idle = !!((tmp>>7)&1);
	nsq = !!((tmp>>6)&1);

	// Do reset when RX is not idle or RX Noise Squelch
	if (!(nsq==1 || rx_idle==0))
		return 0;

	// check sync_ok
	ret = _sds_reg_read(is_8224, priv, sds_idx, 0x05, 0x00, &tmp);
	if (ret)
		return ret;

	sync_ok = !!(tmp&1);
	link_ok = !!((tmp>>12)&1);
	hi_ber = !!((tmp>>1)&1);

	if (sync_ok == 0)
		goto do_reset;
	else
		if ((link_ok==0) || (hi_ber==1))
			goto do_reset;
	return 0;

do_reset:
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x20, 0x00, 0x3<<4, 3);
	if (ret) return ret;
	msleep(1);
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x20, 0x00, 0x3<<4, 1);
	if (ret) return ret;
	msleep(1);
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x20, 0x00, 0x3<<4, 3);
	if (ret) return ret;
	msleep(1);
	ret = _sds_reg_bits_write(is_8224, priv, sds_idx, 0x20, 0x00, 0x3<<4, 0);
	if (ret) return ret;
	msleep(1);

	return 0;
}

int rtl837x_sds_reset_R(struct rtl837x_priv *priv, u16 sds_idx)
{
	return _rtl837x_sds_reset_R(false, priv, sds_idx);
}

int rtl837x_rtl8224_sds_reset_R(struct rtl837x_priv *priv, u16 sds_idx)
{
	return _rtl837x_sds_reset_R(true, priv, sds_idx);
}

// 100M/1G/2.5G/5G reset
int rtl837x_sds_reset_X(struct rtl837x_priv *priv, u16 sds_idx)
{
	int ret;
	u16 rx_sts, tmp;
	bool sig_ok, sync_ok, link_ok;

	ret = rtl837x_sds_reg_bits_read(priv, sds_idx, 0x20, 0, 0x30, &rx_sts);
	if (ret)
		return ret;

	// check serdes rx status
	// 0,2,3: rx is enabled
	// 1    : rx is disabled
	if (rx_sts == 1) // do nothing when rx is disabled
		return 0;

	ret = rtl837x_sds_reg_read(priv, sds_idx, 0x01, 0x1d, &tmp);
	if (ret)
		return ret;

	sig_ok = !!((tmp>>8)&1);
	link_ok = !!((tmp>>4)&1);
	sync_ok = !!(tmp&1);

	if (!sig_ok)
		return 0;

	if (sync_ok==0)
		goto do_reset;
	else
		if (link_ok==0)
			goto do_reset;
	return 0;

do_reset:
	ret = rtl837x_sds_reg_bits_write(priv, sds_idx, 0x00, 0x00, BIT(1), 1);
	if (ret) return ret;
	msleep(1);
	ret = rtl837x_sds_reg_bits_write(priv, sds_idx, 0x00, 0x00, BIT(1), 0);
	if (ret) return ret;
	msleep(1);
	ret = rtl837x_sds_reg_bits_write(priv, sds_idx, 0x00, 0x00, BIT(1), 1);
	if (ret) return ret;
	msleep(1);
	return 0;
}

// TODO: remove this in the future
rtk_api_ret_t rtk_hal_init(void)
{
    rtk_int32  retVal;
    switch_chip_t   switchChip;
    rtk_switch_halCtrl_t **phalCtrl = hal_ctrlInfo_p_get();
    
    /* Find device */
    if((*phalCtrl = hal_find_device()) == NULL)
        return RT_ERR_CHIP_NOT_FOUND;

    if((retVal = phy_identify_init()) != RT_ERR_OK)
        return retVal;

    /* Attached DAL mapper */
    switchChip = (*phalCtrl)->switch_type;
    if((retVal = dal_mgmt_attachDevice(switchChip)) != RT_ERR_OK)
        return retVal;

    /* Set initial state */
    if((retVal = rtk_switch_initialState_set(INIT_COMPLETED)) != RT_ERR_OK)
        return retVal;
    return RT_ERR_OK;
}
