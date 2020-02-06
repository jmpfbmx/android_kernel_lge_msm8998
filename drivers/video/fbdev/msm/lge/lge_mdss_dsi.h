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

#ifndef LGE_MDSS_DSI_H
#define LGE_MDSS_DSI_H
#include "../mdss_dsi.h"
#include <linux/interrupt.h>
#include <linux/irq.h>
#include "mplus/lge_mplus.h"

#define NUM_COLOR_MODES 	10

struct lge_gpio_entry {
	char name[32];
	int gpio;
};

struct lge_dsi_cmds_entry {
	char name[128];
	struct dsi_panel_cmds lge_dsi_cmds;
};

struct lge_ddic_ops {
	/* blmap */
	int (*op_get_blmap_type)(struct mdss_dsi_ctrl_pdata *ctrl);
	void (*op_blmap_changed)(struct mdss_dsi_ctrl_pdata *ctrl);

	/* boost brightness */
	int (*op_boost_brightness)(struct mdss_dsi_ctrl_pdata *ctrl, int value);

	/* image_enhance */
	int (*op_image_enhance_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_image_enhance_get)(struct mdss_dsi_ctrl_pdata *ctrl);

	/* hdr_mode */
	int (*op_hdr_mode_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_hdr_mode_get)(struct mdss_dsi_ctrl_pdata *ctrl);

	/* aod */
	int (*op_send_u2_cmds)(struct mdss_dsi_ctrl_pdata *ctrl);

	/* MPLUS */
	void (*op_send_mplus_mode_cmds)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_mplus_mode_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_mplus_mode_get)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*op_mplus_hd_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_mplus_hd_get)(struct mdss_dsi_ctrl_pdata *ctrl);
	int (*op_mplus_max_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
	int (*op_mplus_max_get)(struct mdss_dsi_ctrl_pdata *ctrl);

	/* ht_tune */
	void (*op_ht_mode_set)(struct mdss_dsi_ctrl_pdata *ctrl, int mode);
};

struct lge_rect {
	int x;
	int y;
	int w;
	int h;
};

struct lge_mdss_dsi_ctrl_pdata {
	/* multi panel support */
	int panel_id;

	/* blank mode support */
	int blank_mode;

	/* panel type for minios */
	char panel_type[MDSS_MAX_PANEL_LEN];

	/* gpio */
	int num_gpios;
	struct lge_gpio_entry *gpio_array;

	/* blmap */
	char **blmap_list;
	int blmap_list_size;
	int **blmap;
	int *blmap_size;
	int cur_bl_lvl;

	/* cmds */
	int num_extra_cmds;
	struct lge_dsi_cmds_entry *extra_cmds_array;

	/* boost brightness */
	int boost_br_criteria;

	/* hdr */
	int hdr_mode;

	/* mplus */
	bool use_mplus;
	enum lge_mplus_mode mplus_hd;
	enum lge_mplus_mode mp_max;
	enum lge_mplus_mode mp_mode;
	enum lge_mplus_mode cur_mp_mode;
	int *mp_to_blmap_tbl;
	int mp_to_blmap_tbl_size;

	/* aod */
	bool aod_area_full;
	struct lge_rect aod_area;

	/* ddic ops */
	struct lge_ddic_ops *ddic_ops;
};

#define LGE_DDIC_OP_CHECK(c, op) (c && c->lge_extra.ddic_ops && c->lge_extra.ddic_ops->op_##op)
#define LGE_DDIC_OP(c, op, ...) (LGE_DDIC_OP_CHECK(c,op)?c->lge_extra.ddic_ops->op_##op(c, ##__VA_ARGS__):-ENODEV)
#define LGE_DDIC_OP_LOCKED(c, op, lock, ...) do { \
	mutex_lock(lock); \
	if (LGE_DDIC_OP_CHECK(c,op)) c->lge_extra.ddic_ops->op_##op(c, ##__VA_ARGS__); \
	mutex_unlock(lock); } while(0)

#define LGE_MDELAY(m) do { if ( m > 0) usleep_range((m)*1000,(m)*1000); } while(0)
#define LGE_OVERRIDE_VALUE(x, v) do { if ((v)) (x) = (v); } while(0)

#include "lge_mdss_dsi_panel.h"

int lge_mdss_dsi_parse_extra_params(
	struct platform_device *ctrl_pdev,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
void lge_extra_gpio_set_value(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
		const char *name, int value);

int detect_factory_cable(void);
int detect_qem_factory_cable(void);

#if IS_ENABLED(CONFIG_LGE_DISPLAY_OVERRIDE_MDSS_DSI_CTRL_SHUTDOWN)
void mdss_dsi_ctrl_shutdown(struct platform_device *pdev);
#endif

#if IS_ENABLED(CONFIG_LGE_DISPLAY_OVERRIDE_MDSS_DSI_PANEL_ON)
int mdss_dsi_panel_on(struct mdss_panel_data *pdata);
#endif

#if IS_ENABLED(CONFIG_LGE_DISPLAY_OVERRIDE_MDSS_DSI_PANEL_OFF)
int mdss_dsi_panel_off(struct mdss_panel_data *pdata);
#endif

#if IS_ENABLED(CONFIG_LGE_DISPLAY_OVERRIDE_MDSS_DSI_PANEL_POWER_ON)
int mdss_dsi_panel_power_on(struct mdss_panel_data *pdata);
#endif

#if IS_ENABLED(CONFIG_LGE_DISPLAY_OVERRIDE_MDSS_DSI_PANEL_POWER_OFF)
int mdss_dsi_panel_power_off(struct mdss_panel_data *pdata);
#endif

#endif /* LGE_MDSS_DSI_H */
