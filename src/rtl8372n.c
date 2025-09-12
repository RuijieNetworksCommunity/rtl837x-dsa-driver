#include <linux/bitops.h>
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/regmap.h>

#include "rtl837x.h"

#define RTL8372N_PORT_NUM_CPU 3
#define RTL8372N_NUM_VLANS 4096
#define RTL8372N_NUM_PORTS 8
#define RTL8372N_VLAN_UNTAG_MASK 0x3FF
#define RTL8372N_VLAN_MEMBER_MASK 0x3FF
#define RTL8372N_VLAN_FID_MASK 0xF

static struct rtl837x_mib_counter rtl8372n_mib_counters[] ={
	{0,"ifInOctets"},
	{2,"ifOutOctets"},
	{4,"ifInUcastPkts"},
	{6,"ifInMulticastPkts"},
	{8,"ifInBroadcastPkts"},
	{0xA,"ifOutUcastPkts"},
	{0xC,"ifOutMulticastPkts"},
	{0xE,"ifOutBroadcastPkts"},
	{0x10,"ifOutDiscards"},
	{0x19,"InPauseFrames"},
	{0x1A,"OutPauseFrames"},
	{0x1C,"TxBroadcastPkts"},
	{0x1D,"TxMulticastPkts"},
	{0x20,"TxUndersizePkts"},
	{0x21,"RxUndersizePkts"},
	{0x22,"TxOversizePkts"},
	{0x23,"RxOversizePkts"},
	{0x24,"TxFragments"},
	{0x25,"RxFragments"},
	{0x26,"TxJabbers"},
	{0x27,"RxJabbers"},
	{0x28,"TxCollisions"},
	{0x29,"Tx64Octets"},
	{0x2A,"Rx64Octets"},
	{0x2B,"Tx65to127Bytes"},
	{0x2C,"Rx65to127Bytes"},
	{0x2D,"Tx128to255Bytes"},
	{0x2E,"Rx128to255Bytes"},
	{0x2F,"Tx256to511Bytes"},
	{0x30,"Rx256to511Bytes"},
	{0x31,"Tx512to1023Bytes"},
	{0x32,"Rx512to1023Bytes"},
	{0x33,"Tx1024to1518Bytes"},
	{0x34,"Rx1024to1518Bytes"},
	{0x36,"RxUndersizedropPkts"},
	{0x37,"Tx1519toMaxBytes"},
	{0x38,"Rx1519toMaxBytes"},
	{0x39,"TxOverMaxBytes"},
	{0x3A,"RxOverMaxBytes"}
};

struct rtl8372n {
	bool pvid_enabled[RTL8372N_NUM_PORTS];
};

// const uint8_t rtl8372_port_map[16] = {
//     3, 4, 5, 6, 7, 8, // 物理端口3-8
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // 填充
// };

static int rtl8372n_detect(struct rtl837x_priv *priv)
{
	struct device *dev = priv->dev;
    rtl_gbl_priv = priv;
	int ret;
	u32 val;

    switch_chip_t sw_chip;
	switch_probe(&sw_chip);

	ret = regmap_read(priv->map, 0x4, &val);
    dev_info(dev, "CHIP_ID: 0x%x \n", val);
	switch (sw_chip) {
        case CHIP_RTL8372N:
            dev_info(dev, "found an %s switch\n", chipid_to_chip_name(sw_chip));
            priv->cpu_port = RTL8372N_PORT_NUM_CPU;
            priv->num_ports = RTL8372N_NUM_PORTS;
            // priv->port_map = rtl8372_port_map;
            priv->mib_counters = rtl8372n_mib_counters;
            priv->num_mib_counters = ARRAY_SIZE(rtl8372n_mib_counters);
            break;
        case CHIP_RTL8373:
        case CHIP_RTL8224:
        case CHIP_RTL8372:
        case CHIP_RTL8373N:
        case CHIP_RTL8221B:
        case CHIP_RTL8224N:
            dev_info(dev, "found an %s switch\n", chipid_to_chip_name(sw_chip));
            dev_err(dev, "this switch is not yet supported!\n");
            return -ENODEV;
        default:
            dev_info(dev, "found an Unknown Realtek switch (id=0x%04x)\n",
                val);
            return -ENODEV;
	}
	return 0;
}


