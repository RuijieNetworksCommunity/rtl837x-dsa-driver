/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2025 StarField Xu <air_jinkela@163.com>
 */

#include <linux/debugfs.h>
#include <linux/regmap.h>

#include "./rtl837x.h"

#define TO_FOPS(name) _##name##_rw_fops

static int simple_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}

#define REGRWFUNC(name, BUF_SIZE) \
	static char _buf_rd_##name[BUF_SIZE];  \
	static ssize_t _##name##_rw_read(struct file *filep, char __user *ubuf,  \
				  size_t count, loff_t *offp)   \
	{   \
		return simple_read_from_buffer(ubuf, count, offp, _buf_rd_##name, strlen(_buf_rd_##name));   \
	}   \
	extern ssize_t _##name##_rw_write(struct file *filep, const char __user *ubuf,   \
				   size_t count, loff_t *offp);   \
	static const struct file_operations _##name##_rw_fops = {   \
		.owner = THIS_MODULE,   \
		.open = simple_debugfs_open,   \
		.write = _##name##_rw_write,   \
		.read = _##name##_rw_read   \
	};

REGRWFUNC(vlan, 128)
REGRWFUNC(reg, 64)
REGRWFUNC(phyreg_mmd, 64)
REGRWFUNC(sdsreg, 64)

ssize_t _vlan_rw_write(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	uint32_t vlan_id;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'r') {
		if(sscanf(buf, "r %d", &vlan_id) == -1)
			return -EFAULT;
		else {
			struct rtl837x_vlan_4k vlan4k;
			memset(&vlan4k, 0, sizeof(vlan4k));
			priv->ops->get_vlan_4k(priv,vlan_id,  &vlan4k);
			snprintf(_buf_rd_vlan, 128, "vid: %d, mbr: 0x%04X, utag: 0x%04X, fid: %d\n",
						  vlan4k.vid, vlan4k.member, vlan4k.untag, vlan4k.fid);
		}
	} else {
		snprintf(_buf_rd_vlan, 128, "echo \"r <vlan_id>\" > vlan_dump\n");
	}
	return count;
}

ssize_t _sdsreg_rw_write(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	u32 sds_id, page, reg, val;
	u16 tmp16;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %x %x %x", &sds_id, &page, &reg, &val) == -1)
			return -EFAULT;
		else{
			if (sds_id > 1)
				return -EFAULT;
			rtl837x_sds_reg_write(priv, sds_id, page, reg, val);
		}
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x %x", &sds_id, &page, &reg) == -1)
			return -EFAULT;
		else {
			rtl837x_sds_reg_read(priv, sds_id, page, reg, &tmp16);
			snprintf(_buf_rd_sdsreg, 64, "sds_id: %d, page: 0x%08x, reg: 0x%08x, val: 0x%08x\n", sds_id, page, reg, tmp16);
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
	u32 port, devad, reg, val;
	u16 tmp16;
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
			if (priv->ops->phy_read_c45(priv, port, devad, reg, &tmp16))
				return -EIO;
			snprintf(_buf_rd_phyreg_mmd, 64, "port: %d, devad: 0x%08x, reg: 0x%08x, val: 0x%08x\n", port, devad, reg, tmp16);
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
	uint32_t reg, val;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;
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
	int ret, len = 0;
#define SDS_DUMP_BUF_SIZE 2048
	char *buf;
	u16 tmp16;
	u32 tmp32;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = kmalloc(SDS_DUMP_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	regmap_read(priv->map, RTL8373_SDS_MODE_SEL_ADDR, &tmp32);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "reg 0x7b20: %#08x\n", tmp32);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x10, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x21  reg 0x10; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x13, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x21  reg 0x13; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x18, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x21  reg 0x18; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x1B, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x21  reg 0x1b; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x1D, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x21  reg 0x1d; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x1C, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x36  reg 0x1c; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x14, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x36  reg 0x14; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x10, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x36  reg 0x10; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 4, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x04; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 6, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x06; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 7, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x07; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 9, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x09; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xB, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x0b; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xC, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x0c; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xD, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x0d; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x15, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x15; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x16, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x16; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x1D, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x2e  reg 0x1d; data = 0x%04x\n", tmp16);

	rtl837x_sds_reg_read(priv, 0, 0x05, 0x00, &tmp16);
	len += snprintf(buf + len, SDS_DUMP_BUF_SIZE-len, "sds page 0x05  reg 0x00; data = 0x%04x\n", tmp16);

	ret = simple_read_from_buffer(ubuf, count, offp, buf, strlen(buf));
	kfree(buf);
	return ret;
}

static const struct file_operations _sds_page_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_debugfs_open,
	.read = _sds_page_dump_read
};

int rtl837x_debug_proc_init(struct rtl837x_priv *priv)
{
	char name[64];
	snprintf(name, 64, "rtl837x-%d-ds", priv->ds->index);
	priv->debugfs_parent = debugfs_create_dir(name, NULL);
	debugfs_create_file("reg", 0600,
			priv->debugfs_parent, priv,
			&TO_FOPS(reg));

	debugfs_create_file("phy_mmd", 0600,
		priv->debugfs_parent, priv,
		&TO_FOPS(phyreg_mmd));

	debugfs_create_file("sdsreg", 0600,
		priv->debugfs_parent, priv,
		&TO_FOPS(sdsreg));

	debugfs_create_file("vlan_dump", 0400,
		priv->debugfs_parent, priv,
		&TO_FOPS(vlan));

	debugfs_create_file("sds_page_dump", 0400,
		priv->debugfs_parent, priv,
		&_sds_page_dump_fops);

	return 0;
}

int rtl837x_debug_proc_deinit(struct rtl837x_priv *priv)
{
	debugfs_remove_recursive(priv->debugfs_parent);
	return 0;
}
