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

#ifndef LGE_MDSS_FB_H
#define LGE_MDSS_FB_H

#include <soc/qcom/lge/board_lge.h>
#include "../mdss_dsi.h"
#include "lge_mdss_dsi.h"

int lge_br_to_bl(struct msm_fb_data_type *mfd, int br_lvl);
#ifdef CONFIG_LGE_LCD_OFF_DIMMING
void lge_set_blank_called(void);
#endif
void lge_mdss_fb_init(struct msm_fb_data_type *mfd);
void lge_boost_brightness(struct msm_fb_data_type *mfd, int value);
#endif /* LGE_MDSS_FB_H */