static int rtl8372n_get_vlan_4k(struct rtl837x_priv *priv, u32 vid,
				 struct rtl837x_vlan_4k *vlan4k)
{

	int ret;

	memset(vlan4k, '\0', sizeof(struct rtl837x_vlan_4k));
    rtk_vlan_entry_t vlanCfg;
    ret = rtk_vlan_get(vid, &vlanCfg);
    if(ret) return ret;

	vlan4k->vid = vid;
	vlan4k->member = vlanCfg.mbr.bits[0] & RTL8372N_VLAN_MEMBER_MASK;
	vlan4k->untag = vlanCfg.untag.bits[0] & RTL8372N_VLAN_UNTAG_MASK;
	vlan4k->fid = vlanCfg.fid_msti & RTL8372N_VLAN_FID_MASK;

	return 0;
}
static int rtl8372n_set_vlan_4k(struct rtl837x_priv *priv,
			       const struct rtl837x_vlan_4k *vlan4k)
{
	int ret;

    rtk_vlan_entry_t vlanCfg;
	memset(&vlanCfg, '\0', sizeof(rtk_vlan_entry_t));

    if(ret) return ret;

    vlanCfg.mbr.bits[0] = vlan4k->member;
    vlanCfg.untag.bits[0] = vlan4k->untag;
    vlanCfg.fid_msti = vlan4k->fid;
    vlanCfg.ivl_svl = 1;

    ret = rtk_vlan_set(vlan4k->vid, &vlanCfg);

	return ret;
}

static int rtl8372n_get_mib_counter(struct rtl837x_priv *priv,
                int port,
                struct rtl837x_mib_counter *mib,
                u64 *mibvalue)
{
    int ret;
    rtk_stat_counter_t counter;
    ret = rtk_stat_port_get(port, mib->base, &counter);
    if(ret) return ret;

    *mibvalue = counter;
    return 0;
}
static int rtl8372n_enable_vlan(struct rtl837x_priv *priv, bool enable)
{
    return 0;
}

static enum dsa_tag_protocol rtl8372n_get_tag_protocol(struct dsa_switch *ds,
                                                        int port,
                                                        enum dsa_tag_protocol mp)
{
    struct rtl837x_priv *priv = ds->priv;
	struct device *dev = priv->dev;
    dev_info(dev, "get_DSA_PROTO\n");

    // if (dsa_is_cpu_port(ds, port)) {
        // 配置 CPU 端口标签设置
		return DSA_TAG_PROTO_RTL8_4;
    // }
	
	// return DSA_TAG_PROTO_NONE;
}

static int rtl8372n_phy_read_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum)
{
    rtk_uint32 data;

    int ret = rtk_port_phyReg_get(phy, devad, regnum, &data);
    if (ret != RT_ERR_OK)
    {
        dev_info(priv->dev,"ERROR:rtk_port_phyReg_get ret: %d\n", ret);
        return -EIO;
    }

    return data;
}

static int rtl8372n_phy_write_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 val)
{

    int ret = rtk_port_phyReg_set(1 << phy, devad, regnum, val);
    if (ret != RT_ERR_OK)
    {
        dev_info(priv->dev,"ERROR:rtk_port_phyReg_set ret: %d\n", ret);
        return -EIO;
    }

    return 0;
}

static int rtl8372n_mdio_phy_read_c45(struct mii_bus *bus, int port, int devad, int regnum)
{
	struct rtl837x_priv *priv = bus->priv;

	return priv->ops->phy_read_c45(priv, port, devad, regnum);
}

static int rtl8372n_mdio_phy_write_c45(struct mii_bus *bus, int port, int devad, int regnum, u16 val)
{
	struct rtl837x_priv *priv = bus->priv;

	return priv->ops->phy_write_c45(priv, port, devad, regnum, val);
}

