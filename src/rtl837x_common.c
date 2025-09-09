#include <linux/bitops.h>
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_irq.h>
#include <linux/regmap.h>

#include "./rtl837x.h"

#include <linux/printk.h>

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