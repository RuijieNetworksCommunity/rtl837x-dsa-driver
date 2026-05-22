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
	case PHY_INTERFACE_MODE_2500BASEX:
		return SERDES_2500BASEX;
	case PHY_INTERFACE_MODE_10GBASER:
	case PHY_INTERFACE_MODE_10GKR:
	default:
		return SERDES_10GR;
	}
}

int rtl837x_phy_read_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum)
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

	return (tmp & RTL8373_SMI_ACCESS_PHY_CTRL_2_DATA_15_0_MASK) >> __ffs(RTL8373_SMI_ACCESS_PHY_CTRL_2_DATA_15_0_MASK);
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

// will remove this in the feature
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
