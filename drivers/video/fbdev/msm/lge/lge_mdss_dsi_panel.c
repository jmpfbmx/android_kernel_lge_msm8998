/*
 * Copyright(c) 2016, LG Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"[Display] %s: " fmt, __func__

#include <linux/of_platform.h>
#include "../mdss_dsi.h"
#include "lge_mdss_dsi_panel.h"
#include <linux/delay.h>

#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "lge."
int panel_not_connected = 0;
static int param_set_panel_not_connected(const char *val, const struct kernel_param *kp)
{
	unsigned long buf;
	int ret = kstrtoul(val, 0, &buf);
	if (ret)
		return ret;
	if (buf)
		panel_not_connected = 1;
	return 0;
}
static int param_get_panel_not_connected(char *buf, const struct kernel_param *kp)
{
	return scnprintf(buf, PAGE_SIZE-1, "%d", panel_not_connected);
}
static struct kernel_param_ops panel_not_connected_ops = {
	.set = param_set_panel_not_connected,
	.get = param_get_panel_not_connected,
};
module_param_cb(pinit_fail, &panel_not_connected_ops, NULL, S_IRUGO);

extern int mdss_dsi_parse_dcs_cmds(struct device_node *np,
		struct dsi_panel_cmds *pcmds, char *cmd_key, char *link_key);
extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_panel_cmds *pcmds, u32 flags);

int lge_mdss_panel_parse_dt_extra_cmds(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc;
	int i;
	const char *name;
	char buf1[256];
	char buf2[256];

	rc = of_property_count_strings(np, "lge,mdss-dsi-extra-command-names");
	if (rc > 0) {
		ctrl_pdata->lge_extra.num_extra_cmds = rc;
		pr_info("num_extra_cmds=%d\n", ctrl_pdata->lge_extra.num_extra_cmds);
		ctrl_pdata->lge_extra.extra_cmds_array = kmalloc(sizeof(struct lge_dsi_cmds_entry)*ctrl_pdata->lge_extra.num_extra_cmds, GFP_KERNEL);
		if (NULL == ctrl_pdata->lge_extra.extra_cmds_array) {
			pr_err("no memory\n");
			ctrl_pdata->lge_extra.num_extra_cmds = 0;
			return -ENOMEM;
		}
		for (i = 0; i < ctrl_pdata->lge_extra.num_extra_cmds; i++) {
			of_property_read_string_index(np, "lge,mdss-dsi-extra-command-names", i, &name);
			pr_info("%s\n", name);
			strlcpy(ctrl_pdata->lge_extra.extra_cmds_array[i].name, name, sizeof(ctrl_pdata->lge_extra.extra_cmds_array[i].name));
			snprintf(buf1, sizeof(buf1), "lge,mdss-dsi-extra-command-%s", name);
			snprintf(buf2, sizeof(buf2), "lge,mdss-dsi-extra-command-state-%s", name);
			mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->lge_extra.extra_cmds_array[i].lge_dsi_cmds, buf1, buf2);
		}

	} else {
		ctrl_pdata->lge_extra.num_extra_cmds = 0;
	}

	return 0;
}

struct dsi_panel_cmds *lge_get_extra_cmds_by_name(struct mdss_dsi_ctrl_pdata *ctrl_pdata, char *name)
{
	int i;
	if (ctrl_pdata == NULL) {
		pr_err("ctrl_pdata is NULL\n");
		return NULL;
	}

	for (i = 0; i < ctrl_pdata->lge_extra.num_extra_cmds; ++i) {
		if (!strcmp(ctrl_pdata->lge_extra.extra_cmds_array[i].name, name))
			return &ctrl_pdata->lge_extra.extra_cmds_array[i].lge_dsi_cmds;
	}
	return NULL;
}

void lge_send_extra_cmds_by_name(struct mdss_dsi_ctrl_pdata *ctrl_pdata, char *name)
{
	struct dsi_panel_cmds *pcmds = lge_get_extra_cmds_by_name(ctrl_pdata, name);
	if (pcmds) {
		mdss_dsi_panel_cmds_send(ctrl_pdata, pcmds, CMD_REQ_COMMIT);
	} else {
		pr_err("unsupported cmds: %s\n", name);
	}
}

void lge_mdss_panel_parse_dt_blmaps(struct device_node *np,
				   struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	const char *name;
	int i, rc;

	rc = of_property_count_strings(np, "lge,blmap-list");
	if (rc > 0) {
		ctrl_pdata->lge_extra.blmap_list_size = rc;
		pr_info("blmap_list_size=%d\n", ctrl_pdata->lge_extra.blmap_list_size);
		ctrl_pdata->lge_extra.blmap_list = kzalloc(sizeof(char*) * ctrl_pdata->lge_extra.blmap_list_size, GFP_KERNEL);
		ctrl_pdata->lge_extra.blmap = kzalloc(sizeof(int*) * ctrl_pdata->lge_extra.blmap_list_size, GFP_KERNEL);
		ctrl_pdata->lge_extra.blmap_size = kzalloc(sizeof(int) * ctrl_pdata->lge_extra.blmap_list_size, GFP_KERNEL);
		if (NULL == ctrl_pdata->lge_extra.blmap_list || NULL == ctrl_pdata->lge_extra.blmap || NULL == ctrl_pdata->lge_extra.blmap_size) {
			pr_err("allocation failed\n");
			ctrl_pdata->lge_extra.blmap_list_size = 0;
			goto error;
		}
		for (i = 0; i < ctrl_pdata->lge_extra.blmap_list_size; i++) {
			of_property_read_string_index(np, "lge,blmap-list", i, &name);
			pr_info("%s\n", name);
			ctrl_pdata->lge_extra.blmap_list[i] = kzalloc(strlen(name)+1, GFP_KERNEL);
			if (NULL == ctrl_pdata->lge_extra.blmap_list[i]) {
				pr_err("allocation for blmap name %s failed\n", name);
				goto error;
			}
			strcpy(ctrl_pdata->lge_extra.blmap_list[i], name);
			pr_info("%s\n", ctrl_pdata->lge_extra.blmap_list[i]);
			if (of_find_property(np, name, &ctrl_pdata->lge_extra.blmap_size[i])) {
				ctrl_pdata->lge_extra.blmap_size[i] /= sizeof(u32);
				ctrl_pdata->lge_extra.blmap[i] = kzalloc(sizeof(int) * ctrl_pdata->lge_extra.blmap_size[i], GFP_KERNEL);
				pr_info("blmap_size for blmap %s = %d\n", name, ctrl_pdata->lge_extra.blmap_size[i]);
				if (NULL == ctrl_pdata->lge_extra.blmap[i]) {
					pr_err("allocation for blmap %s failed\n", name);
					goto error;
				}
				if (of_property_read_u32_array(np, name, ctrl_pdata->lge_extra.blmap[i], ctrl_pdata->lge_extra.blmap_size[i])) {
					pr_err("parsing %s failed\n", name);
					kfree(ctrl_pdata->lge_extra.blmap[i]);
					ctrl_pdata->lge_extra.blmap[i] = NULL;
				}
			} else {
				ctrl_pdata->lge_extra.blmap_size[i] = 0;
				ctrl_pdata->lge_extra.blmap[i] = NULL;
			}
		}
	} else {
		ctrl_pdata->lge_extra.blmap_list_size = 0;
	}
	return;

error:
	for (i = 0; i < ctrl_pdata->lge_extra.blmap_list_size; ++i) {
		if (ctrl_pdata->lge_extra.blmap_list[i]) {
			kfree(ctrl_pdata->lge_extra.blmap_list[i]);
			ctrl_pdata->lge_extra.blmap_list[i] = NULL;
		}
		if (ctrl_pdata->lge_extra.blmap[i]) {
			kfree(ctrl_pdata->lge_extra.blmap[i]);
			ctrl_pdata->lge_extra.blmap[i] = NULL;
		}
	}
	if (ctrl_pdata->lge_extra.blmap_list) {
		kfree(ctrl_pdata->lge_extra.blmap_list);
		ctrl_pdata->lge_extra.blmap_list = NULL;
	}
	if (ctrl_pdata->lge_extra.blmap) {
		kfree(ctrl_pdata->lge_extra.blmap);
		ctrl_pdata->lge_extra.blmap = NULL;
	}
	if (ctrl_pdata->lge_extra.blmap_size) {
		kfree(ctrl_pdata->lge_extra.blmap_size);
		ctrl_pdata->lge_extra.blmap_size = NULL;
	}
	ctrl_pdata->lge_extra.blmap_list_size = 0;
}

static int find_blmap_index_by_name(struct mdss_dsi_ctrl_pdata *ctrl_pdata, const char *name)
{
	int index = -1;
	int i;

	for (i = 0; i < ctrl_pdata->lge_extra.blmap_list_size; ++i) {
		if (ctrl_pdata->lge_extra.blmap_list[i] && !strncmp(ctrl_pdata->lge_extra.blmap_list[i], name, strlen(name))) {
			index = i;
			break;
		}
	}
	if (index == -1)
		pr_err("index of blmap %s not found, blmap_list_size=%d\n", name, ctrl_pdata->lge_extra.blmap_list_size);
	return index;
}

static int lge_mdss_panel_parse_mplus_dt(struct device_node *np,
						struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	const char *name;
	int i, rc;

	ctrl_pdata->lge_extra.mp_to_blmap_tbl = NULL;
	ctrl_pdata->lge_extra.mp_to_blmap_tbl_size = 0;

	ctrl_pdata->lge_extra.use_mplus =
		LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_mode_set)
		&& LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_mode_get)
		&& LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_max_set)
		&& LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_max_get)
		&& LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_hd_set)
		&& LGE_DDIC_OP_CHECK(ctrl_pdata, mplus_hd_get);

	if (!ctrl_pdata->lge_extra.use_mplus)
		return 0;

	rc = of_property_count_strings(np, "lge,blmap-for-mplus-mode");
	if (rc > 0) {
		ctrl_pdata->lge_extra.mp_to_blmap_tbl_size = rc;
		pr_info("mp_to_blmap_tbl_size=%d\n", ctrl_pdata->lge_extra.mp_to_blmap_tbl_size);
		ctrl_pdata->lge_extra.mp_to_blmap_tbl = kzalloc(sizeof(int) * ctrl_pdata->lge_extra.blmap_list_size, GFP_KERNEL);
		if (NULL == ctrl_pdata->lge_extra.mp_to_blmap_tbl) {
			pr_err("allocation failed\n");
			ctrl_pdata->lge_extra.use_mplus = false;
			ctrl_pdata->lge_extra.mp_to_blmap_tbl_size = 0;
			return -ENOMEM;
		}
		for (i = 0; i < ctrl_pdata->lge_extra.mp_to_blmap_tbl_size; i++) {
			of_property_read_string_index(np, "lge,blmap-for-mplus-mode", i, &name);
			pr_info("%s\n", name);
			ctrl_pdata->lge_extra.mp_to_blmap_tbl[i] = find_blmap_index_by_name(ctrl_pdata, name);
		}
	} else {
		pr_err("lge,blmap-for-mplus-mode not exist");
		ctrl_pdata->lge_extra.use_mplus = false;
	}
	return 0;
}

static int lge_mdss_panel_parse_dt(struct device_node *np,
						struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct lge_mdss_dsi_ctrl_pdata *lge_extra;
	int rc = 0;
	u32 tmp = 0;
	const char *data;

	if (np == NULL || ctrl_pdata == NULL) {
		pr_err("Invalid input\n");
		return -EINVAL;
	}

	lge_extra = &(ctrl_pdata->lge_extra);

	rc = of_property_read_u32(np, "lge,panel-id", &tmp);
	if(!rc && tmp >=0 && tmp <= 7) {
		lge_extra->panel_id = tmp;
	} else {
		pr_info("failed to parse panel_id rc=%d, tmp=%d\n", rc, tmp);
	}

	data = of_get_property(np, "lge,panel-type", NULL);
	if (data) {
		snprintf(lge_extra->panel_type, sizeof(lge_extra->panel_type), data);
	} else {
		snprintf(lge_extra->panel_type, sizeof(lge_extra->panel_type), "UNDEFINED");
		pr_err("panel_type not specified\n");
	}

	rc = of_property_read_u32(np, "lge,boost-brightness-criteria", &tmp);
	if(!rc) {
		lge_extra->boost_br_criteria = tmp;
	} else {
		lge_extra->boost_br_criteria = MDSS_MAX_BL_BRIGHTNESS;
	}
	pr_info("boost_br_criteria=%d\n", lge_extra->boost_br_criteria);

	lge_mdss_panel_parse_dt_blmaps(np, ctrl_pdata);
	lge_mdss_panel_parse_mplus_dt(np, ctrl_pdata);
	lge_mdss_panel_parse_dt_extra_cmds(np, ctrl_pdata);

	return 0;
}

int lge_ddic_feature_init(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	if (ctrl_pdata == NULL)
		return -ENODEV;

	ctrl_pdata->lge_extra.mplus_hd = LGE_MP_OFF;
	ctrl_pdata->lge_extra.mp_max = LGE_MP_OFF;
	ctrl_pdata->lge_extra.mp_mode = LGE_MP_OFF;

	ctrl_pdata->lge_extra.hdr_mode = 0;

	return 0;
}

int lge_mdss_dsi_panel_init(struct device_node *node, struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	lge_ddic_ops_init(ctrl_pdata);

	lge_mdss_panel_parse_dt(node, ctrl_pdata);

	lge_ddic_feature_init(ctrl_pdata);
	return 0;
}

/* deprecated
 * TODO: remove this function
 */
