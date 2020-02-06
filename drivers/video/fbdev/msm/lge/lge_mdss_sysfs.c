#include <linux/of_platform.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include "../mdss_fb.h"
#include "../mdss_mdp.h"
#include "../mdss_dsi.h"

#include "lge_mdss_sysfs.h"

extern int lge_mdss_aod_sysfs_init(struct class *panel, struct fb_info *fbi);
extern void lge_mdss_aod_sysfs_deinit(struct fb_info *fbi);

struct class *panel = NULL;					/* lge common class node "/sys/class/panel/" */
struct device *lge_panel_sysfs_dev = NULL;	/* lge common device node "/sys/class/panel/dev0/" */
struct device *lge_panel_sysfs_imgtune = NULL; /* lge common img tune node "/sys/class/panel/img_tune/" */

ssize_t lge_get_multi_panel_support_flag(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret = 0, val = 0;
	GET_DATA

	val = BIT(ctrl->lge_extra.panel_id);
	ret = scnprintf(buf, PAGE_SIZE, "%d\n", val);

	return ret;
}
static DEVICE_ATTR(panel_flag, S_IRUGO, lge_get_multi_panel_support_flag, NULL);

static ssize_t mdss_fb_get_panel_type(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	GET_DATA

	ret = snprintf(buf, PAGE_SIZE, "%s\n", ctrl->lge_extra.panel_type);
	return ret;
}
static DEVICE_ATTR(panel_type, S_IRUGO, mdss_fb_get_panel_type, NULL);

static ssize_t image_enhance_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	GET_DATA

	ret = LGE_DDIC_OP(ctrl, image_enhance_get);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t image_enhance_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, image_enhance_set, &mfd->mdss_sysfs_lock, mode);
	LGE_DDIC_OP_LOCKED(ctrl, blmap_changed, &mfd->bl_lock);
	return ret;
}
static DEVICE_ATTR(image_enhance_set, S_IRUGO|S_IWUSR|S_IWGRP, image_enhance_get, image_enhance_set);

ssize_t ht_lcd_tune_set(struct device *dev,
               struct device_attribute *attr, const char *buf, size_t len)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode = 0;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, ht_mode_set, &mfd->mdss_sysfs_lock, mode);
	return ret;
}

static DEVICE_ATTR(ht_lcd_tune, S_IWUSR|S_IRUGO, NULL, ht_lcd_tune_set);

/* Mplus Set */
static ssize_t mplus_mode_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	GET_DATA

	ret = LGE_DDIC_OP(ctrl, mplus_mode_get);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t mplus_mode_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, mplus_mode_set, &mfd->mdss_sysfs_lock, mode);
	LGE_DDIC_OP_LOCKED(ctrl, blmap_changed, &mfd->bl_lock);
	return ret;
}
static DEVICE_ATTR(mplus_mode, S_IRUGO|S_IWUSR|S_IWGRP, mplus_mode_get, mplus_mode_set);

/* Hidden Menu Mplus Set */
static ssize_t mplus_hd_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	GET_DATA

	ret = LGE_DDIC_OP(ctrl, mplus_hd_get);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t mplus_hd_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, mplus_hd_set, &mfd->mdss_sysfs_lock, mode);
	LGE_DDIC_OP_LOCKED(ctrl, blmap_changed, &mfd->bl_lock);
	return ret;
}
static DEVICE_ATTR(mplus_hd, S_IRUGO|S_IWUSR|S_IWGRP, mplus_hd_get, mplus_hd_set);

/* Mplus Max Set */
static ssize_t mplus_max_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	GET_DATA

	ret = LGE_DDIC_OP(ctrl, mplus_max_get);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t mplus_max_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, mplus_max_set, &mfd->mdss_sysfs_lock, mode);
	LGE_DDIC_OP_LOCKED(ctrl, blmap_changed, &mfd->bl_lock);
	return ret;
}
static DEVICE_ATTR(mplus_max, S_IRUGO|S_IWUSR|S_IWGRP, mplus_max_get, mplus_max_set);

/* HDR mode Set */
static ssize_t hdr_mode_get(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	GET_DATA

	ret = LGE_DDIC_OP(ctrl, hdr_mode_get);
	return sprintf(buf, "%d\n", ret);
}

static ssize_t hdr_mode_set(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int mode;
	GET_DATA

	SKIP_WHILE_CONT_SPLASH_ENABLED();

	sscanf(buf, "%d", &mode);
	LGE_DDIC_OP_LOCKED(ctrl, hdr_mode_set, &mfd->mdss_sysfs_lock, mode);
	LGE_DDIC_OP_LOCKED(ctrl, blmap_changed, &mfd->bl_lock);
	return ret;
}
static DEVICE_ATTR(hdr_mode, S_IRUGO|S_IWUSR|S_IWGRP, hdr_mode_get, hdr_mode_set);

/* "/sys/class/panel/dev0/" */
struct attribute *lge_mdss_panel_sysfs_list[] = {
	&dev_attr_panel_flag.attr,
	NULL,
};