static int rtl8372n_setup_mdio(struct rtl837x_priv *priv)
{
	struct device_node *np = priv->dev->of_node;
    struct device_node *mnp;
	struct dsa_switch *ds = priv->ds;
	struct device *dev = priv->dev;
	struct mii_bus *bus;
	static int idx;
	int ret = 0;

    mnp = of_get_child_by_name(np, "mdio");

	if (mnp && !of_device_is_available(mnp))
		goto out;

	bus = devm_mdiobus_alloc(dev);
	if (!bus) {
		ret = -ENOMEM;
		goto out;
	}

    if (!mnp)
		ds->slave_mii_bus = bus;

    bus->priv = priv;
	bus->name = KBUILD_MODNAME "-mii";
	snprintf(bus->id, MII_BUS_ID_SIZE, KBUILD_MODNAME "-%d", idx++);
	// bus->read = rtl8372n_phy_read_c22;
	// bus->write = rtl8372n_phy_write_c22;
	bus->read_c45 = rtl8372n_mdio_phy_read_c45;
	bus->write_c45 = rtl8372n_mdio_phy_write_c45;
	bus->parent = dev;
	bus->phy_mask = ~ds->phys_mii_mask;

	ret = devm_of_mdiobus_register(dev, bus, mnp);
	if (ret) {
		dev_err(dev, "failed to register MDIO bus: %d\n", ret);
	}
out:
	of_node_put(mnp);
	return ret;
}

static int rtl8372n_setup(struct dsa_switch *ds)
{
    struct rtl837x_priv *priv = ds->priv;
    rtl_gbl_priv = priv;
    int ret;

    dev_info(priv->dev,"Start init RTL8372N Switch");

    ret = rtk_switch_init();
	if(ret){
		dev_err(priv->dev, "rtk_switch_init Fail, erron:%d\n", ret);
		return -1;
	}

    ret = rtl8372n_setup_mdio(priv);
	if(ret){
		dev_err(priv->dev, "rtl8372n_setup_mdio Fail, erron:%d\n", ret);
		return ret;
	}

	ret = rtk_vlan_reset();
    if (ret)
    {
		dev_err(priv->dev, "rtk_vlan_reset failed, errno:%d\n", ret);
		return -1;
    }

	ret = rtk_vlan_init();
    if (ret)
    {
		dev_err(priv->dev, "rtk_vlan_init failed, errno:%d\n", ret);
		return -1;
    }

	rtk_rmaParam_t pRmacfg;
	ret = rtk_rma_get(2, &pRmacfg);
	if ( ret )
	{
		dev_err(priv->dev, "rtk_rma_get get rma failed, errno %d\n", ret);
        return -1;
	}

	pRmacfg.operation = RMAOP_FORWARD;
	ret = rtk_rma_set(2, &pRmacfg);
	if ( ret )
	{
		dev_err(priv->dev, "rtk_rma_get set rma failed, errno %d\n", ret);
		return -1;
	}

	rtk_port_phy_ability_t ana = {
		.Half_10 = 1,
		.Full_10 = 1,
		.Half_100 = 1,
		.Full_100 = 1,
		.Half_1000 = 0,
		.Full_1000 = 1,
		.adv_2_5G = 1,
		.adv_5G = 0,
		.adv_10GBase_T = 0,
		.FC = 1,
		.AsyFC = 1,
	};

	for(int port = 3;port < priv->num_ports;port++){
		ret = rtk_eee_portTxRxEn_set(port, 0, 0);
		if (ret)
		{
			dev_err(priv->dev, "rtk_eee_portTxRxEn_set failed, error %d\n",ret);
			return -EIO;
		}

		rtk_port_backpressureEnable_set(port, 1);
		if (ret)
		{
			dev_err(priv->dev, "rtk_port_backpressureEnable_set failed, error %d\n",ret);
			return -EIO;
		}

		//跳过CPU端口和serdes端口
		if(port == 3 || port == 8 || port == priv->cpu_port) continue;

		ret = rtk_phy_autoNegoAbility_set(port, &ana); 
		if (ret) {
			dev_err(priv->dev, "port: %d autoNegoAbility configure failed, error: %d", port, ret);
			return EIO;
		}
	}

	ret = rtk_cpu_externalCpuPort_set(priv->cpu_port);
	if (ret)
	{
		dev_err(priv->dev, "rtk_cpu_externalCpuPort_set failed, errno %d\n",ret);
		
		return -1;
	}
	// 设置 CPU 标签 TPID
    rtk_cpuTag_tpid_set(0x8899);

    // 设置标签插入模式为所有帧
    rtk_cpuTag_insertMode_set(EXTERNAL_CPU, CPU_INSERT_TO_ALL);

	// 启用内部 CPU 标签功能
    rtk_cpuTag_enable_set(EXTERNAL_CPU, ENABLED);

    // // 设置所有端口为 CPU 感知端口
    // rtk_portmask_t portmask;
	// portmask.bits[0] = 0x1f;
    // rtk_cpuTag_awarePort_set(&portmask);

    // // 设置优先级映射
    // for (int i = 4; i < 8; i++) {
    //     rtk_cpuTag_priRemap_set(EXTERNAL_CPU, i, i);
    // }

	// rtk_vlan_entry_t vlan_cfg = {
	// 	.ivl_svl = 1,
	// 	.fid_msti = 0,
	// 	.mbr = 0x1ff,
	// 	.untag = 0x1ff
	// };
	// rtk_vlan_set(1,&vlan_cfg);
    return 0;
}

