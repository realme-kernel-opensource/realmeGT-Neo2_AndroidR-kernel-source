/***************************************************************
** Copyright (C),  2020,  OPLUS Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oplus_onscreenfingerprint.c
** Description : oplus onscreenfingerprint feature
** Version : 1.0
** Date : 2020/04/15
** Author : Qianxu@MM.Display.LCD Driver
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Qianxu         2020/04/15        1.0           Build this moudle
******************************************************************/

#include "sde_crtc.h"
#include "oplus_onscreenfingerprint.h"
#include "oplus_display_private_api.h"
#include "oplus_display_panel.h"
#include "oplus_adfr.h"

#define DSI_PANEL_OPLUS_DUMMY_VENDOR_NAME  "PanelVendorDummy"
#define DSI_PANEL_OPLUS_DUMMY_MANUFACTURE_NAME  "dummy1024"

bool oplus_pcc_enabled = false;
bool oplus_skip_pcc = false;
bool apollo_backlight_enable = false;
struct drm_msm_pcc oplus_save_pcc;
int oplus_dimlayer_hbm = 0;
/*#ifdef OPLUS_BUG_STABILITY*/
/*Yaqiang.Shi@RM.Display.LCD.Driver, add for 90Hz and 120Hz gamma switch*/
bool gamma_switch_enable = false;
/*#endif*/

extern int oplus_underbrightness_alpha;
extern int oplus_dimlayer_dither_threshold;
extern u32 oplus_last_backlight;
extern int oplus_panel_alpha;
extern int hbm_mode;
extern bool oplus_ffl_trigger_finish;
int oplus_dimlayer_hbm_vblank_count = 0;
atomic_t oplus_dimlayer_hbm_vblank_ref = ATOMIC_INIT(0);
bool oplus_enhance_mipi_strength = false;
extern struct oplus_apollo_backlight_list *p_apollo_backlight;
extern int dynamic_osc_clock;

static struct oplus_brightness_alpha brightness_alpha_lut[] = {
	{0, 0xff},
	{1, 0xee},
	{2, 0xe8},
	{3, 0xe6},
	{4, 0xe5},
	{6, 0xe4},
	{10, 0xe0},
	{20, 0xd5},
	{30, 0xce},
	{45, 0xc6},
	{70, 0xb7},
	{100, 0xad},
	{150, 0xa0},
	{227, 0x8a},
	{300, 0x80},
	{400, 0x6e},
	{500, 0x5b},
	{600, 0x50},
	{800, 0x38},
	{1023, 0x18},
};

static struct oplus_brightness_alpha brightness_alpha_lut_dc[] = {
	{0, 0xff},
	{1, 0xE0},
	{2, 0xd1},
	{3, 0xd0},
	{4, 0xcf},
	{5, 0xc9},
	{6, 0xc7},
	{8, 0xbe},
	{10, 0xb6},
	{15, 0xaa},
	{20, 0x9c},
	{30, 0x92},
	{45, 0x7c},
	{70, 0x5c},
	{100, 0x40},
	{120, 0x2c},
	{140, 0x20},
	{160, 0x1c},
	{180, 0x16},
	{200, 0x8},
	{223, 0x0},
};

int oplus_get_panel_brightness(void)
{
	struct dsi_display *display = get_main_display();

	if (!display)
		return 0;

	return display->panel->bl_config.bl_level;
}

