/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2025 StarField Xu <air_jinkela@163.com>
 */
#include <linux/version.h>
#include <linux/gpio/driver.h>

#include "rtl837x.h"

struct rtl837x_gpio 
{
	struct rtl837x_priv *priv;
	struct gpio_chip gp;
};

// I think this can be rewrite into linux pinctrl
// But there is no more info about the gpio mux or gpio func in the chip datasheet
// So just use gpio mode to simulate i2c or mdio


static int rtl837x_gpio_request(struct gpio_chip *gc, unsigned int offset)
{
	int ret = 0;
	struct rtl837x_gpio *gpio = gpiochip_get_data(gc);
	struct rtl837x_priv *priv = gpio->priv;

	switch (offset)
	{
		case 0 ... 29:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_0_ADDR,
					 BIT(offset), 1);
			break;
		case 30:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_2_ADDR,
					 RTL8373_IO_MUX_SEL_2_ACL_BIT3_EN_MASK, 1);
			break;
		case 31 ... 32:
			// we have to set RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_0_MASK and RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_1_MASK
			// only set RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_0_MASK or RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_1_MASK is not work
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_0_MASK | RTL8373_IO_MUX_SEL_1_PAD_UART0_SEL_1_MASK, 3);
			break;
		case 33 ... 35:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 BIT(offset-31), 1);
			break;
		case 36:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 RTL8373_IO_MUX_SEL_1_GPIO_PWM_OUT_SEL_MASK, 1);
			break;
		case 37 ... 38:
			break; //do nothing
		case 39:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 RTL8373_IO_MUX_SEL_1_GPIO_SDA4_SEL_MASK, 1);
			break;
		case 40:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 RTL8373_IO_MUX_SEL_1_GPIO_MDX1_SEL_0_MASK, 1);
			break;
		case 41:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 RTL8373_IO_MUX_SEL_1_GPIO_MDX1_SEL_1_MASK, 1);
			break;
		case 42 ... 45:
			// what fuck is this?
			// I don't know how does realtek use two bits to represent gpio 42 43 44 45
			// Is this a magic?????
			ret = rtl837x_reg_bits_write(priv, RTL8373_INI_MODE_ADDR,
					 RTL8373_INI_MODE_INI_MODE_MASK, 1);
			break;
		case 46 ... 51:
			// MSCK and MSDA
			// Humm. what val should I set to make the gpiomux mode to gpio?
			// Maybe is 0b0011 ? I'm not sure about this.
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 (0x180 << ((offset-46)*2)), 3);
			break;
		case 52 ... 54:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_2_ADDR,
					 BIT(offset-52), 1);
			break;
		case 55 ... 60:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 BIT(offset-36), 1);
			break;
		case 61 ... 62:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					 BIT(offset-34), 1);
			break;
		case 63:
			ret = rtl837x_reg_bits_write(priv, RTL8373_IO_MUX_SEL_1_ADDR,
					RTL8373_IO_MUX_SEL_1_GPIO_MDIO0_SEL_OFFSET, 1);
			break;
		default:
			dev_err(priv->dev, "gpio(%d): failed to request gpio, Out of range\n", offset);
			return -EINVAL;
	}

	if (ret)
	{
		dev_err(priv->dev, "gpio(%d): failed to request gpio err: %x", offset, ret);
		return -ENODEV;
	}
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,18,0)
static int rtl837x_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
#else
static void rtl837x_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
#endif
{
	int ret;
	struct rtl837x_gpio *gpio = gpiochip_get_data(gc);
	struct rtl837x_priv *priv = gpio->priv;

	if (offset < 32)
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OUT0_ADDR,
				BIT(offset), !!value);
	else 
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OUT1_ADDR,
				BIT(offset-32), !!value);

	if (ret)
	{
		dev_err(priv->dev, "gpio(%d): failed to write gpio val %x", offset, ret);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,18,0)
		return -EIO;
#endif
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,18,0)
	return 0;
#endif
}

static int rtl837x_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	int ret;
	u32 tmp;
	struct rtl837x_gpio *gpio = gpiochip_get_data(gc);
	struct rtl837x_priv *priv = gpio->priv;

	if (offset < 32)
		ret = rtl837x_reg_bits_read(priv, RTL8373_GPIO_IN0_ADDR,
				BIT(offset), &tmp);
	else 
		ret = rtl837x_reg_bits_read(priv, RTL8373_GPIO_IN1_ADDR,
				BIT(offset-32), &tmp);

	if (ret)
	{
		dev_err(priv->dev, "gpio(%d): failed to read gpio val %x", offset, ret);
		return ret;
	}

	return !!(tmp&1);
}

static int rtl837x_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	int ret;
	struct rtl837x_gpio *gpio = gpiochip_get_data(gc);
	struct rtl837x_priv *priv = gpio->priv;
	
	if (offset < 32)
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OE0_ADDR,
				BIT(offset), 0);
	else 
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OE1_ADDR,
				BIT(offset-32), 0);

	if (ret)
	{
		dev_err(priv->dev, "gpio(%d): failed to set gpio direction input %x", offset, ret);
		return -EINVAL;
	}
	return 0;
}

static int rtl837x_gpio_direction_output(struct gpio_chip *gc,
					unsigned offset, int value)
{
	int ret;
	struct rtl837x_gpio *gpio = gpiochip_get_data(gc);
	struct rtl837x_priv *priv = gpio->priv;

	if (offset < 32)
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OE0_ADDR,
				BIT(offset), 1);
	else 
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OE1_ADDR,
				BIT(offset-32), 1);

	if (ret)
	{
		dev_err(gpio->priv->dev, "gpio(%d): failed to set gpio direction output %x", offset, ret);
		return ret;
	}

	if (offset < 32)
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OUT0_ADDR,
				BIT(offset), !!value);
	else 
		ret = rtl837x_reg_bits_write(priv, RTL8373_GPIO_OUT1_ADDR,
				BIT(offset-32), !!value);

	if (ret)
	{
		dev_err(gpio->priv->dev, "gpio(%d): failed to write gpio val %x", offset, ret);
		return ret;
	}
	return 0;
}

static const struct gpio_chip rtl837x_gp = {
	.label = "rtl837x-gpio",
	.owner = THIS_MODULE,
	.request = rtl837x_gpio_request,
	.get = rtl837x_gpio_get,
	.set = rtl837x_gpio_set,
	.direction_input = rtl837x_gpio_direction_input,
	.direction_output = rtl837x_gpio_direction_output,
	.can_sleep = true,
	.ngpio = 63,
};

int rtl837x_gpiochip_init(struct rtl837x_priv *priv)
{
	struct device *dev = priv->dev;
	struct device_node *np = dev->of_node;

	struct rtl837x_gpio *gpio;

	int base = 0;
	if (of_property_read_u32(np, "base", &base))
		base = -1;

	gpio = devm_kmalloc(dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	gpio->priv = priv;
	gpio->gp = rtl837x_gp;
	gpio->gp.base = base;
	gpio->gp.parent = dev;

	return devm_gpiochip_add_data(dev, &gpio->gp, gpio);
}