static void rtl8372n_phylink_get_caps(struct dsa_switch *ds, int port,
				       struct phylink_config *config)
{
	struct rtl837x_priv *priv = ds->priv;

	if (port == priv->cpu_port) {
		__set_bit(PHY_INTERFACE_MODE_10GKR, config->supported_interfaces);
		__set_bit(PHY_INTERFACE_MODE_10GBASER, config->supported_interfaces);
		__set_bit(PHY_INTERFACE_MODE_XGMII, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_5GBASER, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_USXGMII, config->supported_interfaces);
		config->mac_capabilities = MAC_10000FD | MAC_5000FD | MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
	} else {
		__set_bit(PHY_INTERFACE_MODE_INTERNAL, config->supported_interfaces);
		config->mac_capabilities = MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
        
	}
}

static void rtl8372n_mac_link_up(struct dsa_switch *ds, int port, unsigned int mode,
                phy_interface_t interface, struct phy_device *phydev,
                int speed, int duplex, bool tx_pause, bool rx_pause)
{
	struct rtl837x_priv *priv = ds->priv;
	int ret;

    // if (dsa_is_cpu_port(ds, port)) {
    //     // 配置 CPU 端口标签设置
    //     rtk_cpuTag_insertMode_set(EXTERNAL_CPU, CPU_INSERT_TO_ALL);
    // }

	switch (port)
	{
	case 4:
	case 5:
	case 6:
	case 7:
		dev_info(priv->dev, "MAC link up on phy port (%d)\n", port);
		ret = 0;
		/* code */
		break;
	case 3:
		dev_info(priv->dev, "MAC link up on serdes port (%d)\n", 0);
		ret = rtk_sdsMode_set(0, SERDES_10GR);
		break;
	case 8:
		dev_info(priv->dev, "MAC link up on serdes port (%d)\n", 1);
		ret = rtk_sdsMode_set(1, SERDES_10GR);
		break;
	}
	
	if (ret) {
		dev_err(priv->dev, "failed to enable the port(%d)\n", port);
		return;
	}
}

