/* SPDX-License-Identifier: GPL-2.0-or-later */
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

#define MAKE_WRITE_FUNCNAME(_name) _##_name##_rw_write
#define MAKE_WRITE_BUFNAME(_name) _buf_rd_##_name

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
		.write = MAKE_WRITE_FUNCNAME(name),   \
		.read = _##name##_rw_read   \
	};

REGRWFUNC(vlan, 128)
REGRWFUNC(pvid, 64)
REGRWFUNC(reg, 64)
REGRWFUNC(phyreg_mmd, 64)
REGRWFUNC(phyreg_mii, 64)
REGRWFUNC(phyreg_ocp, 64)
REGRWFUNC(sdsreg, 64)
REGRWFUNC(l2uc, 128)

ssize_t MAKE_WRITE_FUNCNAME(vlan)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	u32 vlan_id;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'r') {
		if(sscanf(buf, "r %d", &vlan_id) != 1) {
			kfree(buf);
			return -EFAULT;
		} else {
			struct rtl837x_vlan_4k vlan4k;
			memset(&vlan4k, 0, sizeof(vlan4k));
			priv->ops->get_vlan_4k(priv, vlan_id,  &vlan4k);
			snprintf(MAKE_WRITE_BUFNAME(vlan), 128, "vid: %d, mbr: 0x%04X, utag: 0x%04X, fid: %d\n",
						  vlan4k.vid, vlan4k.member, vlan4k.untag, vlan4k.fid);
		}
	} else {
		snprintf(MAKE_WRITE_BUFNAME(vlan), 128, "echo \"r <vlan_id>\" > vlan_dump\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(pvid)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	int ret;
	u32 port, pvid;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %d", &port, &pvid) != 2) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 8) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_reg_bits_write(priv, RTL8373_VLAN_PORT_PB_VLAN_ADDR(port),
				  RTL8373_VLAN_PORT_PB_VLAN_PVID_MASK(port), pvid);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d", &port) != 1) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 8) {
			kfree(buf);
			return -EFAULT;
		}
		ret = rtl837x_reg_bits_read(priv, RTL8373_VLAN_PORT_PB_VLAN_ADDR(port),
				  RTL8373_VLAN_PORT_PB_VLAN_PVID_MASK(port), &pvid);
		if (ret) {
			kfree(buf);
			return -EIO;
		}
		snprintf(MAKE_WRITE_BUFNAME(pvid), 64, "port: %d, pvid: %d\n", port, pvid);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(pvid), 64, "echo \"w/r <port> [<pvid>]\" > pvid\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(sdsreg)(struct file *filep, const char __user *ubuf,
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
		if(sscanf(buf, "w %d %x %x %x", &sds_id, &page, &reg, &val) != 4) {
			kfree(buf);
			return -EFAULT;
		}
		if (sds_id > 1) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_sds_reg_write(priv, sds_id, page, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x %x", &sds_id, &page, &reg) != 3) {
			kfree(buf);
			return -EFAULT;
		}
		if (sds_id > 1) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_sds_reg_read(priv, sds_id, page, reg, &tmp16);
		snprintf(MAKE_WRITE_BUFNAME(sdsreg), 64, "sds_id: %d, page: 0x%08x, reg: 0x%08x, val: 0x%08x\n", sds_id, page, reg, tmp16);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(sdsreg), 64, "echo \"w/r <sds_id> <page> <reg> [<val>]\" > sdsreg\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(phyreg_mmd)(struct file *filep, const char __user *ubuf,
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
		if(sscanf(buf, "w %d %x %x %x", &port, &devad, &reg, &val) != 4) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		priv->ops->phy_write_c45(priv, port, devad, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x %x", &port, &devad, &reg) != 3) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		if (priv->ops->phy_read_c45(priv, port, devad, reg, &tmp16)) {
			kfree(buf);
			return -EIO;
		}
		snprintf(MAKE_WRITE_BUFNAME(phyreg_mmd), 64, "port: %d, devad: 0x%08x, reg: 0x%08x, val: 0x%08x\n", port, devad, reg, tmp16);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(phyreg_mmd), 64, "echo \"w/r <real_port_index> <devad> <reg> [<val>]\" > phyreg_mmd\n");
	}
	kfree(buf);
	return count;
}


