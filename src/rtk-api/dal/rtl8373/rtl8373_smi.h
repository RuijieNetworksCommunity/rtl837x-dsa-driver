#ifndef __SMI_H__
#define __SMI_H__

#include <rtk_types.h>
#include "rtk_error.h"

#define MDC_MDIO_CTRL_REG           21
#define MDC_MDIO_ADDR_REG           22
#define MDC_MDIO_DATA_LOW           23
#define MDC_MDIO_DATA_HIGH          24
#define MDC_MDIO_READ_CMD           0x1B
#define MDC_MDIO_WRITE_CMD          0x19

#include "../../../rtl837x.h"
// #include <rtl837x.h>

extern struct rtl837x_priv *rtl_gbl_priv;

rtk_int32 rtl8373_smi_read(rtk_uint32 mAddrs, rtk_uint32 *rData);
rtk_int32 rtl8373_smi_write(rtk_uint32 mAddrs, rtk_uint32 rData);

#endif /* __SMI_H__ */