static void rtl8372n_mac_link_down(struct dsa_switch *ds, int port, unsigned int mode,
			phy_interface_t interface)
{
	struct rtl837x_priv *priv = ds->priv;
	int ret;

	switch (port)
	{
	case 4:
	case 5:
	case 6:
	case 7:
		dev_info(priv->dev, "MAC link down on phy port (%d)\n", port);
		ret = 0;
		/* code */
		break;
	case 3:
		dev_info(priv->dev, "MAC link down on serdes port (%d)\n", 0);
		ret = rtk_sdsMode_set(0, SERDES_OFF);
		break;
	case 8:
		dev_info(priv->dev, "MAC link down on serdes port (%d)\n", 1);
		ret = rtk_sdsMode_set(1, SERDES_OFF);
		break;
	}
	
	if (ret) {
		dev_err(priv->dev, "failed to disable the port(%d)\n", port);
		return;
	}
}

static void rtl8372n_get_strings(struct dsa_switch *ds, int port, u32 stringset,
			 uint8_t *data)
{
	struct rtl837x_priv *priv = ds->priv;
	struct rtl837x_mib_counter *mib;
	int i;

	if (port >= priv->num_ports)
		return;

	for (i = 0; i < priv->num_mib_counters; i++) {
		mib = &priv->mib_counters[i];
		strncpy(data + i * ETH_GSTRING_LEN,
			mib->name, ETH_GSTRING_LEN);
	}
}

static void rtl8372n_get_ethtool_stats(struct dsa_switch *ds, int port, uint64_t *data)
{
	struct rtl837x_priv *priv = ds->priv;
	int i;
	int ret;

	if (port >= priv->num_ports)
		return;

	for (i = 0; i < priv->num_mib_counters; i++) {
		struct rtl837x_mib_counter *mib;
		u64 mibvalue = 0;

		mib = &priv->mib_counters[i];
		ret = priv->ops->get_mib_counter(priv, port, mib, &mibvalue);
		if (ret) {
			dev_err(priv->dev, "error reading MIB counter %s\n",
				mib->name);
		}
		data[i] = mibvalue;
	}
}

static int rtl8372n_get_sset_count(struct dsa_switch *ds, int port, int sset)
{
	struct rtl837x_priv *priv = ds->priv;

	/* We only support SS_STATS */
	if (sset != ETH_SS_STATS)
		return 0;
	if (port >= priv->num_ports)
		return -EINVAL;

	return priv->num_mib_counters;
}

static int rtl8372n_vlan_filtering(struct dsa_switch *ds, int port,
                                        bool vlan_filtering, struct netlink_ext_ack *extack)
{
    struct rtl837x_priv *priv = ds->priv;
    rtk_api_ret_t ret;
    
	dev_info(priv->dev, "rtl8372n_vlan_filtering port (%d)\n", port);
    // 设置全局出口过滤
    ret = rtk_vlan_egrFilterEnable_set(vlan_filtering ? ENABLED : DISABLED);
    if (ret != RT_ERR_OK)
        return -EIO;
    
    // 设置端口入口过滤
    ret = rtk_vlan_portIgrFilterEnable_set(port, 
                                        vlan_filtering ? ENABLED : DISABLED);
    if (ret != RT_ERR_OK)
        return -EIO;
    
    return 0;
}

static int rtl8372n_vlan_add(struct dsa_switch *ds, int port,
                            const struct switchdev_obj_port_vlan *vlan,
                            struct netlink_ext_ack *extack)
{
    struct rtl837x_priv *priv = ds->priv;
    rtk_api_ret_t ret;
    rtk_vlan_entry_t entry;
    u16 vid = vlan->vid;

	dev_info(priv->dev, "rtl8372n_vlan_add vid (%d)\n", vid);

    if (vid <= 0 || vid >= 4095)
    {
        NL_SET_ERR_MSG_MOD(extack, "VLAN ID not valid");
        return -EINVAL;
    }
    
