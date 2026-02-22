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
#define RTL8372N_NUM_PORTS 9
#define RTL8372N_VLAN_UNTAG_MASK 0x3FF
#define RTL8372N_VLAN_MEMBER_MASK 0x3FF
#define RTL8372N_VLAN_FID_MASK 0xF

static struct rtl837x_mib_counter rtl8372n_mib_counters[] ={
	{ 0,  2, "ifInOctets"        },
	{ 2,  2, "ifOutOctets"       },
	{ 4,  2, "ifInUcastPkts"     },
	{ 6,  2, "ifInMulticastPkts" },
	{ 8,  2, "ifInBroadcastPkts" },
	{ 10, 2, "ifOutUcastPkts"    },
	{ 12, 2, "ifOutMulticastPkts"},
	{ 14, 2, "ifOutBroadcastPkts"},
	{ 16, 2, "ifOutDiscards"     },

	{ 17, 1, "dot1dTpPortInDiscards"            },
	{ 18, 1, "dot3StatsSingleCollisionFrames"   },
	{ 19, 1, "dot3StatMultipleCollisionFrames"  },
	{ 20, 1, "dot3sDeferredTransmissions"       },
	{ 21, 1, "dot3StatsLateCollisions"          },
	{ 22, 1, "dot3StatsExcessiveCollisions"     },
	{ 23, 1, "dot3StatsSymbolErrors"            },
	{ 24, 1, "dot3ControlInUnknownOpcodes"      },
	{ 25, 1, "dot3InPauseFrames"                },
	{ 26, 1, "dot3OutPauseFrames"               },
	{ 27, 1, "etherStatsDropEvents"             },
	{ 28, 1, "tx_etherStatsBroadcastPkts"       },
	{ 29, 1, "tx_etherStatsMulticastPkts"       },
	{ 30, 1, "tx_etherStatsCRCAlignErrors"      },
	{ 31, 1, "rx_etherStatsCRCAlignErrors"      },
	{ 32, 1, "tx_etherStatsUndersizePkts"       },
	{ 33, 1, "rx_etherStatsUndersizePkts"       },
	{ 34, 1, "tx_etherStatsOversizePkts"        },
	{ 35, 1, "rx_etherStatsOversizePkts"        },
	{ 36, 1, "tx_etherStatsFragments"           },
	{ 37, 1, "rx_etherStatsFragments"           },
	{ 38, 1, "tx_etherStatsJabbers"             },
	{ 39, 1, "rx_etherStatsJabbers"             },
	{ 40, 1, "tx_etherStatsCollisions"          },
	{ 41, 1, "tx_etherStatsPkts64Octets"        },
	{ 42, 1, "rx_etherStatsPkts64Octets"        },
	{ 43, 1, "tx_etherStatsPkts65to127Octets"   },
	{ 44, 1, "rx_etherStatsPkts65to127Octets"   },
	{ 45, 1, "tx_etherStatsPkts128to255Octets"  },
	{ 46, 1, "rx_etherStatsPkts128to255Octets"  },
	{ 47, 1, "tx_etherStatsPkts256to511Octets"  },
	{ 48, 1, "rx_etherStatsPkts256to511Octets"  },
	{ 49, 1, "tx_etherStatsPkts512to1023Octets" },
	{ 50, 1, "rx_etherStatsPkts512to1023Octets" },
	{ 51, 1, "tx_etherStatsPkts1024to1518Octets"},
	{ 52, 1, "rx_etherStatsPkts1024to1518Octets"},

	{ 54, 1, "rx_etherStatsUndersizedropPkts"        },
	{ 55, 1, "tx_etherStatsPkts1519toMaxOctets"      },
	{ 56, 1, "rx_etherStatsPkts1519toMaxOctets"      },
	{ 57, 1, "tx_etherStatsPktsOverMaxOctets"        },
	{ 58, 1, "rx_etherStatsPktsOverMaxOctets"        },
	{ 59, 1, "tx_etherStatsPktsFlexibleOctetsSET1"   },
	{ 60, 1, "rx_etherStatsPktsFlexibleOctetsSET1"   },
	{ 61, 1, "tx_etherStatsPktsFlexibleOctetsCRCSET1"},
	{ 62, 1, "rx_etherStatsPktsFlexibleOctetsCRCSET1"},
	{ 63, 1, "tx_etherStatsPktsFlexibleOctetsSET0"   },
	{ 64, 1, "rx_etherStatsPktsFlexibleOctetsSET0"   },
	{ 65, 1, "tx_etherStatsPktsFlexibleOctetsCRSET0C"},
	{ 66, 1, "rx_etherStatsPktsFlexibleOctetsCRSET0C"},
	{ 67, 1, "lengthFieldError"                      },
	{ 68, 1, "falseCarrieimes"                       },
	{ 69, 1, "underSizeOctets"                       },
	{ 70, 1, "framingErrors"                         },

	{ 72, 1, "rxMacDiscards"              },
	{ 73, 1, "rxMacIPGShortDropRT"        },

