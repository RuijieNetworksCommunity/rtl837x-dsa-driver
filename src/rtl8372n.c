#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/regmap.h>
#include <linux/version.h>

#include "rtl837x.h"

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

	{ 16, 1, "ifOutDiscards"                    },
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

struct rtl8372n_pcs
{
	struct phylink_pcs pcs;
	struct rtl837x_priv *priv;
	int index;
};

struct rtl8372n {
	struct rtl8372n_pcs pcs[RTL8372N_NUM_PORTS];
};

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
            priv->num_ports = RTL8372N_NUM_PORTS;
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
	priv->pMapper = dal_rtl8373_mapper_get();
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
	uint32_t val_h, val_l;

    int mib_id = (mib->offset)/2;

	uint32_t tmp = (FIELD_PREP(RTL8373_INDIRECT_ACCESS_CTRL_PORT_ID_MASK, port) |
					FIELD_PREP(RTL8373_INDIRECT_ACCESS_CTRL_MIB_ID_MASK, mib_id) |
					FIELD_PREP(RTL8373_INDIRECT_ACCESS_CTRL_ACC_CMD_MASK, 1));

	ret = regmap_write(priv->map, RTL8373_INDIRECT_ACCESS_CTRL_ADDR, tmp);
    if(ret) return ret;

	ret = regmap_read_poll_timeout(priv->map, RTL8373_INDIRECT_ACCESS_CTRL_ADDR, tmp, ((tmp & RTL8373_INDIRECT_ACCESS_CTRL_ACC_CMD_MASK) == 0), 0, 1000);
    if(ret) return ret;

	if (mib->length > 1)
	{
		ret = regmap_read(priv->map, RTL8373_INDIRECT_ACCESS_CNT_L_ADDR, &val_l);
		if(ret) return ret;
		ret = regmap_read(priv->map, RTL8373_INDIRECT_ACCESS_CNT_H_ADDR, &val_h);
		if(ret) return ret;
		*mibvalue = ((uint64_t)val_l << 32) | val_h;
		return 0;
	} else
	{
		if(mib->offset % 2)
			return regmap_read(priv->map, RTL8373_INDIRECT_ACCESS_CNT_H_ADDR, (u32*)mibvalue);
		else
			return regmap_read(priv->map, RTL8373_INDIRECT_ACCESS_CNT_L_ADDR, (u32*)mibvalue);
	}
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
	int ret;
    u32 val, tmp;

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

static int rtl8372n_phy_write_c45(struct rtl837x_priv *priv, int phy, int devad, int regnum, u16 val)
{
	int ret;
    u32 tmp;

	ret = regmap_write(priv->map, RTL8373_SMI_ACCESS_PHY_CTRL_0_ADDR, BIT(phy));
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,12,44)
	if (!mnp)
		ds->user_mii_bus = bus;
#endif

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

static int rtl8372n_pcs_validate(struct phylink_pcs *pcs,
			       unsigned long *supported,
			       const struct phylink_link_state *state)
{
	return 0;
}

static void rtl8372n_sds_pcs_get_state(struct phylink_pcs *pcs,
				 struct phylink_link_state *state)
{
	struct rtl8372n_pcs *_pcs = container_of(pcs, struct rtl8372n_pcs, pcs);
	struct rtl837x_priv *priv = _pcs->priv;
	int port = _pcs->index;
	int ret;

	rtk_port_status_t port_status;
	ret = rtk_port_macStatus_get(port, &port_status);
	if(ret)
	{
		dev_err(priv->dev, "get port:%u MAC status Failed: %d", port, ret);
		return;
	}

	state->link = !!(port_status.link);
	state->an_complete = !!(port_status.link);
	state->duplex = !!(port_status.duplex);

	switch (port_status.speed) {
		case 0:
			state->speed = SPEED_10;
			break;
		case 1:
			state->speed = SPEED_100;
			break;
		case 2:
			state->speed = SPEED_1000;
			break;
		case 4:
			state->speed = SPEED_10000;
			break;
		case 5:
			state->speed = SPEED_2500;
			break;
		case 6:
			state->speed = SPEED_5000;
			break;
		default:
			state->speed = SPEED_UNKNOWN;
			break;
	}