    // 获取现有 VLAN 配置（如果存在）
    ret = rtk_vlan_get(vid, &entry);
    if (ret == RT_ERR_VLAN_ENTRY_NOT_FOUND) {
        // 新 VLAN：初始化默认配置
        memset(&entry, 0, sizeof(entry));
        entry.fid_msti = 0;
        entry.ivl_svl = 1;
    } else if (ret != RT_ERR_OK) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to get vlan");
        return -EIO;
    }
    
    // 添加端口到 VLAN 成员
    RTK_PORTMASK_PORT_SET(entry.mbr, port);
    
    // 设置 untagged 属性
    if (vlan->flags & BRIDGE_VLAN_INFO_UNTAGGED) {
        RTK_PORTMASK_PORT_SET(entry.untag, port);
    } else {
        RTK_PORTMASK_PORT_CLEAR(entry.untag, port);
    }
    
    // 更新 VLAN 配置
    ret = rtk_vlan_set(vid, &entry);
    if (ret != RT_ERR_OK)
        return -EIO;
    
    // 设置 PVID 如果标记为 PVID
    if (vlan->flags & BRIDGE_VLAN_INFO_PVID) {
        ret = rtk_vlan_portPvid_set(port, vid);
        if (ret != RT_ERR_OK)
            return -EIO;
    }
    
    return 0;
}

static int rtl8372n_vlan_del(struct dsa_switch *ds, int port,
                                 const struct switchdev_obj_port_vlan *vlan)
{
    struct rtl837x_priv *priv = ds->priv;

    rtk_api_ret_t ret;
    rtk_vlan_entry_t entry;
    u16 vid = vlan->vid;

	dev_info(priv->dev, "rtl8372n_vlan_del vid (%d)\n", vid);
    
    if (vid <= 0 || vid >= 4095)
        return 0;
    
    // 获取 VLAN 配置
    ret = rtk_vlan_get(vid, &entry);
    if (ret != RT_ERR_OK) {
        if (ret == RT_ERR_VLAN_ENTRY_NOT_FOUND)
            return 0;  // VLAN 不存在，无需操作
        return -EIO;
    }
    
    // 从成员中移除端口
    RTK_PORTMASK_PORT_CLEAR(entry.mbr, port);
    RTK_PORTMASK_PORT_CLEAR(entry.untag, port);
    
    // 更新 VLAN 配置
    ret = rtk_vlan_set(vid, &entry);
    if (ret != RT_ERR_OK)
        return -EIO;
    
    // 如果删除的是 PVID，恢复默认 PVID
    if (vlan->flags & BRIDGE_VLAN_INFO_PVID) {
        ret = rtk_vlan_portPvid_set(port, 0);
        if (ret != RT_ERR_OK)
            return -EIO;
    }
    
    return 0;
}

static const struct dsa_switch_ops rtl8372n_switch_ops_mdio = {
	.get_tag_protocol = rtl8372n_get_tag_protocol,
	.setup = rtl8372n_setup,

	.phylink_get_caps = rtl8372n_phylink_get_caps,
	.phylink_mac_link_up = rtl8372n_mac_link_up,
	.phylink_mac_link_down = rtl8372n_mac_link_down,
	.get_strings = rtl8372n_get_strings,
	.get_ethtool_stats = rtl8372n_get_ethtool_stats,
	.get_sset_count = rtl8372n_get_sset_count,

	.port_vlan_filtering = rtl8372n_vlan_filtering,
	.port_vlan_add = rtl8372n_vlan_add,
	.port_vlan_del = rtl8372n_vlan_del,

};

static const struct rtl837x_ops rtl8372n_ops = {
	.detect		= rtl8372n_detect,
	.get_vlan_4k	= rtl8372n_get_vlan_4k,
	.set_vlan_4k	= rtl8372n_set_vlan_4k,
	.get_mib_counter = rtl8372n_get_mib_counter,
	.enable_vlan	= rtl8372n_enable_vlan,

    .phy_read_c45   = rtl8372n_phy_read_c45,
    .phy_write_c45  = rtl8372n_phy_write_c45,
};

const struct rtl837x_variant rtl8372n_variant = {
	.ds_ops_mdio = &rtl8372n_switch_ops_mdio,
	.ops = &rtl8372n_ops,
	.chip_data_sz = sizeof(struct rtl8372n),
};
EXPORT_SYMBOL_GPL(rtl8372n_variant);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("air jinkela <air_jinkela@163.com>");
MODULE_DESCRIPTION("rtl8372n switch driver for MT7988");
