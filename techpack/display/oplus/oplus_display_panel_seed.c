/***************************************************************
** Copyright (C),  2020,  OPLUS Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oplus_display_panel_seed.c
** Description : oplus display panel seed feature
** Version : 1.0
** Date : 2020/06/13
** Author : Li.Sheng@MULTIMEDIA.DISPLAY.LCD
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**  Li.Sheng       2020/06/13        1.0           Build this moudle
******************************************************************/
#include "oplus_display_panel_seed.h"
#include "oplus_dsi_support.h"

extern bool oplus_dc_v2_on;
extern bool pms_mode_flag;
bool seed_mode_flag = false;
extern u32 bl_lvl_backup;
extern int seed_mode_tmp;
extern int lcd_closebl_flag;
extern bool is_dc_set_color_mode;
#define PANEL_LOADING_EFFECT_FLAG  100
#define PANEL_LOADING_EFFECT_MODE1 101
#define PANEL_LOADING_EFFECT_MODE2 102
#define PANEL_LOADING_EFFECT_OFF   100

int seed_mode = 0;
DEFINE_MUTEX(oplus_seed_lock);

int oplus_display_get_seed_mode(void)
{
	return seed_mode;
}

int __oplus_display_set_seed(int mode) {
	struct dsi_display *display = get_main_display();

	mutex_lock(&oplus_seed_lock);
	if (!strcmp(display->panel->oplus_priv.vendor_name, "SOFE03F")) {
		if (bl_lvl_backup == 0 && oplus_dc_v2_on == false && seed_mode == 1) {
			seed_mode_flag = true;
		}
	}
	if (!strcmp(display->panel->oplus_priv.vendor_name, "SOFE03F")) {
		if (display->panel->is_hbm_enabled) {
			if(mode != 101)
				seed_mode_tmp = mode;
		} else if (mode != seed_mode) {
			seed_mode = mode;
		}
	} else {
		seed_mode = mode;
	}
	mutex_unlock(&oplus_seed_lock);
	return 0;
}