static int bl_to_alpha(int brightness)
{
	struct dsi_display *display = get_main_display();
	struct oplus_brightness_alpha *lut = NULL;
	int count = 0;
	int i = 0;
	int alpha;

	if (!display)
		return 0;

	if (display->panel->ba_seq && display->panel->ba_count) {
		count = display->panel->ba_count;
		lut = display->panel->ba_seq;
	} else {
		count = ARRAY_SIZE(brightness_alpha_lut);
		lut = brightness_alpha_lut;
	}

	for (i = 0; i < count; i++){
		if (lut[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		alpha = lut[0].alpha;
	else if (i == count)
		alpha = lut[count - 1].alpha;
	else
		alpha = interpolate(brightness, lut[i-1].brightness,
				    lut[i].brightness, lut[i-1].alpha,
				    lut[i].alpha, display->panel->oplus_priv.bl_interpolate_nosub);

	return alpha;
}

static int bl_to_alpha_dc(int brightness)
{
	int level = ARRAY_SIZE(brightness_alpha_lut_dc);
	int i = 0;
	int alpha;

	for (i = 0; i < ARRAY_SIZE(brightness_alpha_lut_dc); i++){
		if (brightness_alpha_lut_dc[i].brightness >= brightness)
			break;
	}

	if (i == 0)
		alpha = brightness_alpha_lut_dc[0].alpha;
	else if (i == level)
		alpha = brightness_alpha_lut_dc[level - 1].alpha;
	else
		alpha = interpolate(brightness,
			brightness_alpha_lut_dc[i-1].brightness,
			brightness_alpha_lut_dc[i].brightness,
			brightness_alpha_lut_dc[i-1].alpha,
			brightness_alpha_lut_dc[i].alpha, false);
	return alpha;
}

static int brightness_to_alpha(int brightness)
{
	int alpha;

	if (brightness == 0 || brightness == 1)
		brightness = oplus_last_backlight;

	if (oplus_dimlayer_hbm)
		alpha = bl_to_alpha(brightness);
	else
		alpha = bl_to_alpha_dc(brightness);

	return alpha;
}

extern int oplus_display_fix_apollo_level(void);

static int oplus_find_index_invmaplist(uint32_t bl_level)
{
	int index = 0;
	int ret = -1;

	if (bl_level == p_apollo_backlight->bl_level_last) {
		index = p_apollo_backlight->bl_index_last;
		return index;
	}

	if (!p_apollo_backlight->bl_fix) {
		ret = oplus_display_fix_apollo_level();
		if (ret < 0) {
			return ret;
		}
	}

	for (index; index < p_apollo_backlight->bl_id_lens; index++) {
		if (p_apollo_backlight->apollo_bl_list[index] == bl_level) {
			p_apollo_backlight->bl_index_last = index;
			p_apollo_backlight->bl_level_last = bl_level;
			return index;
		}
	}

	pr_err("%s error\n", __func__);
	return -1;
}
static int oplus_get_panel_brightness_to_alpha(void)
{
	struct dsi_display *display = get_main_display();
	int index = 0;
	uint32_t brightness_panel = 0;

	if (!display)
		return 0;
	if (oplus_panel_alpha)
		return oplus_panel_alpha;

	if (hbm_mode)
		return 0;

	if (!oplus_ffl_trigger_finish)
		return brightness_to_alpha(FFL_FP_LEVEL);

	if (apollo_backlight_enable) {
		index = oplus_find_index_invmaplist(display->panel->bl_config.bl_level);
		DSI_DEBUG("[%s] index = %d, panel_level = %d, apollo_level = %d", __func__, index,
			p_apollo_backlight->panel_bl_list[index], p_apollo_backlight->apollo_bl_list[index]);
		if (index >= 0) {
			brightness_panel = p_apollo_backlight->panel_bl_list[index];
			return brightness_to_alpha(brightness_panel);
		}
	}

	return brightness_to_alpha(display->panel->bl_config.bl_level);
}

/*Mark.Yao@PSW.MM.Display.LCD.Stable,2019-10-24 add for fingerprint */
int dsi_panel_parse_oplus_fod_config(struct dsi_panel *panel)
{
	int rc = 0;
	int i;
	u32 length = 0;
	u32 count = 0;
	u32 size = 0;
	u32 *arr_32 = NULL;
	const u32 *arr;
	struct dsi_parser_utils *utils = &panel->utils;
	struct oplus_brightness_alpha *seq;

	if (panel->host_config.ext_bridge_mode)
		return 0;

	arr = utils->get_property(utils->data, "oplus,dsi-fod-brightness", &length);
	if (!arr) {
		DSI_DEBUG("[%s] oplus,dsi-fod-brightness  not found\n", panel->name);
		return -EINVAL;
	}

	if (length & 0x1) {
		DSI_ERR("[%s] oplus,dsi-fod-brightness length error\n", panel->name);
		return -EINVAL;
	}

	DSI_DEBUG("RESET SEQ LENGTH = %d\n", length);
	length = length / sizeof(u32);
	size = length * sizeof(u32);

	arr_32 = kzalloc(size, GFP_KERNEL);
	if (!arr_32) {
		rc = -ENOMEM;
		goto error;
	}

	rc = utils->read_u32_array(utils->data, "oplus,dsi-fod-brightness",
					arr_32, length);
	if (rc) {
		DSI_ERR("[%s] cannot read dsi-fod-brightness\n", panel->name);
		goto error_free_arr_32;
	}

	count = length / 2;
	size = count * sizeof(*seq);
	seq = kzalloc(size, GFP_KERNEL);
	if (!seq) {
		rc = -ENOMEM;
		goto error_free_arr_32;
	}

	panel->ba_seq = seq;
	panel->ba_count = count;

	for (i = 0; i < length; i += 2) {
		seq->brightness = arr_32[i];
		seq->alpha = arr_32[i + 1];
		seq++;
	}

error_free_arr_32:
	kfree(arr_32);
error:
	return rc;
}

static int dsi_panel_parse_oplus_backlight_remapping_config(struct dsi_panel *panel)
{
	int rc = 0;
	int i;
	u32 length = 0;
	u32 count = 0;
	u32 size = 0;
	u32 *arr_32 = NULL;
	const u32 *arr;
	struct dsi_parser_utils *utils = &panel->utils;
	struct oplus_brightness_alpha *bl_remap;

	if (panel->host_config.ext_bridge_mode)
		return 0;

	arr = utils->get_property(utils->data, "oplus,dsi-brightness-remapping", &length);
	if (!arr) {
		DSI_DEBUG("[%s] oplus,dsi-fod-brightness  not found\n", panel->name);
		return -EINVAL;
	}

	if (length & 0x1) {
		DSI_ERR("[%s] oplus,dsi-fod-brightness length error\n", panel->name);
		return -EINVAL;
	}

	panel->oplus_priv.bl_interpolate_nosub = utils->read_bool(utils->data,
			"oplus,bl_interpolate_nosub");

	DSI_DEBUG("RESET SEQ LENGTH = %d, interpolate_nosub = %d\n", length, panel->oplus_priv.bl_interpolate_nosub ? 1 : 0);
	length = length / sizeof(u32);
	size = length * sizeof(u32);

	arr_32 = kzalloc(size, GFP_KERNEL);
	if (!arr_32) {
		rc = -ENOMEM;
		goto error;
	}

	rc = utils->read_u32_array(utils->data, "oplus,dsi-brightness-remapping",
					arr_32, length);
	if (rc) {
		DSI_ERR("[%s] cannot read oplus,dsi-brightness-remapping\n", panel->name);
		goto error_free_arr_32;
	}

	count = length / 2;
	size = count * sizeof(*bl_remap);
	bl_remap = kzalloc(size, GFP_KERNEL);
	if (!bl_remap) {
		rc = -ENOMEM;
		goto error_free_arr_32;
	}

	panel->oplus_priv.bl_remap = bl_remap;
	panel->oplus_priv.bl_remap_count = count;

	for (i = 0; i < length; i += 2) {
		bl_remap->brightness = arr_32[i];
		bl_remap->alpha = arr_32[i + 1];
		bl_remap++;
	}

error_free_arr_32:
	kfree(arr_32);
error:
	return rc;
}

int dsi_panel_parse_oplus_config(struct dsi_panel *panel)
{
	struct dsi_parser_utils *utils = &panel->utils;
	int ret = 0;

	dsi_panel_parse_oplus_fod_config(panel);
	dsi_panel_parse_oplus_backlight_remapping_config(panel);

	panel->oplus_priv.vendor_name = utils->get_property(utils->data,
				"oplus,mdss-dsi-vendor-name", NULL);
	if (!panel->oplus_priv.vendor_name) {
		pr_err("Failed to found panel name, using dumming name\n");
		panel->oplus_priv.vendor_name = DSI_PANEL_OPLUS_DUMMY_VENDOR_NAME;
	}
	panel->oplus_priv.manufacture_name = utils->get_property(utils->data,
				"oplus,mdss-dsi-manufacture", NULL);
	if (!panel->oplus_priv.manufacture_name) {
		pr_err("Failed to found panel name, using dumming name\n");
		panel->oplus_priv.manufacture_name = DSI_PANEL_OPLUS_DUMMY_MANUFACTURE_NAME;
	}

	if (!strcmp(panel->oplus_priv.vendor_name, "ANA6706") || !strcmp(panel->oplus_priv.vendor_name, "SOFE03F")) {
		oplus_enhance_mipi_strength = true;
	} else {
		oplus_enhance_mipi_strength = false;
	}

	panel->oplus_priv.is_pxlw_iris5 = utils->read_bool(utils->data,
				"oplus,is_pxlw_iris5");
	DSI_INFO("is_pxlw_iris5: %s", panel->oplus_priv.is_pxlw_iris5 ? "true" : "false");

#ifdef OPLUS_FEATURE_AOD_RAMLESS
/* Yuwei.Zhang@MULTIMEDIA.DISPLAY.LCD, 2020/09/25, sepolicy for aod ramless */
	panel->oplus_priv.is_aod_ramless = utils->read_bool(utils->data,
			"oplus,aod_ramless");
	DSI_INFO("aod ramless mode: %s", panel->oplus_priv.is_aod_ramless ? "true" : "false");
#endif /* OPLUS_FEATURE_AOD_RAMLESS */

	apollo_backlight_enable = utils->read_bool(utils->data,
				"oplus,apollo_backlight_enable");
	DSI_INFO("apollo_backlight_enable: %s", apollo_backlight_enable ? "true" : "false");

/*#ifdef OPLUS_BUG_STABILITY*/
/*Yaqiang.Shi@RM.Display.LCD.Driver, add for 90Hz and 120Hz gamma switch*/
	gamma_switch_enable = utils->read_bool(utils->data,
                                "oplus,gamma_switch_enable");
        DSI_INFO("gamma_switch_enable: %s", gamma_switch_enable ? "true" : "false");
/*#endif*/

/*yanghanyue@RM.MM.Display.LCD.Params, 2020/12/17 add for panel osc config*/
	panel->oplus_priv.is_osc_support = utils->read_bool(utils->data, "oplus,osc-support");
	pr_info("[%s]osc mode support: %s", __func__, panel->oplus_priv.is_osc_support ? "Yes" : "Not");

	if(panel->oplus_priv.is_osc_support) {
		ret = utils->read_u32(utils->data, "oplus,mdss-dsi-osc-clk-mode0-rate",
					&panel->oplus_priv.osc_clk_mode0_rate);
		if (ret) {
			pr_err("[%s]failed get panel parameter: oplus,mdss-dsi-osc-clk-mode0-rate\n", __func__);
			panel->oplus_priv.osc_clk_mode0_rate = 0;
		}
		dynamic_osc_clock = panel->oplus_priv.osc_clk_mode0_rate;

		ret = utils->read_u32(utils->data, "oplus,mdss-dsi-osc-clk-mode1-rate",
					&panel->oplus_priv.osc_clk_mode1_rate);
		if (ret) {
			pr_err("[%s]failed get panel parameter: oplus,mdss-dsi-osc-clk-mode1-rate\n", __func__);
			panel->oplus_priv.osc_clk_mode1_rate = 0;
		}
	}
#ifdef OPLUS_FEATURE_ADFR
	/* Lauwo.Zhong@MM.Display.LCD.Feature,2021-01-14 add for adfr hardware revision compatibility */
	if (oplus_adfr_is_support()) {
		if (oplus_adfr_get_vsync_mode() == OPLUS_EXTERNAL_TE_TP_VSYNC) {
			/* power on with vsync_switch_gpio high bacause default timing is fhd OA 60hz */
			panel->vsync_switch_gpio_level = 1;
			/* default resolution is FHD when use mux switch */
			panel->cur_h_active = 1080;
		}
	}
#endif /* OPLUS_FEATURE_ADFR */

	return 0;
}

int dsi_panel_parse_oplus_mode_config(struct dsi_display_mode *mode,
				struct dsi_parser_utils *utils)
{
	int rc;
	struct dsi_display_mode_priv_info *priv_info;
	int val = 0;

	priv_info = mode->priv_info;

	rc = utils->read_u32(utils->data, "oplus,fod-on-vblank", &val);
	if (rc) {
		DSI_ERR("oplus,fod-on-vblank is not defined, rc=%d\n", rc);
		priv_info->fod_on_vblank = 0;
	} else {
		priv_info->fod_on_vblank = val;
		DSI_INFO("oplus,fod-on-vblank is %d", val);
	}

	rc = utils->read_u32(utils->data, "oplus,fod-off-vblank", &val);
	if (rc) {
		DSI_ERR("oplus,fod-on-vblank is not defined, rc=%d\n", rc);
		priv_info->fod_off_vblank = 0;
	} else {
		priv_info->fod_off_vblank = val;
		DSI_INFO("oplus,fod-off-vblank is %d", val);
	}

	return 0;
}
/* End of Mark.Yao@PSW.MM.Display.LCD.Stable,2019-10-24 add for fingerprint */

bool sde_crtc_get_dimlayer_mode(struct drm_crtc_state *crtc_state)
{
	struct sde_crtc_state *cstate;

	if (!crtc_state)
		return false;

	cstate = to_sde_crtc_state(crtc_state);
	return !!cstate->fingerprint_dim_layer;
}

bool sde_crtc_get_fingerprint_mode(struct drm_crtc_state *crtc_state)
{
	struct sde_crtc_state *cstate;

	if (!crtc_state)
		return false;

	cstate = to_sde_crtc_state(crtc_state);
	return !!cstate->fingerprint_mode;
}

bool sde_crtc_get_fingerprint_pressed(struct drm_crtc_state *crtc_state)
{
	struct sde_crtc_state *cstate;

	if (!crtc_state)
		return false;

	cstate = to_sde_crtc_state(crtc_state);
	return cstate->fingerprint_pressed;
}

int sde_crtc_set_onscreenfinger_defer_sync(struct drm_crtc_state *crtc_state, bool defer_sync)
{
	struct sde_crtc_state *cstate;

	if (!crtc_state)
		return -EINVAL;

	cstate = to_sde_crtc_state(crtc_state);
	cstate->fingerprint_defer_sync = defer_sync;
	return 0;
}

int sde_crtc_config_fingerprint_dim_layer(struct drm_crtc_state *crtc_state, int stage)
{
	struct sde_crtc_state *cstate;
	struct drm_display_mode *mode = &crtc_state->adjusted_mode;
	struct sde_hw_dim_layer *fingerprint_dim_layer;
	int alpha = oplus_get_panel_brightness_to_alpha();
	struct sde_kms *kms;
	struct dsi_display *display = get_main_display();

	kms = _sde_crtc_get_kms_(crtc_state->crtc);

	if (!kms || !kms->catalog) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}
	if (!display || !display->panel) {
		SDE_ERROR("failed get_main_display\n");
		return -EINVAL;
	}

	cstate = to_sde_crtc_state(crtc_state);

	if (cstate->num_dim_layers == SDE_MAX_DIM_LAYERS - 1) {
		pr_err("failed to get available dim layer for custom\n");
		return -EINVAL;
	}

	if ((stage + SDE_STAGE_0) >= kms->catalog->mixer[0].sblk->maxblendstages) {
		return -EINVAL;
	}
	fingerprint_dim_layer = &cstate->dim_layer[cstate->num_dim_layers];
	fingerprint_dim_layer->flags = SDE_DRM_DIM_LAYER_INCLUSIVE;
	fingerprint_dim_layer->stage = stage + SDE_STAGE_0;

	fingerprint_dim_layer->rect.x = 0;
	fingerprint_dim_layer->rect.y = 0;
	fingerprint_dim_layer->rect.w = mode->hdisplay;
	fingerprint_dim_layer->rect.h = mode->vdisplay;
	if (!strcmp(display->panel->oplus_priv.vendor_name, "AMB655X")) {
		if ((alpha >= 232) && (alpha <= 255)) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {3, 3, 3, alpha};
			} else if ((alpha >= 230) && (alpha <= 231)) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {5, 5, 5, alpha};
			} else if (alpha == 229) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {3, 3, 3, alpha};
			} else if (alpha == 228) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {5, 5, 5, alpha};
			} else if ((alpha >= 225) && (alpha <= 227)) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {6, 6, 6, alpha};
			} else if ((alpha >= 220) && (alpha <= 224)) {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {1, 1, 1, alpha};
			} else {
			fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {0, 0, 0, alpha};
			}
		} else {
		fingerprint_dim_layer->color_fill = (struct sde_mdss_color) {0, 0, 0, alpha};
		}
	cstate->fingerprint_dim_layer = fingerprint_dim_layer;
	oplus_underbrightness_alpha = alpha;

	return 0;
}