	{ 75, 1, "dot1dTpLearnedEntryDiscards"},
	{ 76, 1, "egrQueue7DropPktRT"         },
	{ 77, 1, "egrQueue6DropPktRT"         },
	{ 78, 1, "egrQueue5DropPktRT"         },
	{ 79, 1, "egrQueue4DropPktRT"         },
	{ 80, 1, "egrQueue3DropPktRT"         },
	{ 81, 1, "egrQueue2DropPktRT"         },
	{ 82, 1, "egrQueue1DropPktRT"         },
	{ 83, 1, "egrQueue0DropPktRT"         },
	{ 84, 1, "egrQueue7OutPktRT"          },
	{ 85, 1, "egrQueue6OutPktRT"          },
	{ 86, 1, "egrQueue5OutPktRT"          },
	{ 87, 1, "egrQueue4OutPktRT"          },
	{ 88, 1, "egrQueue3OutPktRT"          },
	{ 89, 1, "egrQueue2OutPktRT"          },
	{ 90, 1, "egrQueue1OutPktRT"          },
	{ 91, 1, "egrQueue0OutPktRT"          },

	{ 92, 2, "TxGoodCnt"                  },
	{ 94, 2, "RxGoodCnt"                  },

	{ 96, 1, "RxErrorCnt"                 },
	{ 97, 1, "TxErrorCnt"                 },

	{ 98, 2, "TxGoodCnt_phy"              },
	{ 100, 2, "RxGoodCnt_phy"             },

	{ 102, 1, "RxErrorCnt_phy"            },
	{ 103, 1, "TxErrorCnt_phy"            }
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
    ret = rtk_stat_port_get(port, mib->offset, &counter);
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
    dev_info(dev, "get_DSA_PROTO port:%d\n", port);

	return DSA_TAG_PROTO_RTL8_4;
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

    dev_info(priv->dev,"Start init RTL8372N Switch\n");

    ret = rtk_switch_init();
	if(ret){
		dev_err(priv->dev, "rtk_switch_init Fail, error:%d\n", ret);
		return -EIO;
	}

    ret = rtl8372n_setup_mdio(priv);
	if(ret){
		dev_err(priv->dev, "rtl8372n_setup_mdio Fail, error:%d\n", ret);
		return ret;
	}

	for(int port = 0;port < priv->num_ports;port++){
		if (dsa_is_unused_port(priv->ds, port))
			continue;

    	/* Disable per-port learning limits */
        rtk_l2_limitLearningCnt_set(port, 0);
        rtk_l2_limitLearningCntAction_set(port, LIMIT_LEARN_CNT_ACTION_FORWARD);

		rtk_vlan_tagMode_set(port, VLAN_EGRESS_TAG_MODE_KEEP_FORMAT);
		rtk_vlan_portAcceptFrameType_set(port, ACCEPT_FRAME_TYPE_ALL);
		rtk_vlan_portIgrFilterEnable_set(port, DISABLED);
		ret = rtk_eee_portTxRxEn_set(port, DISABLED, DISABLED);
		if (ret)
		{
			dev_err(priv->dev, "rtk_eee_portTxRxEn_set failed, error %d\n",ret);
			return -EIO;
		}

		rtk_port_backpressureEnable_set(port, ENABLED);
		if (ret)
		{
			dev_err(priv->dev, "rtk_port_backpressureEnable_set failed, error %d\n",ret);
			return -EIO;
		}

		//跳过CPU端口和serdes端口
		if(port == UTP_PORT3 || port == UTP_PORT8 || port == priv->cpu_port) continue;

		rtk_port_t isolation_port_mask = (1 << priv->cpu_port);

		ret = rtk_port_isolation_set(port, isolation_port_mask);
		if (ret) {
			dev_err(priv->dev, "port: %d rtk_port_isolation_set configure failed, error: %d\n", port, ret);
			return -EIO;
		}

	}

    rtk_l2_limitSystemLearningCnt_set(0);
    rtk_l2_limitSystemLearningCntAction_set(LIMIT_LEARN_CNT_ACTION_FORWARD);

	rtk_vlan_egrFilterEnable_set(DISABLED);

	ret = rtk_mirror_keep_set(MIRROR_KEEP_ORIGINAL);
	if (ret)
	{
		dev_err(priv->dev, "rtk_mirror_keep_set failed, error %d\n",ret);
		return -1;
	}

	// ret = rtk_mirror_isolationLeaky_set(ENABLED, ENABLED);
	// if (ret)
	// {
	// 	dev_err(priv->dev, "rtk_mirror_isolationLeaky_set failed, error %d\n",ret);
		
	// 	return -1;
	// }

	ret = rtk_mirror_vlanLeaky_set(DISABLED, DISABLED);
	if (ret)
	{
		dev_err(priv->dev, "rtk_mirror_vlanLeaky_set failed, error %d\n",ret);
		
		return -1;
	}

	ret = rtk_cpu_externalCpuPort_set(priv->cpu_port);
	if (ret)
	{
		dev_err(priv->dev, "rtk_cpu_externalCpuPort_set failed, error %d\n",ret);
		return -1;
	}