	state->pause &= ~(MLO_PAUSE_RX | MLO_PAUSE_TX);
	if (port_status.rxpause)
		state->pause |= MLO_PAUSE_RX;
	if (port_status.txpause)
		state->pause |= MLO_PAUSE_TX;
}

static int rtl8372n_pcs_config(struct phylink_pcs *pcs, unsigned int neg_mode,
			     phy_interface_t interface,
			     const unsigned long *advertising,
			     bool permit_pause_to_mac)
{
	return 0;
}

static void rtl8372n_pcs_an_restart(struct phylink_pcs *pcs)
{
}

static const struct phylink_pcs_ops rtl8372n_sds_pcs_ops = {
	.pcs_validate = rtl8372n_pcs_validate,
	.pcs_get_state = rtl8372n_sds_pcs_get_state,
	.pcs_config = rtl8372n_pcs_config,
	.pcs_an_restart = rtl8372n_pcs_an_restart,
};
/*
struct phylink_pcs *rtl8372n_phylink_mac_select_pcs(struct phylink_config *config,
						phy_interface_t interface)
{
	struct dsa_port *dp = dsa_phylink_to_port(config);
	struct rtl837x_priv *priv = dp->ds->priv;
	struct rtl8372n *chip_data = priv->chip_data;

	if (dp->index != UTP_PORT3 && dp->index != UTP_PORT8)
		return NULL;
	return &(chip_data->pcs[dp->index].pcs);
}

void rtl8372n_phylink_mac_config(struct phylink_config *config, unsigned int mode,
			const struct phylink_link_state *state)
{
	struct dsa_port *dp = dsa_phylink_to_port(config);
	struct rtl837x_priv *priv = dp->ds->priv;
	int port = dp->index;

	// dev_info(priv->dev, "\n\ncalled rtl8372n_phylink_mac_config: port: %d, mode: %s\n\n\n", port, phy_modes(interface));

	if (port != UTP_PORT8 && port != UTP_PORT3)
		return;
	dev_info(priv->dev, "MAC config serdes port(%d) mode (%x)\n", 
			  port == UTP_PORT3 ? 0 : 1, 
			  phy_interface_to_rtk_sds_mode(state->interface));
	rtk_sdsMode_set(port == UTP_PORT3 ? 0 : 1, phy_interface_to_rtk_sds_mode(state->interface));
}

void rtl8372n_phylink_mac_link_down(struct phylink_config *config, unsigned int mode,
				phy_interface_t interface)
{
	struct dsa_port *dp = dsa_phylink_to_port(config);
	struct rtl837x_priv *priv = dp->ds->priv;
	int port = dp->index;
	int ret;

	switch (port)
	{
	case UTP_PORT4:
	case UTP_PORT5:
	case UTP_PORT6:
	case UTP_PORT7:
		dev_info(priv->dev, "MAC link down on phy port (%d)\n", port);
		ret = 0;
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

void rtl8372n_phylink_mac_link_up(struct phylink_config *config,
			struct phy_device *phy, unsigned int mode,
			phy_interface_t interface, int speed, int duplex,
			bool tx_pause, bool rx_pause)
{
	struct dsa_port *dp = dsa_phylink_to_port(config);
	struct rtl837x_priv *priv = dp->ds->priv;
	int port = dp->index;
	int ret = 0;

	switch (port)
	{
	case UTP_PORT4:
	case UTP_PORT5:
	case UTP_PORT6:
	case UTP_PORT7:
		dev_info(priv->dev, "MAC link up on phy port(%d)\n", port);
		break;
	case UTP_PORT3:
	case UTP_PORT8:
		dev_info(priv->dev, "MAC link up on serdes port(%d) mode (%x), speed (%d)\n", 
							port == UTP_PORT3 ? 0 : 1, 
							phy_interface_to_rtk_sds_mode(interface),
							speed);
		ret = rtk_sdsMode_set(port == UTP_PORT3 ? 0 : 1, phy_interface_to_rtk_sds_mode(interface));
		break;
	}

	if (ret) {
		dev_err(priv->dev, "failed to enable the port(%d)\n", port);
		return;
	}
}

static const struct phylink_mac_ops rtl8372n_phylink_mac_ops = {
	.mac_select_pcs	= rtl8372n_phylink_mac_select_pcs,
	.mac_config	= rtl8372n_phylink_mac_config,
	.mac_link_down	= rtl8372n_phylink_mac_link_down,
	.mac_link_up	= rtl8372n_phylink_mac_link_up,
};
*/