int dsi_panel_seed_mode_unlock(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!dsi_panel_initialized(panel))
		return -EINVAL;

	if (oplus_dc_v2_on) {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_ENTER);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_ENTER cmds, rc=%d\n",
				panel->name, rc);
		}
		if (!strcmp(panel->oplus_priv.vendor_name, "SOFE03F")) {
			if (seed_mode_flag  == true) {
				seed_mode = 1;
				mode = 1;
				seed_mode_flag = false;
			}
		}
	} else {
		if(!strcmp(panel->oplus_priv.vendor_name, "AMB655XL08")) {
			int frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);
			dsi_panel_set_backlight(panel, panel->bl_config.bl_level);
			usleep_range(frame_time_us * 2, frame_time_us * 2 + 100);
		} else if (!strcmp(panel->oplus_priv.vendor_name, "SOFE03F")) {
			int frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);
			if (pms_mode_flag == true) {
				pms_mode_flag = false;
			} else {
				if (lcd_closebl_flag == 1) {
					pr_err("silence mode need set backlight to zero");
					mipi_dsi_dcs_set_display_brightness(&panel->mipi_device, 0);
					usleep_range(frame_time_us * 2, frame_time_us * 2 + 100);
				} else {
					mipi_dsi_dcs_set_display_brightness(&panel->mipi_device, panel->bl_config.bl_level);
					usleep_range(frame_time_us * 2, frame_time_us * 2 + 100);
				}
			}
		} else if (!strcmp(panel->oplus_priv.vendor_name, "AMS662ZS01")) {
			int frame_time_us = mult_frac(1000, 1000, panel->cur_mode->timing.refresh_rate);
			if (pms_mode_flag == true) {
				pms_mode_flag = false;
			} else {
				if (lcd_closebl_flag == 1) {
                                        pr_err("silence mode need set backlight to zero");
                                        mipi_dsi_dcs_set_display_brightness(&panel->mipi_device, 0);
				} else {
					mipi_dsi_dcs_set_display_brightness(&panel->mipi_device, panel->bl_config.bl_level);
					usleep_range(frame_time_us, frame_time_us + 100);
				}
			}
		}
		if (!strcmp(panel->oplus_priv.vendor_name, "SOFE03F")) {
			if (panel->is_hbm_enabled)
				mode = seed_mode_tmp - 100;
		}
	}

	switch (mode) {
	case 0:
		if (!strcmp(panel->oplus_priv.vendor_name, "AMS662ZS01")) {
			if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0_DC_SWITCH);
				if (rc) {
					pr_err("[%s] failed to send DSI_CMD_SEED_MODE0_DC_SWITCH cmds, rc=%d\n",
							panel->name, rc);
				}
			} else if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (!oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0_SWITCH);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE0_SWITCH cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
			} else if (oplus_dc_v2_on) {
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0_DC);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE0_DC cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
			} else {
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0);
				if (rc) {
					pr_err("[%s] failed to send DSI_CMD_SEED_MODE0 cmds, rc=%d\n",
							panel->name, rc);
				}
			}
		} else {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE0);
			if (rc) {
				pr_err("[%s] failed to send DSI_CMD_SEED_MODE0 cmds, rc=%d\n",
						panel->name, rc);
			}
		}
		break;
	case 1:
		if (!strcmp(panel->oplus_priv.vendor_name, "AMS662ZS01")) {
                        if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1_DC_SWITCH);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE1_DC_SWITCH cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
			} else if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (!oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1_SWITCH);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE1_SWITCH cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
                        } else if (oplus_dc_v2_on) {
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1_DC);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE1_DC cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
                        } else {
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1);
				if (rc) {
					pr_err("[%s] failed to send DSI_CMD_SEED_MODE1 cmds, rc=%d\n",
							panel->name, rc);
				}
			}
		} else {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE1);
			if (rc) {
				pr_err("[%s] failed to send DSI_CMD_SEED_MODE1 cmds, rc=%d\n",
						panel->name, rc);
			}
		}
		break;
	case 2:
		if (!strcmp(panel->oplus_priv.vendor_name, "AMS662ZS01")) {
                        if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2_DC_SWITCH);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE2_DC_SWITCH cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
			} else if ((is_dc_set_color_mode) && (!panel->is_hbm_enabled) && (!oplus_dc_v2_on)) {
				is_dc_set_color_mode = false;
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2_SWITCH);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE2_SWITCH cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
                        } else if (oplus_dc_v2_on) {
                                rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2_DC);
                                if (rc) {
                                        pr_err("[%s] failed to send DSI_CMD_SEED_MODE2_DC cmds, rc=%d\n",
                                                        panel->name, rc);
                                }
                        } else {
				rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2);
				if (rc) {
					pr_err("[%s] failed to send DSI_CMD_SEED_MODE2 cmds, rc=%d\n",
							panel->name, rc);
				}
			}
		} else {
			rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE2);
			if (rc) {
				pr_err("[%s] failed to send DSI_CMD_SEED_MODE2 cmds, rc=%d\n",
						panel->name, rc);
			}
		}
		break;
	case 3:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE3);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE3 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 4:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_MODE4);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE4 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_OFF);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_OFF cmds, rc=%d\n",
					panel->name, rc);
		}
		pr_err("[%s] seed mode Invalid %d\n",
			panel->name, mode);
	}

	if (!oplus_dc_v2_on) {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SEED_EXIT);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_EXIT cmds, rc=%d\n",
				panel->name, rc);
		}
	}

	return rc;
}

int dsi_panel_loading_effect_mode_unlock(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!dsi_panel_initialized(panel)) {
		return -EINVAL;
	}

	switch (mode) {
	case PANEL_LOADING_EFFECT_MODE1:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_LOADING_EFFECT_MODE1);

		if (rc) {
			pr_err("[%s] failed to send PANEL_LOADING_EFFECT_MODE1 cmds, rc=%d\n",
			       panel->name, rc);
		}

		break;

	case PANEL_LOADING_EFFECT_MODE2:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_LOADING_EFFECT_MODE2);

		if (rc) {
			pr_err("[%s] failed to send PANEL_LOADING_EFFECT_MODE2 cmds, rc=%d\n",
			       panel->name, rc);
		}

		break;

	case PANEL_LOADING_EFFECT_OFF:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_LOADING_EFFECT_OFF);

		if (rc) {
			pr_err("[%s] failed to send PANEL_LOADING_EFFECT_OFF cmds, rc=%d\n",
			       panel->name, rc);
		}

		break;

	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_LOADING_EFFECT_OFF);

		if (rc) {
			pr_err("[%s] failed to send PANEL_LOADING_EFFECT_OFF cmds, rc=%d\n",
			       panel->name, rc);
		}

		pr_err("[%s] loading effect mode Invalid %d\n",
		       panel->name, mode);
	}

	return rc;
}