ssize_t MAKE_WRITE_FUNCNAME(phyreg_mii)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	u32 port, reg, val;
	u16 tmp16;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %x %x", &port, &reg, &val) != 3) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_phy_write_c22(priv, port, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x", &port, &reg) != 2) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		if (rtl837x_phy_read_c22(priv, port, reg, &tmp16)) {
			kfree(buf);
			return -EIO;
		}
		snprintf(MAKE_WRITE_BUFNAME(phyreg_mii), 64, "port: %d, reg: 0x%08x, val: 0x%08x\n", port, reg, tmp16);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(phyreg_mii), 64, "echo \"w/r <real_port_index> <reg> [<val>]\" > phyreg_mii\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(phyreg_ocp)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	u32 port, reg, val;
	u16 tmp16;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);
	
	if(buf[0] == 'w') {
		if(sscanf(buf, "w %d %x %x", &port, &reg, &val) != 3) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_phy_write_c22(priv, port, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %x", &port, &reg) != 2) {
			kfree(buf);
			return -EFAULT;
		}
		if (port > 9) {
			kfree(buf);
			return -EFAULT;
		}
		if (rtl837x_phy_read_c22(priv, port, reg, &tmp16)) {
			kfree(buf);
			return -EIO;
		}
		snprintf(MAKE_WRITE_BUFNAME(phyreg_ocp), 64, "port: %d, reg: 0x%08x, val: 0x%08x\n", port, reg, tmp16);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(phyreg_ocp), 64, "echo \"w/r <real_port_index> <reg> [<val>]\" > phyreg_ocp\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(reg)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	char *buf;
	u32 reg, val;
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
		if(sscanf(buf, "w %x %x", &reg, &val) != 2) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_reg_write(priv, reg, val);
	} else if(buf[0] == 'r') {
		if(sscanf(buf, "r %x", &reg) != 1) {
			kfree(buf);
			return -EFAULT;
		}
		rtl837x_reg_read(priv, reg, &val);
		snprintf(MAKE_WRITE_BUFNAME(reg), 64, "reg: 0x%08x, val: 0x%08x\n", reg, val);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(reg), 64, "echo \"w/r <reg> [<val>]\" > reg\n");
	}
	kfree(buf);
	return count;
}

ssize_t MAKE_WRITE_FUNCNAME(l2uc)(struct file *filep, const char __user *ubuf,
				   size_t count, loff_t *offp)
{
	int ret, len = 0;
	char *buf;
	u32 method, index;
	struct seq_file *sfile;
	struct rtl837x_lut_entry entry = {0};
	struct rtl837x_priv *priv;
	if (*offp)
		return 0;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	if(buf[0] == 'r') {
		if(sscanf(buf, "r %d %d", &method, &index) != 2) {
			kfree(buf);
			return -EFAULT;
		}
		entry.addr = index;
		ret = rtl837x_lut_query(priv, method, &entry);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "result: %s%d ", ret==0?"ok  ":"no  ", ret);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "type: %s ", entry.type==LUT_TYPE_L2_UC?"l2uc":(entry.type==LUT_TYPE_L2_MC?"l2mc":"l3"));
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "addr: %d ", entry.addr);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "%02X:%02X:%02X:%02X:%02X:%02X ", 
											  entry.uc.key.mac_addr[0],
											  entry.uc.key.mac_addr[1],
											  entry.uc.key.mac_addr[2],
											  entry.uc.key.mac_addr[3],
											  entry.uc.key.mac_addr[4],
											  entry.uc.key.mac_addr[5]
											);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "vid_fid: %d ", entry.uc.key.vid_fid);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "ivl: %d ", entry.uc.key.ivl);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "auth: %d ", entry.uc.auth);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "is_static: %d ", entry.uc.is_static);
		len += snprintf(MAKE_WRITE_BUFNAME(l2uc)+len, 128-len, "l3lookup: %d\n", entry.uc.l3lookup);
	} else {
		snprintf(MAKE_WRITE_BUFNAME(l2uc), 128, "echo \"r <read_method> [<index>]\" > l2uc\n");
	}
	kfree(buf);
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

