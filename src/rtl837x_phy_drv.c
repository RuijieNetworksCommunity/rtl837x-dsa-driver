#include <linux/bitops.h>
#include <linux/of.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/string_choices.h>

#include "rtl837x.h"

int rtl837x_read_mmd(struct phy_device *phydev, int devnum, u16 regnum)
{
    struct mii_bus *bus = phydev->mdio.bus;
    struct rtl837x_priv *priv;
        // 从 MDIO 总线获取私有数据
    if (!bus || !bus->priv) {
        dev_err(&phydev->mdio.dev, "No private data in MDIO bus\n");
        return -ENODEV;
    }
    priv = bus->priv;
    dev_info(priv->dev, "got priv %s\n", priv->chip_name);
    
    return 0;
}

int rtl837x_write_mmd(struct phy_device *phydev, int devnum, u16 regnum, u16 val)
{
    struct mii_bus *bus = phydev->mdio.bus;
    struct rtl837x_priv *priv;
        // 从 MDIO 总线获取私有数据
    if (!bus || !bus->priv) {
        dev_err(&phydev->mdio.dev, "No private data in MDIO bus\n");
        return -ENODEV;
    }
    
    priv = bus->priv;
    dev_info(priv->dev, "got priv %s\n", priv->chip_name);
    return 0;
}

static struct phy_driver rtl837x_phy_drvs[] = {
    {
		.phy_id		= 0x1CCAD0,
		.phy_id_mask	= 0xFFFFFFF0,
		.name		= "RTL8372N Gigabit PHY",
		.read_mmd	= &rtl837x_read_mmd,
		.write_mmd	= &rtl837x_write_mmd,
	},
};

int rtl837x_phy_module_init(struct module *owner, const void *driver_data)
{
    rtl837x_phy_drvs[0].driver_data = driver_data;
	return phy_drivers_register(rtl837x_phy_drvs, ARRAY_SIZE(rtl837x_phy_drvs), owner);
}

void rtl837x_phy_module_exit(void)
{
    printk("awa\n");
	phy_drivers_unregister(rtl837x_phy_drvs, ARRAY_SIZE(rtl837x_phy_drvs));
}