static int rtl8372n_setup(struct dsa_switch *ds)
{
    int ret;
    struct rtl837x_priv *priv = ds->priv;
	struct rtl8372n *chip_data = priv->chip_data;
	struct dsa_port *cpu_dp = NULL;
	struct dsa_port *dp;
    rtl_gbl_priv = priv;

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

	chip_data->pcs[3].pcs.ops = &rtl8372n_sds_pcs_ops;
	chip_data->pcs[3].pcs.neg_mode = true;
	chip_data->pcs[3].priv = priv;
	chip_data->pcs[3].index = 3;

	chip_data->pcs[8].pcs.ops = &rtl8372n_sds_pcs_ops;
	chip_data->pcs[8].pcs.neg_mode = true;
	chip_data->pcs[8].priv = priv;
	chip_data->pcs[8].index = 8;

    dev_info(priv->dev,"Start init RTL8372N Switch\n");

    ret = rtk_switch_init();
	if(ret){
		dev_err(priv->dev, "rtk_switch_init Fail, error:%d\n", ret);
		return -EIO;
	}
	if (priv->swap_cfg.sds0_rx_swap)
	{
		priv->pMapper->rtl8373_sds_regbits_write(0, 0, 0, 0x200, 1); //#SDS0RX PN swap
		priv->pMapper->rtl8373_sds_regbits_write(0, 6, 2, 0x2000, 1);
	}

	if (priv->swap_cfg.sds0_tx_swap)
	{
		priv->pMapper->rtl8373_sds_regbits_write(0, 0, 0, 1 << 8, 1); //#SDS0RTX PN swap
		priv->pMapper->rtl8373_sds_regbits_write(0, 6, 2, 1 << 14, 1);
	}

	if (priv->swap_cfg.sds1_rx_swap)
	{
		priv->pMapper->rtl8373_sds_regbits_write(1, 0, 0, 0x200, 1); //#SDS1RX PN swap
		priv->pMapper->rtl8373_sds_regbits_write(1, 6, 2, 0x2000, 1);
	}

	if (priv->swap_cfg.sds1_tx_swap)
	{
		priv->pMapper->rtl8373_sds_regbits_write(1, 0, 0, 1 << 8, 1); //#SDS1TX PN swap
		priv->pMapper->rtl8373_sds_regbits_write(1, 6, 2, 1 << 14, 1);
	}

    // ##MDI reverse configuration for Demo Tap UP RJ45, RTL8366U/RTL8373N/RTL8372N
	if (priv->swap_cfg.phy_mdi_reverse){
		priv->pMapper->rtl8373_setAsicRegBits(RTL8373_CFG_PHY_MDI_REVERSE_ADDR, 0xF, 0xC);
	}

	if (priv->swap_cfg.phy_tx_polarity_swap)
	{
    	priv->pMapper->rtl8373_setAsicRegBits(RTL8373_CFG_PHY_TX_POLARITY_SWAP_ADDR, 0xFFFF, 0x596A); //#TX_POLARITY_SWAP
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
		if(port == cpu_dp->index) continue;

		rtk_port_t isolation_port_mask = BIT(cpu_dp->index);

		ret = rtk_port_isolation_set(port, isolation_port_mask);
		if (ret) {
			dev_err(priv->dev, "port: %d rtk_port_isolation_set configure failed, error: %d\n", port, ret);
			return -EIO;
		}
	}

	ret = rtk_port_isolation_set(cpu_dp->index, dsa_user_ports(ds));
	if (ret) {
		dev_err(priv->dev, "port: %d rtk_port_isolation_set configure failed, error: %d\n", cpu_dp->index, ret);
		return -EIO;
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

	ret = rtk_cpu_externalCpuPort_set(cpu_dp->index);
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

	struct net_device *master_dev = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,12,44)
	master_dev = cpu_dp->master;
#else
	master_dev = cpu_dp->conduit;
#endif

	if (!master_dev)
	{
		dev_err(priv->dev, "cannot get master netdev from cpu port\n");
		return -ENODEV;
	}

    rtnl_lock();
    master_dev->wanted_features &= ~(NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
    master_dev->wanted_features &= ~NETIF_F_HW_CSUM;
    netdev_update_features(master_dev);
    rtnl_unlock();

    return 0;
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

static int
rtl8372n_port_bridge_join(struct dsa_switch *ds, int port,
			   struct dsa_bridge bridge,
			   bool *tx_fwd_offload,
			   struct netlink_ext_ack *extack)
{
    struct rtl837x_priv *priv = ds->priv;
	unsigned int port_bitmap = 0;
	int ret, i;
	dev_info(priv->dev, "port_bridge_join %d\n", port);

	/* Loop over all other ports than the current one */
	for (i = 0; i < priv->num_ports; i++) {
		/* Current port handled last */
		if (i == port)
			continue;
		/* Not on this bridge */
		if (!dsa_port_offloads_bridge(dsa_to_port(ds, i), &bridge))
			continue;
		/* Join this port to each other port on the bridge */
		ret = regmap_update_bits(priv->map, 
				  RTL8373_PORT_ISO_PORT_PMSK_ADDR(i),
				  BIT(port),
				  BIT(port));
		if (ret)
			dev_err(priv->dev, "failed to join port %d\n", port);

		port_bitmap |= BIT(i);
	}

	/* Set the bits for the ports we can access */
	ret = regmap_update_bits(priv->map, 
			RTL8373_PORT_ISO_PORT_PMSK_ADDR(port),
			port_bitmap,
			port_bitmap);
	return ret;
}

static void
rtl8372n_port_bridge_leave(struct dsa_switch *ds, int port,
			    struct dsa_bridge bridge)
{
    struct rtl837x_priv *priv = ds->priv;
	unsigned int port_bitmap = 0;
	int ret, i;
	dev_info(priv->dev, "port_bridge_leave %d\n", port);

	/* Loop over all other ports than this one */
	for (i = 0; i < priv->num_ports; i++) {
		/* Current port handled last */
		if (i == port)
			continue;
		/* Not on this bridge */
		if (!dsa_port_offloads_bridge(dsa_to_port(ds, i), &bridge))
			continue;
		/* Remove this port from any other port on the bridge */
		ret = regmap_update_bits(priv->map, RTL8373_PORT_ISO_PORT_PMSK_ADDR(i),
					 BIT(port), 0);
		if (ret)
			dev_err(priv->dev, "failed to leave port %d\n", port);

		port_bitmap |= BIT(i);
	}

	/* Clear the bits for the ports we can not access, leave ourselves */
	regmap_update_bits(priv->map, RTL8373_PORT_ISO_PORT_PMSK_ADDR(port),
			   port_bitmap, 0);
}

static int rtl8372n_port_enable(struct dsa_switch *ds, int port,
			       struct phy_device *phy)
{
    struct rtl837x_priv *priv = ds->priv;
	int ret;
	dev_info(priv->dev, "port_enable %d\n", port);

	ret = priv->pMapper->fMdrv_miim_mmd_write(BIT(port), 31, 0xa610, 0x2058);
	if (ret != RT_ERR_OK)
		return -EIO;

	return 0;
}

static void rtl8372n_port_disable(struct dsa_switch *ds, int port)
{
    struct rtl837x_priv *priv = ds->priv;
	dev_info(priv->dev, "port_disable %d\n", port);

	priv->pMapper->fMdrv_miim_mmd_write(BIT(port), 31, 0xa610, 0x2858);
}

static void rtl8372n_phylink_mac_config(struct dsa_switch *ds, int port,
					unsigned int mode,
					const struct phylink_link_state *state)
{
    struct rtl837x_priv *priv = ds->priv;

	if (port != UTP_PORT8 && port != UTP_PORT3)
		return;
	dev_info(priv->dev, "MAC config serdes port(%d) mode (%x)\n", 
			  port == UTP_PORT3 ? 0 : 1, 
			  phy_interface_to_rtk_sds_mode(state->interface));
	rtk_sdsMode_set(port == UTP_PORT3 ? 0 : 1, phy_interface_to_rtk_sds_mode(state->interface));
}

static struct phylink_pcs *rtl8372n_phylink_mac_select_pcs(struct dsa_switch *ds, int port,
			     phy_interface_t interface)
{
    struct rtl837x_priv *priv = ds->priv;
	struct rtl8372n *chip_data = priv->chip_data;
	
	if (port != UTP_PORT3 && port != UTP_PORT8)
		return NULL;
	return &(chip_data->pcs[port].pcs);
}

static void rtl8372n_phylink_get_caps(struct dsa_switch *ds, int port,
				       struct phylink_config *config)
{
	if ((port == UTP_PORT3) || (port == UTP_PORT8)) {
		__set_bit(PHY_INTERFACE_MODE_10GKR, config->supported_interfaces);
		__set_bit(PHY_INTERFACE_MODE_10GBASER, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_5GBASER, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_USXGMII, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_1000BASEX, config->supported_interfaces);
        __set_bit(PHY_INTERFACE_MODE_2500BASEX, config->supported_interfaces);

		config->mac_capabilities = MAC_10000FD | MAC_5000FD | MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
	} else {
		__set_bit(PHY_INTERFACE_MODE_INTERNAL, config->supported_interfaces);
		config->mac_capabilities = MAC_2500FD | MAC_1000 | MAC_100 | MAC_10 |
                                    MAC_SYM_PAUSE | MAC_ASYM_PAUSE;
	}
}

static void rtl8372n_phylink_mac_link_up(struct dsa_switch *ds, int port, unsigned int mode,
                phy_interface_t interface, struct phy_device *phydev,
                int speed, int duplex, bool tx_pause, bool rx_pause)
{
	struct rtl837x_priv *priv = ds->priv;
	int ret = 0;

	switch (port)
	{
	case UTP_PORT4:
	case UTP_PORT5:
	case UTP_PORT6:
	case UTP_PORT7:
		dev_info(priv->dev, "MAC link up on phy port(%d)\n", port);
		break;
	case UTP_PORT3:
	case UTP_PORT8:
		dev_info(priv->dev, "MAC link up on serdes port(%d) mode (%x), speed (%d)\n", 
							port == UTP_PORT3 ? 0 : 1, 
							phy_interface_to_rtk_sds_mode(interface),
							speed);
		ret = rtk_sdsMode_set(port == UTP_PORT3 ? 0 : 1, phy_interface_to_rtk_sds_mode(interface));
		break;
	}

	if (ret) {
		dev_err(priv->dev, "failed to enable the port(%d)\n", port);
		return;
	}
}

static void rtl8372n_phylink_mac_link_down(struct dsa_switch *ds, int port, unsigned int mode,
			phy_interface_t interface)
{
	struct rtl837x_priv *priv = ds->priv;
	int ret = 0;

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
		/* todo */
		// ret = rtk_sdsMode_set(0, SERDES_OFF);
		break;
	case UTP_PORT8:
		dev_info(priv->dev, "MAC link down on serdes port (%d)\n", 1);
		/* todo */
		// ret = rtk_sdsMode_set(1, SERDES_OFF);
		break;
	}

	if (ret) {
		dev_err(priv->dev, "failed to disable the port(%d)\n", port);
		return;
	}
}

static const struct dsa_switch_ops rtl8372n_switch_ops_mdio = {
	.get_tag_protocol = rtl8372n_get_tag_protocol,
	.setup = rtl8372n_setup,

	.phylink_mac_select_pcs = rtl8372n_phylink_mac_select_pcs,
	.phylink_mac_config = rtl8372n_phylink_mac_config,
	.phylink_get_caps = rtl8372n_phylink_get_caps,
	.phylink_mac_link_up = rtl8372n_phylink_mac_link_up,
	.phylink_mac_link_down = rtl8372n_phylink_mac_link_down,

	.get_strings = rtl8372n_get_strings,
	.get_ethtool_stats = rtl8372n_get_ethtool_stats,
	.get_sset_count = rtl8372n_get_sset_count,

	// .port_vlan_filtering = rtl8372n_vlan_filtering,
	// .port_vlan_add = rtl8372n_vlan_add,
	// .port_vlan_del = rtl8372n_vlan_del,

	.port_bridge_join = rtl8372n_port_bridge_join,
	.port_bridge_leave = rtl8372n_port_bridge_leave,

	.port_enable = rtl8372n_port_enable,
	.port_disable = rtl8372n_port_disable
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
	// .phy_mac_ops = &rtl8372n_phylink_mac_ops,
	.chip_data_sz = sizeof(struct rtl8372n),
};
EXPORT_SYMBOL_GPL(rtl8372n_variant);