#define SDS_DUMP_APPEND(fmt, ...) do { \
		if (len < SDS_DUMP_BUF_SIZE) { \
			int _n = snprintf(buf + len, SDS_DUMP_BUF_SIZE - len, \
					  fmt, ##__VA_ARGS__); \
			if (_n > 0) \
				len += _n; \
			if (len >= SDS_DUMP_BUF_SIZE) \
				len = SDS_DUMP_BUF_SIZE - 1; \
		} \
	} while (0)

	rtl837x_reg_read(priv, RTL8373_SDS_MODE_SEL_ADDR, &tmp32);
	SDS_DUMP_APPEND("reg 0x7b20: %#08x\n", tmp32);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x10, &tmp16);
	SDS_DUMP_APPEND("sds page 0x21  reg 0x10; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x13, &tmp16);
	SDS_DUMP_APPEND("sds page 0x21  reg 0x13; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x18, &tmp16);
	SDS_DUMP_APPEND("sds page 0x21  reg 0x18; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x1B, &tmp16);
	SDS_DUMP_APPEND("sds page 0x21  reg 0x1b; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x21, 0x1D, &tmp16);
	SDS_DUMP_APPEND("sds page 0x21  reg 0x1d; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x1C, &tmp16);
	SDS_DUMP_APPEND("sds page 0x36  reg 0x1c; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x14, &tmp16);
	SDS_DUMP_APPEND("sds page 0x36  reg 0x14; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x36, 0x10, &tmp16);
	SDS_DUMP_APPEND("sds page 0x36  reg 0x10; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 4, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x04; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 6, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x06; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 7, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x07; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 9, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x09; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xB, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x0b; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xC, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x0c; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0xD, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x0d; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x15, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x15; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x16, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x16; data = 0x%04x\n", tmp16);
	rtl837x_sds_reg_read(priv, 0, 0x2E, 0x1D, &tmp16);
	SDS_DUMP_APPEND("sds page 0x2e  reg 0x1d; data = 0x%04x\n", tmp16);

	rtl837x_sds_reg_read(priv, 0, 0x05, 0x00, &tmp16);
	SDS_DUMP_APPEND("sds page 0x05  reg 0x00; data = 0x%04x\n", tmp16);

#undef SDS_DUMP_APPEND

	ret = simple_read_from_buffer(ubuf, count, offp, buf, strlen(buf));
	kfree(buf);
	return ret;
}

static const struct file_operations _sds_page_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_debugfs_open,
	.read = _sds_page_dump_read
};

static ssize_t _vlan_dump_read(struct file *filep, char __user *ubuf,
			       size_t count, loff_t *offp)
{
	int ret, len = 0;
	char *buf;
	u32 vid;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;
	struct rtl837x_vlan_4k vlan4k;

	sfile = filep->private_data;
	priv = sfile->private;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (vid = 0; vid <= 4095; vid++) {
		memset(&vlan4k, 0, sizeof(vlan4k));
		ret = priv->ops->get_vlan_4k(priv, vid, &vlan4k);
		if (ret)
			continue;

		if (vlan4k.member == 0)
			continue;

		len += scnprintf(buf + len, PAGE_SIZE - len,
				 "vid: %d, mbr: 0x%04X, untag: 0x%04X, fid: %d\n",
				 vlan4k.vid, vlan4k.member, vlan4k.untag, vlan4k.fid);

		if (len >= PAGE_SIZE - 64)
			break;
	}

	ret = simple_read_from_buffer(ubuf, count, offp, buf, len);
	kfree(buf);
	return ret;
}

static const struct file_operations _vlan_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_debugfs_open,
	.read = _vlan_dump_read
};


static ssize_t _l2uc_dump_read(struct file *filep, char __user *ubuf,
			       size_t count, loff_t *offp)
{
	int ret, len = 0;
	char *buf;
	struct seq_file *sfile;
	struct rtl837x_priv *priv;
	struct rtl837x_lut_entry entry = {0};

	sfile = filep->private_data;
	priv = sfile->private;

#define L2UC_BUF_SIZE PAGE_SIZE*64

	buf = kmalloc(L2UC_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

#define L2UC_DUMP_APPEND(fmt, ...) do { \
		if (len < L2UC_BUF_SIZE) { \
			int _n = snprintf(buf + len, L2UC_BUF_SIZE - len, \
					  fmt, ##__VA_ARGS__); \
			if (_n > 0) \
				len += _n; \
			if (len >= L2UC_BUF_SIZE) \
				len = L2UC_BUF_SIZE - 1; \
		} \
	} while (0)

	for (int i = 0; i < 4160; i++) {
		entry.addr = i;
		ret = rtl837x_lut_query(priv, LUT_READ_METHOD_ADDRESS, &entry);
		if (ret)
			continue;
		L2UC_DUMP_APPEND("result: %s%d ", ret==0?"ok  ":"no  ", ret);
		L2UC_DUMP_APPEND("type: %s ", entry.type==LUT_TYPE_L2_UC?"l2uc":(entry.type==LUT_TYPE_L2_MC?"l2mc":"l3"));
		L2UC_DUMP_APPEND("addr: %d ", entry.addr);
		L2UC_DUMP_APPEND("%02X:%02X:%02X:%02X:%02X:%02X ", 
											  entry.uc.key.mac_addr[0],
											  entry.uc.key.mac_addr[1],
											  entry.uc.key.mac_addr[2],
											  entry.uc.key.mac_addr[3],
											  entry.uc.key.mac_addr[4],
											  entry.uc.key.mac_addr[5]
											);
		L2UC_DUMP_APPEND("vid_fid: %d ", entry.uc.key.vid_fid);
		L2UC_DUMP_APPEND("ivl: %d ", entry.uc.key.ivl);
		L2UC_DUMP_APPEND("auth: %d ", entry.uc.auth);
		L2UC_DUMP_APPEND("is_static: %d ", entry.uc.is_static);
		L2UC_DUMP_APPEND("l3lookup: %d\n", entry.uc.l3lookup);

		if (len >= L2UC_BUF_SIZE - 64)
			break;
	}

	ret = simple_read_from_buffer(ubuf, count, offp, buf, len);
	kfree(buf);
	return ret;
}

static const struct file_operations _l2uc_dump_fops = {
	.owner = THIS_MODULE,
	.open = simple_debugfs_open,
	.read = _l2uc_dump_read
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

	debugfs_create_file("phy_mii", 0600,
		priv->debugfs_parent, priv,
		&TO_FOPS(phyreg_mii));

	debugfs_create_file("phy_ocp", 0600,
		priv->debugfs_parent, priv,
		&TO_FOPS(phyreg_ocp));

	debugfs_create_file("sdsreg", 0600,
		priv->debugfs_parent, priv,
		&TO_FOPS(sdsreg));

	debugfs_create_file("vlan", 0400,
		priv->debugfs_parent, priv,
		&TO_FOPS(vlan));

	debugfs_create_file("pvid", 0400,
		priv->debugfs_parent, priv,
		&TO_FOPS(pvid));

	debugfs_create_file("l2uc", 0400,
		priv->debugfs_parent, priv,
		&TO_FOPS(l2uc));

	debugfs_create_file("l2uc_dump", 0400,
		priv->debugfs_parent, priv,
		&_l2uc_dump_fops);

	debugfs_create_file("vlan_dump", 0400,
		priv->debugfs_parent, priv,
		&_vlan_dump_fops);

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
