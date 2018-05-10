#ifndef __SAMSUNG_SYSLSI_APDEV_HDMILIB_H__
#define __SAMSUNG_SYSLSI_APDEV_HDMILIB_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define HDMI_USES_GRAPHIC_LAYER

// video processor
typedef enum {
	VPROC_SRC_COLOR_NV12  = 0,
	VPROC_SRC_COLOR_NV12IW  = 1,
	VPROC_SRC_COLOR_TILE_NV12  = 2,
	VPROC_SRC_COLOR_TILE_NV12IW  = 3
}s5p_vp_src_color;

// functions for video layer
extern int hdmi_initialize();
extern int hdmi_deinitialize();
extern int hdmi_set_v_param(unsigned int ui_top_y_address, unsigned int ui_top_c_address, int i_width, int i_height);
// functions for graphic layer
extern int hdmi_gl_initialize(int layer);
extern int hdmi_gl_deinitialize(int layer);
extern int hdmi_gl_streamon(int layer);
extern int hdmi_gl_streamoff(int layer);
extern int hdmi_v_streamon();
extern int hdmi_v_streamoff();
extern int hdmi_cable_status();
extern void hdmi_gl_set_mode(unsigned int mode);
#ifdef __cplusplus
}
#endif

#endif //__SAMSUNG_SYSLSI_APDEV_HDMILIB_H__