int dsi_panel_seed_mode(struct dsi_panel *panel, int mode) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	//mutex_lock(&panel->panel_lock);

	if ((!strcmp(panel->oplus_priv.vendor_name, "S6E3HC3")
		|| !strcmp(panel->oplus_priv.vendor_name, "AMB655XL08"))
		&& (mode >= PANEL_LOADING_EFFECT_FLAG)) {
		rc = dsi_panel_loading_effect_mode_unlock(panel, mode);
	} else if ((!strcmp(panel->oplus_priv.vendor_name, "ANA6706") || !strcmp(panel->oplus_priv.vendor_name, "SOFE03F")
			|| !strcmp(panel->oplus_priv.vendor_name, "AMS662ZS01"))
				&& (mode >= PANEL_LOADING_EFFECT_FLAG)) {
		mode = mode - PANEL_LOADING_EFFECT_FLAG;
		rc = dsi_panel_seed_mode_unlock(panel, mode);
		if (!strcmp(panel->oplus_priv.vendor_name, "SOFE03F")) {
			if (!panel->is_hbm_enabled)
				seed_mode = mode;
		} else {
			seed_mode = mode;
		}
	} else if (!strcmp(panel->oplus_priv.vendor_name, "AMB655X")) {
		rc = dsi_panel_loading_effect_mode_unlock(panel, mode);
	} else {
		rc = dsi_panel_seed_mode_unlock(panel, mode);
	}
	//mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_seed_mode(struct dsi_display *display, int mode) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}
	/* james.zhu@MULTIMEDIA.DISPLAY.LCD.FEATURE, 2021/03/22, Add for loading effect cmd lock */
	if (!strcmp(display->panel->oplus_priv.vendor_name, "AMB655X") || !strcmp(display->panel->oplus_priv.vendor_name, "SOFE03F") ||
		!strcmp(display->panel->oplus_priv.vendor_name, "AMS662ZS01")) {
		mutex_lock(&display->panel->panel_lock);
	}
	rc = dsi_panel_seed_mode(display->panel, mode);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_seed_or_loading_effect, rc=%d\n",
			       display->name, rc);
	}
	if (!strcmp(display->panel->oplus_priv.vendor_name, "AMB655X") || !strcmp(display->panel->oplus_priv.vendor_name, "SOFE03F") ||
		!strcmp(display->panel->oplus_priv.vendor_name, "AMS662ZS01")) {
		mutex_unlock(&display->panel->panel_lock);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);

	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int oplus_dsi_update_seed_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	ret = dsi_display_seed_mode(display, seed_mode);

	return ret;
}

int oplus_display_panel_get_seed(void *data)
{
	uint32_t *temp = data;
	printk(KERN_INFO "oplus_display_get_seed = %d\n",seed_mode);

	(*temp) = seed_mode;
	return 0;
}

int oplus_display_panel_set_seed(void *data)
{
	uint32_t *temp_save = data;

	printk(KERN_INFO "%s oplus_display_set_seed = %d\n", __func__, *temp_save);
	seed_mode = *temp_save;

	__oplus_display_set_seed(*temp_save);
	if(get_oplus_display_power_status() == OPLUS_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oplus_display_set_seed and main display is null");
			return -EINVAL;
		}
		dsi_display_seed_mode(get_main_display(), seed_mode);
	} else {
		printk(KERN_ERR	 "%s oplus_display_set_seed = %d, but now display panel status is not on\n", __func__, *temp_save);
	}

	return 0;
}