bool is_skip_pcc(struct drm_crtc *crtc)
{
	struct sde_crtc_state *cstate = NULL;
	cstate = to_sde_crtc_state(crtc->state);

	if (cstate == NULL) {
		pr_err("[%s]: error: cstate is NULL\n", __func__);
		return false;
	}

	if (OPLUS_DISPLAY_POWER_DOZE_SUSPEND == get_oplus_display_power_status() ||
	    OPLUS_DISPLAY_POWER_DOZE == get_oplus_display_power_status() ||
	    sde_crtc_get_fingerprint_mode(crtc->state) ||
	    (cstate->aod_skip_pcc == true)) {
		return true;
	}

	return false;
}

bool sde_cp_crtc_update_pcc(struct drm_crtc *crtc)
{
	struct sde_hw_cp_cfg hw_cfg;
	struct sde_hw_dspp *hw_dspp;
	struct sde_hw_mixer *hw_lm;
	struct sde_crtc *sde_crtc = to_sde_crtc(crtc);
	struct sde_mdss_cfg *catalog = NULL;
	u32 num_mixers = sde_crtc->num_mixers;
	bool pcc_skip_mode;
	int i = 0;

	if (!is_dsi_panel(&sde_crtc->base))
		return false;

	pcc_skip_mode = is_skip_pcc(crtc);
	if (oplus_skip_pcc == pcc_skip_mode)
		return false;

	oplus_skip_pcc = pcc_skip_mode;
	memset(&hw_cfg, 0, sizeof(hw_cfg));

	if (!pcc_skip_mode && oplus_pcc_enabled){
		hw_cfg.payload = &oplus_save_pcc;
		hw_cfg.len = sizeof(oplus_save_pcc);
	}

	hw_cfg.num_of_mixers = sde_crtc->num_mixers;
	hw_cfg.last_feature = 0;

	for (i = 0; i < num_mixers; i++) {
		hw_dspp = sde_crtc->mixers[i].hw_dspp;
		if (!hw_dspp || i >= DSPP_MAX)
			continue;
		hw_cfg.dspp[i] = hw_dspp;
	}

	catalog = get_kms_(&sde_crtc->base)->catalog;
	hw_cfg.broadcast_disabled = catalog->dma_cfg.broadcast_disabled;
	for (i = 0; i < num_mixers; i++) {

		hw_lm = sde_crtc->mixers[i].hw_lm;
		hw_dspp = sde_crtc->mixers[i].hw_dspp;
		if (!hw_lm)
			continue;
		if (!hw_dspp || !hw_dspp->ops.setup_pcc)
			continue;

		hw_cfg.ctl = sde_crtc->mixers[i].hw_ctl;
		hw_cfg.mixer_info = hw_lm;
		hw_cfg.displayh = num_mixers * hw_lm->cfg.out_width;
		hw_cfg.displayv = hw_lm->cfg.out_height;
		hw_dspp->ops.setup_pcc(hw_dspp, &hw_cfg);
	}
	return true;
}


