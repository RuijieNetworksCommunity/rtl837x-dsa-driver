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
