/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 StarField Xu <air_jinkela@163.com>
 */

#include <linux/uaccess.h>
#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/u64_stats_sync.h>

#include "./rtl837x.h"

#define TO_FOPS(name) _##name##_rw_fops

#define REGRWFUNC(name) \
	static char _buf_rd_##name[64];  \
	static ssize_t _##name##_rw_read(struct file *filep, char __user *ubuf,  \
				  size_t count, loff_t *offp)   \
	{   \
		return simple_read_from_buffer(ubuf, count, offp, _buf_rd_##name, strlen(_buf_rd_##name));   \
	}   \
	extern ssize_t _##name##_rw_write(struct file *filep, const char __user *ubuf,   \
				   size_t count, loff_t *offp);   \
	static const struct file_operations _##name##_rw_fops = {   \
		.owner = THIS_MODULE,   \
		.open = simple_open,   \
		.write = _##name##_rw_write,   \
		.read = _##name##_rw_read   \
	};

REGRWFUNC(reg)
REGRWFUNC(phyreg_mmd)
REGRWFUNC(sdsreg)

ssize_t _sdsreg_rw_write(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	uint32_t sds_id, page, reg, val;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %x %x %x", &sds_id, &page, &reg, &val) == -1)
			return -EFAULT;
		else{
			if (sds_id > 1)
				return -EFAULT;
			rtk_rtl8373_sds_reg_write(sds_id, page, reg, val);
		}
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x %x", &sds_id, &page, &reg) == -1)
			return -EFAULT;
		else {
			rtk_rtl8373_sds_reg_read(sds_id, page, reg, &val);
			snprintf(_buf_rd_sdsreg, 64, "sds_id: %d, page: 0x%08x, reg: 0x%08x, val: 0x%08x\n", sds_id, page, reg, val);
		}
	} else {
		snprintf(_buf_rd_sdsreg, 64, "echo \"w/r <sds_id> <page> <reg> [<val>]\" > sdsreg\n");
	}
	return count;
}

ssize_t _phyreg_mmd_rw_write(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	uint32_t port, devad, reg, val;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %x %x %x", &port, &devad, &reg, &val) == -1)
			return -EFAULT;
		else{
			if (port > 9)
				return -EFAULT;
			priv->ops->phy_write_c45(priv, port, devad, reg, val);
		}
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x %x", &port, &devad, &reg) == -1)
			return -EFAULT;
		else {
			val = priv->ops->phy_read_c45(priv, port, devad, reg);
			if (val < 0)
				return -EIO;
			snprintf(_buf_rd_phyreg_mmd, 64, "port: %d, devad: 0x%08x, reg: 0x%08x, val: 0x%08x\n", port, devad, reg, val);
		}
	} else {
		snprintf(_buf_rd_phyreg_mmd, 64, "echo \"w/r <real_port_index> <devad> <reg> [<val>]\" > phyreg_mmd\n");
	}
	return count;
}

ssize_t _reg_rw_write(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;
	uint32_t reg, val;
	if (*offp)
		return 0;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	if(buf[0] == 'w') {
		if(sscanf(buf, "w %x %x", &reg, &val) == -1)
			return -EFAULT;
		else
			regmap_write(priv->map, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %x", &reg) == -1)
			return -EFAULT;
		else {
			regmap_read(priv->map, reg, &val);
			snprintf(_buf_rd_reg, 64, "reg: 0x%08x, val: 0x%08x\n", reg, val);
		}
	} else {
		snprintf(_buf_rd_reg, 64, "echo \"w/r <reg> [<val>]\" > reg\n");
	}
	return count;
}

static ssize_t _sds_page_dump_read(struct file *filep, char __user *ubuf,
				size_t count, loff_t *offp)
{
	char buf[2048];
	struct seq_file *sfile;
	struct rtl837x_priv *priv;
	int len = 0;
	unsigned int v3;

	sfile = filep->private_data;
	priv = sfile->private;

	regmap_read(priv->map, RTL8373_SDS_MODE_SEL_ADDR, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "reg 0x7b20: %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x21, 0x10, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x21  reg 0x10; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x21, 0x13, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x21  reg 0x13; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x21, 0x18, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x21  reg 0x18; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x21, 0x1B, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x21  reg 0x1b; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x21, 0x1D, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x21  reg 0x1d; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x36, 0x1C, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x36  reg 0x1c; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x36, 0x14, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x36  reg 0x14; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x36, 0x10, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x36  reg 0x10; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 4, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x04; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 6, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x06; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 7, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x07; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 9, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x09; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0xB, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x0b; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0xC, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x0c; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0xD, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x0d; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0x15, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x15; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0x16, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x16; data = %#x\n", v3);
	rtk_rtl8373_sds_reg_read(0, 0x2E, 0x1D, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 0x2e  reg 0x1d; data = %#x\n", v3);

	rtk_rtl8373_sds_regbits_read(0, 5, 0, 1, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 5  reg 0; bit0 = %#x\n", v3);
	rtk_rtl8373_sds_regbits_read(0, 5, 1, 255, &v3);
	len += snprintf(buf + len, sizeof(buf)-len, "sds page 5  reg 1; bit7:0 = %#x\n", v3);

	return simple_read_from_buffer(ubuf, count, offp, buf, strlen(buf));
}

static const struct file_operations _sds_page_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = _sds_page_dump_read
};

int rtl837x_debug_proc_init(struct rtl837x_priv *priv)
{
	char name[64];
	snprintf(name, 64, "%s-ds", priv->dev->init_name);
	priv->debugfs_parent = debugfs_create_dir(name, NULL);
	debugfs_create_file("reg", 0600,
			priv->debugfs_parent, NULL,
			&TO_FOPS(reg));

	debugfs_create_file("phy_mmd", 0600,
		priv->debugfs_parent, NULL,
		&TO_FOPS(phyreg_mmd));

	debugfs_create_file("sdsreg", 0600,
		priv->debugfs_parent, NULL,
		&TO_FOPS(sdsreg));

	debugfs_create_file("sds_page_dump", 0400,
		priv->debugfs_parent, NULL,
		&_sds_page_dump_fops);

	return 0;
}

int rtl837x_debug_proc_deinit(struct rtl837x_priv *priv)
{
	debugfs_remove_recursive(priv->debugfs_parent);
	return 0;
}