/* "/sys/class/graphics/fb0/" */
struct attribute *lge_mdss_fb_sysfs_list[] = {
	&dev_attr_panel_type.attr,
	NULL,
};

/* "/sys/class/panel/img_tune/" *" */
struct attribute *lge_mdss_imgtune_sysfs_list[] = {
	&dev_attr_image_enhance_set.attr,
	&dev_attr_ht_lcd_tune.attr,
	&dev_attr_hdr_mode.attr,
	NULL,
};

/* "/sys/class/panel/img_tune/mplus_*" */
struct attribute *lge_mdss_imgtune_mplus_sysfs_list[] = {
	&dev_attr_mplus_mode.attr,
	&dev_attr_mplus_hd.attr,
	&dev_attr_mplus_max.attr,
	NULL,
};

static struct attribute_group lge_mdss_fb_sysfs_group = {
	.attrs = lge_mdss_fb_sysfs_list,
};

static struct attribute_group lge_mdss_panel_sysfs_group = {
	.attrs = lge_mdss_panel_sysfs_list,
};

static struct attribute_group lge_mdss_imgtune_sysfs_group = {
	.attrs = lge_mdss_imgtune_sysfs_list
};

static struct attribute_group lge_mdss_imgtune_mplus_sysfs_group = {
	.attrs = lge_mdss_imgtune_mplus_sysfs_list
};

static int lge_mdss_mplus_sysfs_init(struct fb_info *fbi)
{
	int ret = 0;
	struct msm_fb_data_type *mfd = NULL;
	struct mdss_panel_data *pdata = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	mfd = (struct msm_fb_data_type *)(fbi)->par;
	if (mfd == NULL) {
		pr_err("uninitialzed mfd\n");
		return -EINVAL;
	}
	pdata = dev_get_platdata(&mfd->pdev->dev);
	if (pdata == NULL) {
		pr_err("no panel connected!\n");
		return -EINVAL;
	}
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
	if (ctrl->lge_extra.use_mplus) {
		ret = sysfs_create_group(&lge_panel_sysfs_imgtune->kobj, &lge_mdss_imgtune_mplus_sysfs_group);
	}
	return ret;
}

static void lge_mdss_mplus_sysfs_deinit(struct fb_info *fbi)
{
	struct msm_fb_data_type *mfd = NULL;
	struct mdss_panel_data *pdata = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;

	mfd = (struct msm_fb_data_type *)(fbi)->par;
	if (mfd == NULL) {
		pr_err("uninitialzed mfd\n");
		return;
	}
	pdata = dev_get_platdata(&mfd->pdev->dev);
	if (pdata == NULL) {
		pr_err("no panel connected!\n");
		return;
	}
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);
	if (ctrl->lge_extra.use_mplus) {
		sysfs_remove_group(&fbi->dev->kobj, &lge_mdss_imgtune_mplus_sysfs_group);
	}
}

int lge_mdss_sysfs_init(struct fb_info *fbi)
{
	int ret = 0;

	if(!panel) {
		panel = class_create(THIS_MODULE, "panel");
		if (IS_ERR(panel))
			pr_err("%s: Failed to create panel class\n", __func__);
	}

	if(!lge_panel_sysfs_imgtune) {
		lge_panel_sysfs_imgtune = device_create(panel, NULL, 0, fbi, "img_tune");
		if(IS_ERR(lge_panel_sysfs_imgtune)) {
			pr_err("%s: Failed to create dev(lge_panel_sysfs_imgtune)!", __func__);
		}
		else {
			ret += sysfs_create_group(&lge_panel_sysfs_imgtune->kobj, &lge_mdss_imgtune_sysfs_group);

			ret += lge_mdss_mplus_sysfs_init(fbi);
		}
	}

	if(!lge_panel_sysfs_dev) {
		lge_panel_sysfs_dev = device_create(panel, NULL, 0, fbi, "dev0");
		if (IS_ERR(lge_panel_sysfs_dev)) {
			pr_err("%s: Failed to create lge_panel_sysfs_dev class\n", __func__);
		}else{
			ret += sysfs_create_group(&lge_panel_sysfs_dev->kobj, &lge_mdss_panel_sysfs_group);
		}
	}
	ret += sysfs_create_group(&fbi->dev->kobj, &lge_mdss_fb_sysfs_group);

	ret += lge_mdss_aod_sysfs_init(panel, fbi);

	if (ret)
		return ret;

	return 0;
}

void lge_mdss_sysfs_deinit(struct fb_info *fbi) {
	sysfs_remove_group(&fbi->dev->kobj, &lge_mdss_fb_sysfs_group);
	sysfs_remove_group(&fbi->dev->kobj, &lge_mdss_panel_sysfs_group);
	sysfs_remove_group(&fbi->dev->kobj, &lge_mdss_imgtune_sysfs_group);
	lge_mdss_mplus_sysfs_deinit(fbi);
	lge_mdss_aod_sysfs_deinit(fbi);
}