    ret = rtk_cpuTag_insertMode_set(EXTERNAL_CPU, CPU_INSERT_TO_ALL);
	if (ret)
	{
		dev_err(priv->dev, "rtk_cpuTag_insertMode_set failed, error %d\n",ret);
		return -1;
	}

    ret = rtk_cpuTag_enable_set(EXTERNAL_CPU, ENABLED);
	if (ret)
	{
		dev_err(priv->dev, "rtk_cpuTag_enable_set failed, error %d\n",ret);
		return -1;
	}

	struct dsa_port *cpu_dp = NULL;
	struct dsa_port *dp;

	dsa_switch_for_each_port(dp, ds) {
		if (dsa_port_is_cpu(dp)) {
			cpu_dp = dp;
			break;
		}
	}

	if (!cpu_dp) {
		dev_err(priv->dev,"No CPU port found\n");
		return -ENODEV;
	}

	struct net_device *master_dev = cpu_dp->master;
    rtnl_lock();
    master_dev->wanted_features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
    master_dev->wanted_features &= ~NETIF_F_HW_CSUM;
    netdev_update_features(master_dev);
    rtnl_unlock();

    return 0;
}

static void rtl8372n_phylink_get_caps(struct dsa_switch *ds, int port,
				       struct phylink_config *config)
{
	struct rtl837x_priv *priv = ds->priv;

	if ((port == UTP_PORT3) || (port == UTP_PORT8)) {
		__set_bit(PHY_INTERFACE_MODE_10GKR, config->supported_interfaces);
		__set_bit(PHY_INTERFACE_MODE_10GBASER, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_5GBASER, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_USXGMII, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_1000BASEX, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_2500BASEX, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_TBI, config->supported_interfaces);
		config->mac_capabilities = MAC_10000FD | MAC_5000FD | MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
	} else {
		__set_bit(PHY_INTERFACE_MODE_INTERNAL, config->supported_interfaces);
		config->mac_capabilities = MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
	}
}

static rtk_sds_mode_t phy_interface_to_rtk_sds_mode(phy_interface_t interface)
{
	switch (interface)
	{
	case PHY_INTERFACE_MODE_10GBASER:
		return SERDES_10GR;
	case PHY_INTERFACE_MODE_USXGMII:
		return SERDES_10GUSXG;
	case PHY_INTERFACE_MODE_2500BASEX:
		return SERDES_2500BASEX;
	case PHY_INTERFACE_MODE_TBI:
		return SERDES_8221B;
	case PHY_INTERFACE_MODE_1000BASEX:
		return SERDES_SG;
	case PHY_INTERFACE_MODE_2500BASEX:
		return SERDES_HSG;
	default:
		return SERDES_10GR;
	}
}

static void rtl8372n_mac_link_up(struct dsa_switch *ds, int port, unsigned int mode,
                phy_interface_t interface, struct phy_device *phydev,
                int speed, int duplex, bool tx_pause, bool rx_pause)
{
	struct rtl837x_priv *priv = ds->priv;
	int ret;

	switch (port)
	{
	case UTP_PORT4:
	case UTP_PORT5:
	case UTP_PORT6:
	case UTP_PORT7:
		dev_info(priv->dev, "MAC link up on phy port(%d)\n", port);
		ret = 0;
		/* code */
		break;
	case UTP_PORT3:
		dev_info(priv->dev, "MAC link up on serdes port(%d) mode (%x)\n", 0, phy_interface_to_rtk_sds_mode(interface));
		ret = rtk_sdsMode_set(0, phy_interface_to_rtk_sds_mode(interface));
		break;
	case UTP_PORT8:
		dev_info(priv->dev, "MAC link up on serdes port(%d) mode (%x)\n", 1, phy_interface_to_rtk_sds_mode(interface));
		ret = rtk_sdsMode_set(1, phy_interface_to_rtk_sds_mode(interface));
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
	case UTP_PORT4:
	case UTP_PORT5:
	case UTP_PORT6:
	case UTP_PORT7:
		dev_info(priv->dev, "MAC link down on phy port (%d)\n", port);
		ret = 0;
		/* code */
		break;
	case UTP_PORT3:
		dev_info(priv->dev, "MAC link down on serdes port (%d)\n", 0);
		ret = rtk_sdsMode_set(0, SERDES_OFF);
		break;
	case UTP_PORT8:
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

// static int rtl8372n_port_enable(struct dsa_switch *ds, int port,
// 			     struct phy_device *phydev)
// {
// 	struct dsa_port *dp = dsa_to_port(ds, port);
// 	struct rtl837x_priv *priv = ds->priv;

// 	if (!dsa_port_is_user(dp))
// 		return 0;
// 	if(dsa_is_cpu_port(ds, port)){
// 		struct net_device *master = dsa_port_to_master(dp);
// 		dev_info(priv->dev, "Device %s\n", master->name);
// 	}
// 	return 0;
// }

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

    RTK_PORTMASK_PORT_SET(entry.untag, 3);

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

	// .port_enable			= rtl8372n_port_enable,
	// .port_vlan_filtering = rtl8372n_vlan_filtering,
	// .port_vlan_add = rtl8372n_vlan_add,
	// .port_vlan_del = rtl8372n_vlan_del,

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