bool _sde_encoder_setup_dither_for_onscreenfingerprint(struct sde_encoder_phys *phys,
						  void *dither_cfg, int len, struct sde_hw_pingpong *hw_pp)
{
	struct drm_encoder *drm_enc = phys->parent;
	struct drm_msm_dither dither;

	if (!drm_enc || !drm_enc->crtc)
		return -EFAULT;

	if (!sde_crtc_get_dimlayer_mode(drm_enc->crtc->state))
		return -EINVAL;

	if (len != sizeof(dither))
		return -EINVAL;

	if (oplus_get_panel_brightness_to_alpha() < oplus_dimlayer_dither_threshold)
		return -EINVAL;

	if(hw_pp == 0){
		return 0;
	}

	memcpy(&dither, dither_cfg, len);
	dither.c0_bitdepth = 6;
	dither.c1_bitdepth = 6;
	dither.c2_bitdepth = 6;
	dither.c3_bitdepth = 6;
	dither.temporal_en = 1;

	phys->hw_pp->ops.setup_dither(hw_pp, &dither, len);

	return 0;
}

int sde_plane_check_fingerprint_layer(const struct drm_plane_state *drm_state)
{
	struct sde_plane_state *pstate;

	if (!drm_state)
		return 0;

	pstate = to_sde_plane_state(drm_state);

	return sde_plane_get_property(pstate, PLANE_PROP_CUSTOM);
}

