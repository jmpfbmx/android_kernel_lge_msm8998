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

#include <linux/msm_mdp.h>
#include "../mdss_fb.h"
#ifdef CONFIG_LGE_PM_LGE_POWER_CORE
#include <soc/qcom/lge/power/lge_power_class.h>
#include <soc/qcom/smem.h>
#endif
#ifdef CONFIG_LGE_PM_LGE_POWER_CLASS_CABLE_DETECT
#include <soc/qcom/lge/power/lge_cable_detect.h>
#else
#endif
#include <linux/power/lge_battery_id.h>
#include "lge_mdss_display.h"

struct msm_fb_data_type *mfd_primary_base = NULL;
extern void mdss_fb_bl_update_notify(struct msm_fb_data_type *mfd,
				uint32_t notification_type);
extern struct msm_fb_data_type *get_msm_fb_data(void);
extern void mdss_fb_report_panel_dead(struct msm_fb_data_type *mfd);

void lge_mdss_fb_init(struct msm_fb_data_type *mfd)
{
	if(mfd->index != 0)
		return;
	mfd_primary_base = mfd;

	mfd->bl_level_scaled = -1;
}

/*---------------------------------------------------------------------------*/
/* LCD off & dimming                                                         */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_LGE_LCD_OFF_DIMMING
static bool fb_blank_called;
static inline bool is_blank_called(void)
{
	return fb_blank_called;
}

static inline bool is_factory_cable(void)
{
	unsigned int cable_info;
#ifdef CONFIG_LGE_PM_LGE_POWER_CLASS_CABLE_DETECT
	struct lge_power *lge_cd_lpc;
	union lge_power_propval lge_val = {0,};
	int rc;
	unsigned int *p_cable_type = NULL;
	unsigned int cable_smem_size = 0;

	lge_cd_lpc = lge_power_get_by_name("lge_cable_detect");
	if (!lge_cd_lpc) {
		pr_err("lge_cd_lpc is not yet ready\n");
		p_cable_type = smem_get_entry(SMEM_ID_VENDOR1,
					&cable_smem_size, 0, 0);
		if (p_cable_type)
			cable_info = *p_cable_type;
		else
			return  false;

		pr_err("cable %d\n", cable_info);
#if !defined(CONFIG_LGE_PM_EMBEDDED_BATTERY)
		if (cable_info == LT_CABLE_56K ||
			cable_info == LT_CABLE_130K ||
			cable_info == LT_CABLE_910K)
#else
		if (cable_info == LT_CABLE_130K ||
			cable_info == LT_CABLE_910K)
#endif
			return true;
	} else {
		rc = lge_cd_lpc->get_property(lge_cd_lpc,
				LGE_POWER_PROP_CABLE_TYPE, &lge_val);
		cable_info = lge_val.intval;

#if !defined(CONFIG_LGE_PM_EMBEDDED_BATTERY)
		if (cable_info == CABLE_ADC_56K ||
			cable_info == CABLE_ADC_130K ||
			cable_info == CABLE_ADC_910K) {
#else
		if (cable_info == CABLE_ADC_130K ||
			cable_info == CABLE_ADC_910K) {
#endif

				pr_err("lge_cd_lpc is ready, cable_info = %d\n", cable_info);
				return true;
		}
	}
#elif defined (CONFIG_LGE_PM_CABLE_DETECTION)
	cable_info = lge_pm_get_cable_type();

#if !defined(CONFIG_LGE_PM_EMBEDDED_BATTERY)
	if (cable_info == CABLE_56K ||
		cable_info == CABLE_130K ||
		cable_info == CABLE_910K)
#else
	if (cable_info == CABLE_130K ||
		cable_info == CABLE_910K)
#endif
		return true;
	else
#else
	cable_info = 0;
#endif
		return false;
}

void lge_set_blank_called(void)
{
	fb_blank_called = true;
}
#else
static inline bool is_blank_called(void)
{
	return true;
}

static inline bool is_factory_cable(void)
{
	return false;
}
#endif

#ifdef CONFIG_LGE_PM_LGE_POWER_CLASS_BATTERY_ID_CHECKER
static bool batt_present = false;
bool lge_battery_present(void){
	struct lge_power *lge_batt_id_lpc;
	union lge_power_propval	lge_val = {0,};
	uint *smem_batt = 0;
	uint _smem_batt_id = 0;
	int rc;
	if (batt_present == true){
		return true;
		}
	lge_batt_id_lpc = lge_power_get_by_name("lge_batt_id");
	if (lge_batt_id_lpc) {
		rc = lge_batt_id_lpc->get_property(lge_batt_id_lpc,
				LGE_POWER_PROP_PRESENT, &lge_val);
		batt_present = lge_val.intval;
	}else{
		pr_err("Failed to get batt presnet property\n");
		smem_batt = (uint *)smem_alloc(SMEM_BATT_INFO,
				sizeof(smem_batt), 0, SMEM_ANY_HOST_FLAG);
		if (smem_batt == NULL) {
			pr_err("smem_alloc returns NULL\n");
			batt_present  = false;
		} else {
			_smem_batt_id = *smem_batt;
			pr_err("Battery was read in sbl is = %d\n",
					_smem_batt_id);
			if (_smem_batt_id == BATT_NOT_PRESENT) {
				pr_err("Set batt_id as DEFAULT\n");
				batt_present = false;
			}else{
				batt_present = true;
			}
		}
	}
	return batt_present;
}
#endif

/*---------------------------------------------------------------------------*/
/* Brightness - Backlight mapping (main)                                     */
/*---------------------------------------------------------------------------*/
int lge_br_to_bl (struct msm_fb_data_type *mfd, int br_lvl)
{
	/* TODO: change default value more reasonablly */
	int bl_lvl = 100;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if(mfd->index != 0) {
		pr_err("[Ambient] fb%d is not for ambient display\n", mfd->index);
		return bl_lvl ;
	}

	/* modify brightness level */
	if (is_factory_cable() && !is_blank_called()
#ifdef CONFIG_LGE_PM_LGE_POWER_CLASS_BATTERY_ID_CHECKER
#if !defined(CONFIG_LGE_PM_EMBEDDED_BATTERY)
		&& !lge_battery_present()
#endif
#elif defined (CONFIG_LGE_PM_BATTERY_ID_CHECKER)
		&& !lge_battery_check()
#endif
		) {
		br_lvl = 1;
		pr_info("Detect factory cable. set value = %d\n", br_lvl);
	}

	if (mfd->pdev && dev_get_platdata(&mfd->pdev->dev)) {
		ctrl_pdata = container_of(dev_get_platdata(&mfd->pdev->dev),
				struct mdss_dsi_ctrl_pdata, panel_data);
		/* map brightness level to device backlight level */
		bl_lvl = lge_panel_br_to_bl(ctrl_pdata, br_lvl);
	} else {
		pr_err("not ready\n");
	}

	return bl_lvl;
}

void lge_boost_brightness(struct msm_fb_data_type *mfd, int value)
{
	if (mfd && mfd->pdev && dev_get_platdata(&mfd->pdev->dev)) {
		struct mdss_dsi_ctrl_pdata *ctrl = container_of(dev_get_platdata(&mfd->pdev->dev), struct mdss_dsi_ctrl_pdata, panel_data);
		LGE_DDIC_OP(ctrl, boost_brightness, value);
	}
}

void lge_mdss_report_panel_dead(void)
{
	struct msm_fb_data_type *mfd = get_msm_fb_data();
	if (mfd) {
		mdss_fb_report_panel_dead(mfd);
	}
}