int lge_mdss_dsi_panel_reg_backup(struct mdss_dsi_ctrl_pdata *ctrl)
{
	int ret = 0;
	return ret;
}

static int lge_panel_get_blmap_type(struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (!LGE_DDIC_OP_CHECK(ctrl, get_blmap_type))
		return 0;

	return LGE_DDIC_OP(ctrl, get_blmap_type);
}

int lge_panel_br_to_bl(struct mdss_dsi_ctrl_pdata *ctrl, int br_lvl)
{
	int type = 0;
	int *blmap = NULL;
	int blmap_size = 0;

	if (ctrl) {
		type = lge_panel_get_blmap_type(ctrl);
		pr_info("blmap type = %d\n", type);
		if (ctrl->lge_extra.blmap_list_size && type >= 0 && type < ctrl->lge_extra.blmap_list_size) {
			blmap = ctrl->lge_extra.blmap[type];
			blmap_size = ctrl->lge_extra.blmap_size[type];
		}
	} else {
		pr_err("ctrl is NULL\n");
	}

	if (blmap == NULL || blmap_size == 0) {
		pr_err("there is no blmap\n");
		return br_lvl?100:0;
	}
	if (br_lvl < 0)
		br_lvl = 0;
	if (br_lvl >= blmap_size)
		br_lvl = blmap_size-1;

	if (LGE_MP_FHB == ctrl->lge_extra.mplus_hd)
		br_lvl = blmap_size-1;

	return blmap[br_lvl];
}