int oplus_display_panel_get_dimlayer_hbm(void *data)
{
	uint32_t *dimlayer_hbm = data;

	(*dimlayer_hbm) = oplus_dimlayer_hbm;

	return 0;
}

int oplus_display_panel_set_dimlayer_hbm(void *data)
{
	struct dsi_display *display = get_main_display();
	struct drm_connector *dsi_connector = display->drm_conn;
	uint32_t *dimlayer_hbm = data;
	int err = 0;
	int value = (*dimlayer_hbm);

	value = !!value;
	if (oplus_dimlayer_hbm == value)
		return 0;
	if (!dsi_connector || !dsi_connector->state || !dsi_connector->state->crtc) {
		pr_err("[%s]: display not ready\n", __func__);
	} else {
		err = drm_crtc_vblank_get(dsi_connector->state->crtc);
		if (err) {
			pr_err("failed to get crtc vblank, error=%d\n", err);
		} else {
			/* do vblank put after 5 frames */
			oplus_dimlayer_hbm_vblank_count = 5;
			atomic_inc(&oplus_dimlayer_hbm_vblank_ref);
		}
	}
	oplus_dimlayer_hbm = value;

#ifdef VENDOR_EDIT
/* Hu Jie@PSW.MM.Display.Lcd.Stability, 2019-09-27, add log at display key evevnt */
	pr_err("debug for oplus_display_set_dimlayer_hbm set oplus_dimlayer_hbm = %d\n", oplus_dimlayer_hbm);
#endif

	return 0;
}

