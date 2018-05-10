/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HDMI_API_H__
#define __HDMI_API_H__

#include "sec_g2d.h"

//#define BOARD_USES_HDMI_SUBTITLES
//#define BOARD_USES_OVERLAY

/**
 * pixel format definitions
 */

enum {
	COPYBIT_FORMAT_RGBA_8888    = 1,
	COPYBIT_FORMAT_RGBX_8888    = 2,
	COPYBIT_FORMAT_RGB_888      = 3,
	COPYBIT_FORMAT_RGB_565      = 4,
	COPYBIT_FORMAT_BGRA_8888    = 5,
	COPYBIT_FORMAT_RGBA_5551    = 6,
	COPYBIT_FORMAT_RGBA_4444    = 7,
	COPYBIT_FORMAT_YCbCr_422_SP = 0x10,
	COPYBIT_FORMAT_YCrCb_420_SP = 0x11,
	COPYBIT_FORMAT_YCbCr_422_P  = 0x12,
	COPYBIT_FORMAT_YCbCr_420_P  = 0x13,
	COPYBIT_FORMAT_YCbCr_422_I  = 0x14,
	COPYBIT_FORMAT_YCbCr_420_I  = 0x15,
	COPYBIT_FORMAT_CbYCrY_422_I = 0x16,
	COPYBIT_FORMAT_CbYCrY_420_I = 0x17,
	COPYBIT_FORMAT_YCbCr_420_SP_TILED = 0x20,
	COPYBIT_FORMAT_YCbCr_420_SP       = 0x21,
	COPYBIT_FORMAT_YCrCb_420_SP_TILED = 0x22,
	COPYBIT_FORMAT_YCrCb_422_SP       = 0x23,
#if defined(SLSI_S5PC110) || defined(SLSI_S5PC100)
	COPYBIT_FORMAT_CUSTOM_YCbCr_420_SP = 0x30,
	COPYBIT_FORMAT_CUSTOM_YCbCr_422_I  = 0x31,
	COPYBIT_FORMAT_CUSTOM_CbYCr_422_I  = 0x32
#endif
};

enum s5p_hdmi_audio_type {
	HDMI_AUDIO_NO,
	HDMI_AUDIO_PCM
};


#ifdef __cplusplus
extern "C" {
#endif

int tvout_v4l2_querycap(int fp);
int tvout_v4l2_g_std(int fp, v4l2_std_id *std_id);
int tvout_v4l2_s_std(int fp, v4l2_std_id std_id);
int tvout_v4l2_enum_std(int fp, struct v4l2_standard *std, v4l2_std_id std_id);
int tvout_v4l2_enum_output(int fp, struct v4l2_output *output);
int tvout_v4l2_s_output(int fp, int index);
int tvout_v4l2_g_output(int fp, int *index);
int tvout_v4l2_enum_fmt(int fp, struct v4l2_fmtdesc *desc);
int tvout_v4l2_g_fmt(int fp, int buf_type, void* ptr);
int tvout_v4l2_s_fmt(int fp, int buf_type, void *ptr);
int tvout_v4l2_g_parm(int fp, int buf_type, void *ptr);
int tvout_v4l2_s_parm(int fp, int buf_type, void *ptr);
int tvout_v4l2_g_fbuf(int fp, struct v4l2_framebuffer *frame);
int tvout_v4l2_s_fbuf(int fp, struct v4l2_framebuffer *frame);
int tvout_v4l2_g_crop(int fp, unsigned int type, struct v4l2_rect *rect);
int tvout_v4l2_s_crop(int fp, unsigned int type, struct v4l2_rect *rect);
int tvout_v4l2_streamon(int fp, unsigned int type);
int tvout_v4l2_streamoff(int fp, unsigned int type);
int tvout_v4l2_start_overlay(int fp);
int tvout_v4l2_stop_overlay(int fp);
int tvout_v4l2_cropcap(int fp, struct v4l2_cropcap *a);
int hdmi_gl_set_param(int layer, unsigned int ui_base_address, int i_width, int i_height, int chromakey_enable, int chromakey, int alpha_enable, int alpha_value, int pixel_blend);
int hdmi_gl_set_param_dst(int layer, unsigned int ui_base_address, int i_width, int i_height, int destLeft, int destTop, int destWidth, int destHeight);

//Kamat added
int hdmi_initialize();
int hdmi_set_v_param(unsigned int ui_top_y_address, unsigned int ui_top_c_address, int i_width, int i_height);
int hdmi_set_v_param_dst(unsigned int ui_top_y_address, unsigned int ui_top_c_address, int i_width, int i_height, int dst_left, int dst_top, int dst_width, int dst_height);
int hdmi_deinitialize();
int hdmi_gl_initialize(int layer);
int hdmi_gl_streamon(int layer);
int hdmi_gl_streamoff(int layer);
int hdmi_v_streamon();
int hdmi_v_streamoff();
int hdmi_cable_status();

int doG2D(g2d_rect *src_rect, g2d_rect *dst_rect, g2d_flag *flag);
int CECRun();
void SendCECActiveSource();
void SendCECPowerStatusReq();

#ifdef __cplusplus
}
#endif


#endif //__HDMI_API_H__

