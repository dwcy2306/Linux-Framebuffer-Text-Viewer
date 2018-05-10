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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

//#include <linux/videodev2.h>
#include "videodev2.h"
#include "videodev2_s5p.h"
#include "hdmi_api.h"
#include "hdmi_lib.h"

//#include <hardware/copybit.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "libhdmi"
//#include <cutils/log.h>
#include <linux/fb.h>

#include "libcec.h"
#include "libedid.h"

#define LOGV printf
#define LOGE printf
#define LOGD printf

#define SEC_G2D_DEV_NAME		"/dev/sec-g2d"

#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')
#define VIDIOC_HDCP_STATUS _IOR('V', 101, unsigned int)
#define VIDIOC_INIT_AUDIO _IOR('V', 103, unsigned int)
#define VIDIOC_AV_MUTE _IOR('V', 104, unsigned int)
#define VIDIOC_G_AVMUTE _IOR('V', 105, unsigned int)

#define V4L2_OUTPUT_TYPE_HDMI			V4L2_OUTPUT_TYPE_DIGITAL
//#define V4L2_OUTPUT_TYPE_HDMI_RGB		11
//#define V4L2_OUTPUT_TYPE_DVI			12


int initCEC(void);
int deinitCEC(void);

int hdmi_rotate = 0;

//unsigned int output_type = V4L2_OUTPUT_TYPE_DIGITAL;
unsigned int output_type = V4L2_OUTPUT_TYPE_DVI;
//v4l2_std_id t_std_id     = V4L2_STD_1080P_30;
v4l2_std_id t_std_id     = V4L2_STD_720P_60;
//v4l2_std_id t_std_id     = V4L2_STD_480P_60_16_9;
//unsigned int HDMI_WIDTH  = 1920;
//unsigned int HDMI_HEIGHT = 1080;
//unsigned int HDMI_WIDTH  = 720;
//unsigned int HDMI_HEIGHT = 480;
unsigned int HDMI_WIDTH  = 1280;
unsigned int HDMI_HEIGHT = 720;

static sec_g2d_t s5p_g2d;
static int createG2D(sec_g2d_t *t_g2d);
static int destroyG2D(sec_g2d_t *sec_g2d);
static inline int colorFormatCopybit2G2D(int format);
static int getG2D_memory(sec_g2d_t *t_g2d);
static int g2d_reserved_memory = 0;
static int lcd_width = 0;
static int lcd_height = 0;
static int prev_phy_lcd_addr = 0;

int hdmi_audio_init( int fp )
{
	int ret;
	int sel;

	sel = HDMI_AUDIO_PCM;

	ret = ioctl(fp, VIDIOC_INIT_AUDIO, sel );

	if (ret < 0) {
		LOGE("hdmi_audio_init failed %d\n", errno);
		return ret;
	}

}

int tvout_v4l2_cropcap(int fp, struct v4l2_cropcap *a)
{
	struct v4l2_cropcap *cropcap = a;
	int ret;

	ret = ioctl(fp, VIDIOC_CROPCAP, cropcap);

	if (ret < 0) {
		LOGE("tvout_v4l2_cropcap" "VIDIOC_CROPCAP failed %d\n", errno);
		return ret;
	}

#if 0
	LOGV("tvout_v4l2_cropcap" "bound width : %d, bound height : %d,\n", 
			cropcap->bounds.width, cropcap->bounds.height);
#endif
	return ret;
}

int tvout_v4l2_querycap(int fp)
{
	struct v4l2_capability cap;
	int ret;

	ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

	if (ret < 0) {
		LOGE("tvout_v4l2_querycap" "VIDIOC_QUERYCAP failed %d\n", errno);
		return ret;
	}

	//if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
	//	printf("no overlay devices\n");
	//	return ret;
	//}

	LOGV("tvout_v4l2_querycap" "DRIVER : %s, CARD : %s, CAP.: 0x%08x\n", 
			cap.driver, cap.card, cap.capabilities);

	return ret;
}

/*
   ioctl VIDIOC_G_STD, VIDIOC_S_STD
   To query and select the current video standard applications use the VIDIOC_G_STD and
   VIDIOC_S_STD ioctls which take a pointer to a v4l2_std_id type as argument. VIDIOC_G_STD can
   return a single flag or a set of flags as in struct v4l2_standard field id
   */

int tvout_v4l2_g_std(int fp, v4l2_std_id *std_id)
{
	int ret;

	ret = ioctl(fp, VIDIOC_G_STD, std_id);
	if (ret < 0) {
		LOGE("tvout_v4l2_g_std" "VIDIOC_G_STD failed %d\n", errno);
		return ret;
	}

	return ret;
}

int tvout_v4l2_s_std(int fp, v4l2_std_id std_id)
{
	int ret;

	ret = ioctl(fp, VIDIOC_S_STD, &std_id);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_std" "VIDIOC_S_STD failed %d\n", errno);
		return ret;
	}

	return ret;
}

/*
   ioctl VIDIOC_ENUMSTD
   To query the attributes of a video standard, especially a custom (driver defined) one, applications
   initialize the index field of struct v4l2_standard and call the VIDIOC_ENUMSTD ioctl with a pointer
   to this structure. Drivers fill the rest of the structure or return an EINVAL error code when the index
   is out of bounds.
   */
int tvout_v4l2_enum_std(int fp, struct v4l2_standard *std, v4l2_std_id std_id)
{
	std->index = 0;
	while (0 == ioctl (fp, VIDIOC_ENUMSTD, std)) {
		if (std->id & std_id) {
			LOGV("tvout_v4l2_s_std> Current video standard: %s\n", std->name);
		}
		std->index++;
	}

	return 0;
}

/*
   ioctl VIDIOC_ENUMOUTPUT
   To query the attributes of a video outputs applications initialize the index field of struct v4l2_output
   and call the VIDIOC_ENUMOUTPUT ioctl with a pointer to this structure. Drivers fill the rest of the
   structure or return an EINVAL error code when the index is out of bounds
   */
int tvout_v4l2_enum_output(int fp, struct v4l2_output *output)
{
	int ret;

	ret = ioctl(fp, VIDIOC_ENUMOUTPUT, output);

	if(ret >=0) {
		LOGV("tvout_v4l2_enum_output" "enum. output [index = %d] :: type : 0x%08x , name = %s\n",
				output->index,output->type,output->name);
	}

	return ret;
}

/*
   ioctl VIDIOC_G_OUTPUT, VIDIOC_S_OUTPUT
   To query the current video output applications call the VIDIOC_G_OUTPUT ioctl with a pointer to an
   integer where the driver stores the number of the output, as in the struct v4l2_output index field.
   This ioctl will fail only when there are no video outputs, returning the EINVAL error code
   */
int tvout_v4l2_s_output(int fp, int index)
{
	int ret;

	ret = ioctl(fp, VIDIOC_S_OUTPUT, &index);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_output" "VIDIOC_S_OUTPUT failed %d\n", errno);
		return ret;
	}

	return ret;
}

int tvout_v4l2_g_output(int fp, int *index)
{
	int ret;

	ret = ioctl(fp, VIDIOC_G_OUTPUT, index);
	if (ret < 0) {
		LOGE("tvout_v4l2_g_output>" "VIDIOC_G_OUTPUT failed %d\n", errno);
		return ret;
	}else{
		LOGV("tvout_v4l2_g_output>" "Current output index %d\n", *index);
	}

	return ret;
}

/*
   ioctl VIDIOC_ENUM_FMT
   To enumerate image formats applications initialize the type and index field of struct v4l2_fmtdesc
   and call the VIDIOC_ENUM_FMT ioctl with a pointer to this structure. Drivers fill the rest of the
   structure or return an EINVAL error code. All formats are enumerable by beginning at index zero
   and incrementing by one until EINVAL is returned.
   */
int tvout_v4l2_enum_fmt(int fp, struct v4l2_fmtdesc *desc)
{
	desc->index = 0;
	while (0 == ioctl(fp, VIDIOC_ENUM_FMT, desc)) {
		LOGV("tvout_v4l2_enum_fmt" "enum. fmt [id : 0x%08x] :: type = 0x%08x, name = %s, pxlfmt = 0x%08x\n",
				desc->index,
				desc->type,
				desc->description,
				desc->pixelformat);
		desc->index++;
	}

	return 0;
}

int tvout_v4l2_g_fmt(int fp, int buf_type, void* ptr)
{
	int ret;
	struct v4l2_format format;
	struct v4l2_pix_format_s5p_tvout *fmt_param = (struct v4l2_pix_format_s5p_tvout*)ptr;

	format.type = (enum v4l2_buf_type)buf_type;

	ret = ioctl(fp, VIDIOC_G_FMT, &format);
	if (ret < 0) {
		LOGE("tvout_v4l2_g_fmt" "type : %d, VIDIOC_G_FMT failed %d\n", buf_type, errno);
		return ret;
	}else{
		memcpy(fmt_param, format.fmt.raw_data, sizeof(struct v4l2_pix_format_s5p_tvout));
		LOGV("tvout_v4l2_g_fmt" "get. fmt [base_c : 0x%08x], [base_y : 0x%08x] type = 0x%08x, width = %d, height = %d\n",
				fmt_param->base_c,
				fmt_param->base_y,
				fmt_param->pix_fmt.pixelformat,
				fmt_param->pix_fmt.width,
				fmt_param->pix_fmt.height);
	}

	return 0;
}

int tvout_v4l2_s_fmt(int fp, int buf_type, void *ptr)
{
	struct v4l2_format format;
	int ret;

	format.type = (enum v4l2_buf_type)buf_type;
	switch(buf_type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		{
			struct v4l2_pix_format_s5p_tvout *fmt_param = (struct v4l2_pix_format_s5p_tvout*)ptr;
			memcpy(format.fmt.raw_data, fmt_param, sizeof(struct v4l2_pix_format_s5p_tvout));
			break;
		}
	default:
		break;
	}
	ret = ioctl(fp, VIDIOC_S_FMT, &format);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_fmt" "[tvout_v4l2_s_fmt] : type : %d, VIDIOC_S_FMT failed %d\n", buf_type, errno);
		return ret;
	}
	return 0;
}

int tvout_v4l2_g_parm(int fp, int buf_type, void *ptr)
{
	int ret;
	struct v4l2_streamparm parm;
	struct v4l2_window_s5p_tvout *vparm = (struct v4l2_window_s5p_tvout*)ptr;

	parm.type = (enum v4l2_buf_type)buf_type;

	ret = ioctl(fp, VIDIOC_G_PARM, &parm);

	if (ret < 0) {
		LOGE("tvout_v4l2_g_parm" "type : %d, VIDIOC_G_PARM failed %d\n", buf_type, errno);
		return ret;
	}else{
		memcpy(vparm, parm.parm.raw_data, sizeof(struct v4l2_pix_format_s5p_tvout));
		LOGV("tvout_v4l2_g_parm" "get. param : width  = %d, height = %d\n",
				vparm->win.w.width,
				vparm->win.w.height);
	}
	return 0;
}

int tvout_v4l2_s_parm(int fp, int buf_type, void *ptr)
{
	struct v4l2_streamparm parm;
	struct v4l2_window_s5p_tvout *vparm = (struct v4l2_window_s5p_tvout*)ptr;
	int ret;

	parm.type = (enum v4l2_buf_type)buf_type;
	memcpy(parm.parm.raw_data, vparm, sizeof(struct v4l2_window_s5p_tvout));

	ret = ioctl(fp, VIDIOC_S_PARM, &parm);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_parm" "VIDIOC_S_PARM failed %d\n", errno);
		return ret;
	}

	return 0;
}

int tvout_v4l2_g_fbuf(int fp, struct v4l2_framebuffer *frame)
{
	int ret;

	ret = ioctl(fp, VIDIOC_G_FBUF, frame);
	if (ret < 0) {
		LOGE("tvout_v4l2_g_fbuf" "VIDIOC_STREAMON failed %d\n", errno);
		return ret;
	}

	LOGV("tvout_v4l2_g_fbuf" "get. fbuf: base = 0x%08X, pixel format = %d\n",
			frame->base, 
			frame->fmt.pixelformat);
	return 0;
}

int tvout_v4l2_s_fbuf(int fp, struct v4l2_framebuffer *frame)
{
	int ret;

	ret = ioctl(fp, VIDIOC_S_FBUF, frame);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_fbuf" "VIDIOC_STREAMON failed %d\n", errno);
		return ret;
	}
	return 0;
}

int tvout_v4l2_g_crop(int fp, unsigned int type, struct v4l2_rect *rect)
{
	int ret;
	struct v4l2_crop crop;
	crop.type = (enum v4l2_buf_type)type;
	ret = ioctl(fp, VIDIOC_G_CROP, &crop);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_crop" "VIDIOC_S_CROP failed %d\n", errno);
		return ret;
	}

	rect->left	= crop.c.left;
	rect->top	= crop.c.top;
	rect->width	= crop.c.width;
	rect->height	= crop.c.height;

#if 0
	LOGV("tvout_v4l2_g_crop" "get. crop : left = %d, top = %d, width  = %d, height = %d\n",
			rect->left, 
			rect->top,
			rect->width,
			rect->height);
#endif
	return 0;
}

int tvout_v4l2_s_crop(int fp, unsigned int type, struct v4l2_rect *rect)
{
	struct v4l2_crop crop;
	int ret;

	crop.type 	= (enum v4l2_buf_type)type;

	crop.c.left 	= rect->left;
	crop.c.top 	= rect->top;
	crop.c.width 	= rect->width;
	crop.c.height 	= rect->height;

	ret = ioctl(fp, VIDIOC_S_CROP, &crop);
	if (ret < 0) {
		LOGE("tvout_v4l2_s_crop" "VIDIOC_S_CROP failed %d\n", errno);
		return ret;
	}

	return 0;
}

int tvout_v4l2_streamon(int fp, unsigned int type)
{
	int ret;

	ret = ioctl(fp, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		LOGE("tvout_v4l2_streamon" "VIDIOC_STREAMON failed %d\n", errno);
		return ret;
	}

	LOGV("tvout_v4l2_streamon" "requested streamon buftype[id : 0x%08x]\n", type);
	return 0;
}

int tvout_v4l2_streamoff(int fp, unsigned int type)
{
	int ret;

	ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
	if (ret < 0) {
		LOGE("tvout_v4l2_streamoff""VIDIOC_STREAMOFF failed \n");
		return ret;
	}

	LOGV("tvout_v4l2_streamoff" "requested streamoff buftype[id : 0x%08x]\n", type);
	return 0;
}


int tvout_v4l2_start_overlay(int fp)
{
	int ret, start = 1;

	ret = ioctl(fp, VIDIOC_OVERLAY, &start);
	if (ret < 0) {
		LOGE("tvout_v4l2_start_overlay" "VIDIOC_OVERLAY failed\n");
		return ret;
	}

	return ret;
}

int tvout_v4l2_stop_overlay(int fp)
{
	int ret, stop =0;

	ret = ioctl(fp, VIDIOC_OVERLAY, &stop);
	if (ret < 0) 
	{
		LOGE("tvout_v4l2_stop_overlay" "VIDIOC_OVERLAY failed\n");
		return ret;
	}

	return ret;
}

struct v4l2_s5p_video_param
{
	unsigned short b_win_blending;
	unsigned int ui_alpha;
	unsigned int ui_priority;
	unsigned int ui_top_y_address;
	unsigned int ui_top_c_address;
	struct v4l2_rect rect_src;
	unsigned short src_img_endian;
};

struct tvout_param {
	struct v4l2_pix_format_s5p_tvout    tvout_src;
	struct v4l2_window_s5p_tvout        tvout_rect;
	struct v4l2_rect            tvout_dst;
};

struct overlay_param {
	struct v4l2_framebuffer 		overlay_frame;
	struct v4l2_window_s5p_tvout 	overlay_rect;
	struct v4l2_rect				overlay_dst;	
};

#define TVOUT_DEV	"/dev/video14"
#define TVOUT_DEV_G0	"/dev/video21"
#define TVOUT_DEV_G1	"/dev/video22"

static int fp_tvout 	= -1;
static int fp_tvout_g0	= -1;
static int fp_tvout_g1	= -1;

struct tvout_param tv_param;
static int f_vp_stream_on = 0;
static int f_grp0_stream_on = 0;
static int f_grp1_stream_on = 0;

unsigned int init_tv(void)
{
	//unsigned int fp_tvout;
	int ret;
	struct v4l2_output output;
	struct v4l2_standard std;
	v4l2_std_id std_g_id;
	v4l2_std_id std_id = t_std_id;
	unsigned int matched=0, i=0;
	int output_index;
	int hdcp_en = 0;

	// It was initialized already
	if(fp_tvout != -1) return fp_tvout;

	fp_tvout = open(TVOUT_DEV, O_RDWR);
	if (fp_tvout < 0) {
		LOGE("tvout video drv open failed:%s\n", strerror(errno));
		return fp_tvout;
	}

	// hdcp on
	// if(hdcp_flag)
	//ioctl(fp_tvout, VIDIOC_HDCP_ENABLE, 1);

	/* ============== query capability============== */
	tvout_v4l2_querycap(fp_tvout);

	tvout_v4l2_enum_std(fp_tvout, &std, std_id);

	// set std
	tvout_v4l2_s_std(fp_tvout, std_id);
	tvout_v4l2_g_std(fp_tvout, &std_g_id);
	
	i = 0;

	do {
		output.index = i;
		ret = tvout_v4l2_enum_output(fp_tvout, &output);
		if(output.type == output_type) {
			matched = 1;
			break;
		}
		i++;
	}while(ret >=0);

	if(!matched) {
		LOGE("no matched output type [type : 0x%08x]\n", output_type);
		return ret;
	}

	// set output
	tvout_v4l2_s_output(fp_tvout, output.index);
	output_index = 0;
	tvout_v4l2_g_output(fp_tvout, &output_index);

	//hdmi_audio_init( fp_tvout );
	printf("### HDMI Cable Status:%d\n", hdmi_cable_status());

	return fp_tvout;
}

int hdmi_set_v_param(unsigned int ui_top_y_address, unsigned int ui_top_c_address, int i_width, int i_height)
{
	tv_param.tvout_src.base_y = (void *)ui_top_y_address;
	tv_param.tvout_src.base_c = (void *)ui_top_c_address;
	tv_param.tvout_src.pix_fmt.field = V4L2_FIELD_NONE;
	tv_param.tvout_src.pix_fmt.width = i_width;
	tv_param.tvout_src.pix_fmt.height = i_height;
	tvout_v4l2_s_fmt(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_src);

	tv_param.tvout_rect.win.w.width = i_width;
	tv_param.tvout_rect.win.w.height = i_height;
	tvout_v4l2_s_parm(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_rect);

	if ( i_width * HDMI_HEIGHT >= i_height * HDMI_WIDTH ) {
		tv_param.tvout_dst.left = 0;
		tv_param.tvout_dst.top = (HDMI_HEIGHT - ((HDMI_WIDTH * i_height) / i_width)) / 2;
		tv_param.tvout_dst.width = HDMI_WIDTH;
		tv_param.tvout_dst.height = ((HDMI_WIDTH * i_height) / i_width);
	} else {
		tv_param.tvout_dst.left = (HDMI_WIDTH - ((HDMI_HEIGHT * i_width) / i_height)) / 2;
		tv_param.tvout_dst.top = 0;
		tv_param.tvout_dst.width = ((HDMI_HEIGHT * i_width) / i_height);
		tv_param.tvout_dst.height = HDMI_HEIGHT;
	}

	tvout_v4l2_s_crop(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_dst);

	//LOGD("hdmi_set_v_param() - yaddress: [0x%08x], caddress[0x%08x], %d\n", ui_top_y_address, ui_top_c_address, f_vp_stream_on);
	if (!f_vp_stream_on) {
		tvout_v4l2_streamon(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
		f_vp_stream_on = 1;
	}

	//LOGV("hdmi_set_v_param() - yaddress: [0x%08x], caddress[0x%08x]\n", ui_top_y_address, ui_top_c_address);

	return 0;
}

int hdmi_set_v_param_dst(unsigned int ui_top_y_address, unsigned int ui_top_c_address, int i_width, int i_height, int dst_left, int dst_top, int dst_width, int dst_height)
{
	tv_param.tvout_src.base_y = (void *)ui_top_y_address;
	tv_param.tvout_src.base_c = (void *)ui_top_c_address;
	tv_param.tvout_src.pix_fmt.field = V4L2_FIELD_NONE;
	tv_param.tvout_src.pix_fmt.width = i_width;
	tv_param.tvout_src.pix_fmt.height = i_height;
	tvout_v4l2_s_fmt(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_src);

	tv_param.tvout_rect.win.w.width = i_width;
	tv_param.tvout_rect.win.w.height = i_height;
	tvout_v4l2_s_parm(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_rect);

	tv_param.tvout_dst.left = dst_left;
	tv_param.tvout_dst.top = dst_top;
	tv_param.tvout_dst.width = dst_width;
	tv_param.tvout_dst.height = dst_height;

	tvout_v4l2_s_crop(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &tv_param.tvout_dst);

	//LOGD("hdmi_set_v_param() - yaddress: [0x%08x], caddress[0x%08x], %d\n", ui_top_y_address, ui_top_c_address, f_vp_stream_on);
	if (!f_vp_stream_on) {
		tvout_v4l2_streamon(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
		f_vp_stream_on = 1;
	}

	//LOGV("hdmi_set_v_param() - yaddress: [0x%08x], caddress[0x%08x]\n", ui_top_y_address, ui_top_c_address);

	return 0;
}

int hdmi_initialize()
{
	struct tvout_param tv_g_param;
	struct v4l2_fmtdesc desc;

	fp_tvout = init_tv();

	createG2D(&s5p_g2d);

	// initialize tv_param data
	//set fmt param
	tv_param.tvout_src.base_y = (void *)0x0;
	tv_param.tvout_src.base_c = (void *)0x0;
	tv_param.tvout_src.pix_fmt.width = 0;
	tv_param.tvout_src.pix_fmt.height = 0;
	tv_param.tvout_src.pix_fmt.field = V4L2_FIELD_NONE;
	tv_param.tvout_src.pix_fmt.pixelformat = VPROC_SRC_COLOR_TILE_NV12;
	//tv_param.tvout_src.pix_fmt.pixelformat = VPROC_SRC_COLOR_NV12;
	desc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	tvout_v4l2_enum_fmt(fp_tvout, &desc);

	//set window param
	tv_param.tvout_rect.flags = 0;
	tv_param.tvout_rect.priority = 1;
	tv_param.tvout_rect.win.w.left = 0;
	tv_param.tvout_rect.win.w.top = 0;
	tv_param.tvout_rect.win.w.width = 0;
	tv_param.tvout_rect.win.w.height = 0;

	f_vp_stream_on = 0;

	return 0;
}

int hdmi_deinitialize()
{
	if(fp_tvout != -1) {
		if(f_vp_stream_on == 1) {
			tvout_v4l2_streamoff(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
			f_vp_stream_on = 0;
		}
		close(fp_tvout);
		fp_tvout = -1;
	}

	destroyG2D(&s5p_g2d);

	return 0;
}

//=============================
// 

static int open_tvout(char *fp_name)
{
	int fp;

	fp = open(fp_name, O_RDWR);
	if (fp < 0) {
		LOGE("tvout graphic drv open failed!!\n");
		return fp;
	}

	return fp;
}

static int set_overlay_param(int layer, void *param)
{
	int fptv;

	struct overlay_param *vo_param = param;
	struct overlay_param vo_g_param;
	struct v4l2_cropcap cropcap;

	if(layer == 0)
		fptv = fp_tvout_g0;
	else
		fptv = fp_tvout_g1;

	tvout_v4l2_s_fbuf(fptv, &(vo_param->overlay_frame));
	//tvout_v4l2_g_fbuf(fptv, &(vo_g_param.overlay_frame));

	tvout_v4l2_s_parm(fptv, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY, &(vo_param->overlay_rect));
	//tvout_v4l2_g_parm(fptv, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY, &(vo_g_param.overlay_rect));	//test get param

	cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
	tvout_v4l2_cropcap(fptv, &cropcap);
	if(vo_param->overlay_dst.width <= cropcap.bounds.width && vo_param->overlay_dst.height <= cropcap.bounds.height){
		tvout_v4l2_s_crop(fptv, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,&(vo_param->overlay_dst));
		tvout_v4l2_g_crop(fptv, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,&(vo_g_param.overlay_dst));
	}
	else{
		LOGE("[%s] invalid crop size dst.w=%d bounds.w=%d dst.h=%d bounds.h=%d\n", __func__, vo_param->overlay_dst.width, cropcap.bounds.width, vo_param->overlay_dst.height, cropcap.bounds.height);
		return -1;;
	}

	return 1;
}

int hdmi_gl_initialize(int layer)
{
	fp_tvout = init_tv();

	if(layer == 0) {
		if(fp_tvout_g0 < 0) {
			fp_tvout_g0 	= open_tvout(TVOUT_DEV_G0);
			f_grp0_stream_on  = 0;
		}

		return fp_tvout_g0;	
	} else {
		if(fp_tvout_g1 < 0) {
			fp_tvout_g1 	= open_tvout(TVOUT_DEV_G1);
			f_grp1_stream_on  = 0;
		}

		return fp_tvout_g1;	
	}
}

int hdmi_gl_deinitialize(int layer)
{
	if(layer == 0) {
		if (fp_tvout_g0 >= 0) {
			close(fp_tvout_g0);
			f_grp0_stream_on  = 0;
			fp_tvout_g0 = -1;
		}
	} else {
		if (fp_tvout_g1 >= 0) {
			close(fp_tvout_g1);
			f_grp1_stream_on  = 0;
			fp_tvout_g1 = -1;
		}
	}

	return 0;
}

void hdmi_gl_set_mode(unsigned int mode)
{
	hdmi_deinitialize();
	switch (mode) {
	case 10809:
		t_std_id    = V4L2_STD_1080P_30;
		HDMI_WIDTH  = 1920;
		HDMI_HEIGHT = 1080;
		break;
	case 10801:
		t_std_id    = V4L2_STD_1080I_60;
		HDMI_WIDTH  = 1920;
		HDMI_HEIGHT = 1080;
		break;
	case 7209:
		t_std_id    = V4L2_STD_720P_60;
		HDMI_WIDTH  = 1280;
		HDMI_HEIGHT = 720;
		break;

	case 5769:
		t_std_id    = V4L2_STD_576P_50_16_9;
		HDMI_WIDTH  = 720;
		HDMI_HEIGHT = 576;
		break;
	case 4809:
		t_std_id    = V4L2_STD_480P_60_16_9;
		HDMI_WIDTH  = 720;
		HDMI_HEIGHT = 480;
		break;
	default:
		break;
	}
	hdmi_initialize();
}
int hdmi_gl_set_param(int layer, unsigned int ui_base_address, int i_width, int i_height, int chromakey_enable, int chromakey, int alpha_enable, int alpha_value, int pixel_blend)
{
	struct overlay_param ov_param;

	//ov_param.overlay_frame.fmt.pixelformat = V4L2_PIX_FMT_RGB565;
	ov_param.overlay_frame.fmt.pixelformat = V4L2_PIX_FMT_RGB32;
	ov_param.overlay_frame.base = ui_base_address;

	prev_phy_lcd_addr = ui_base_address;

	ov_param.overlay_rect.flags 	= 0;
	ov_param.overlay_rect.priority 	= 0x02;
	ov_param.overlay_rect.win.w.left 	= 0;
	ov_param.overlay_rect.win.w.top 	= 0;
	ov_param.overlay_rect.win.w.width 	= i_width;
	ov_param.overlay_rect.win.w.height 	= i_height;
	ov_param.overlay_rect.win.global_alpha 	= 0;

	if ( chromakey_enable != 0 )
	{
		ov_param.overlay_rect.flags 		+= V4L2_FBUF_FLAG_CHROMAKEY;	/* color key use */
		ov_param.overlay_rect.win.chromakey	= chromakey; 			/* color key: black(XRGB888) */
	}

	if ( alpha_enable != 0 )
	{
		ov_param.overlay_rect.flags 		+= V4L2_FBUF_FLAG_GLOBAL_ALPHA;	
		ov_param.overlay_rect.win.global_alpha = alpha_value; 			

		printf("GLOBAL ALPHA is %d\n", alpha_value);
	}

	if ( pixel_blend != 0 )
	{
		ov_param.overlay_rect.flags 		|= V4L2_FBUF_FLAG_LOCAL_ALPHA;	
		printf("PIXEL BLENDING ENABLE\n");
	}

	if ( i_width ==  HDMI_WIDTH ) {
		ov_param.overlay_dst.left   = 0;
		ov_param.overlay_dst.top    = (HDMI_HEIGHT - i_height) / 2;
	} else {
		ov_param.overlay_dst.left   = (HDMI_WIDTH - i_width) / 2;
		ov_param.overlay_dst.top    = 0;
	}
	ov_param.overlay_dst.width  = i_width;
	ov_param.overlay_dst.height = i_height;

	return set_overlay_param(layer, &ov_param);
}

int hdmi_gl_set_param_dst(int layer, unsigned int ui_base_address, int i_width, int i_height, int destLeft, int destTop, int destWidth, int destHeight)
{
	struct overlay_param ov_param;

	ov_param.overlay_frame.fmt.pixelformat = V4L2_PIX_FMT_RGB32;
	ov_param.overlay_frame.base = ui_base_address;

	ov_param.overlay_rect.flags 	= 0;
	ov_param.overlay_rect.priority 	= 0x02;
	ov_param.overlay_rect.win.w.left 	= 0;
	ov_param.overlay_rect.win.w.top 	= 0;
	ov_param.overlay_rect.win.w.width 	= i_width;
	ov_param.overlay_rect.win.w.height 	= i_height;
	ov_param.overlay_rect.win.global_alpha 	= 0;

	ov_param.overlay_dst.left   = destLeft;
	ov_param.overlay_dst.top    = destTop;
	ov_param.overlay_dst.width  = destWidth;
	ov_param.overlay_dst.height = destHeight;

	return set_overlay_param(layer, &ov_param);
}

int hdmi_gl_streamon(int layer)
{
	int ret = 0;
	if (0 == layer) {
        ret = tvout_v4l2_start_overlay(fp_tvout_g0);
        f_grp0_stream_on = 1;
	} else {
        ret = tvout_v4l2_start_overlay(fp_tvout_g1);
		f_grp1_stream_on = 1;
	}

	return ret;
}

int hdmi_gl_streamoff(int layer)
{
	int ret = 0;
	if (0 == layer) {
		ret = tvout_v4l2_stop_overlay(fp_tvout_g0);
		f_grp0_stream_on = 0;
	} else {
		ret = tvout_v4l2_stop_overlay(fp_tvout_g1);
		f_grp1_stream_on = 0;
	}

	return ret;
}

int hdmi_v_streamon()
{
	int ret = 0;
	if (fp_tvout !=-1 && !f_vp_stream_on) {
		ret = tvout_v4l2_streamon(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
		f_vp_stream_on = 1;
	}
	return ret;
}


int hdmi_v_streamoff()
{
	int ret = 0;
	if(fp_tvout != -1) {
		if(f_vp_stream_on == 1) {
			ret = tvout_v4l2_streamoff(fp_tvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
			f_vp_stream_on = 0;
		}
	}
	return ret;
}

int hdmi_cable_status()
{
	int cable_status;
	int ret = 0;

	if(fp_tvout != 0) {
		ret = ioctl(fp_tvout, VIDIOC_HDCP_STATUS, &cable_status); 
	} else {
		LOGE("%s tv-out is not initialized", __func__);
		ret = -1;
	}

	if(ret != 0)
		return -1;
	else
		return cable_status;

}

static int createG2D(sec_g2d_t *t_g2d)
{

    sec_g2d_t *sec_g2d = t_g2d;

    if(sec_g2d->dev_fd == 0)
        sec_g2d->dev_fd = open(SEC_G2D_DEV_NAME, O_RDONLY);

    if(sec_g2d->dev_fd < 0)
    {
        LOGE("%s::open(%s) fail\n", __func__, SEC_G2D_DEV_NAME);
        return -1;
    }
    memset(&sec_g2d->params, 0, sizeof(g2d_params));

    return 0;
}

static int destroyG2D(sec_g2d_t *sec_g2d)
{
    if(sec_g2d->dev_fd != 0)
        close(sec_g2d->dev_fd);
    sec_g2d->dev_fd = 0;

    return 0;
}

static inline int colorFormatCopybit2G2D(int format)
{
    switch (format)
    {
        case COPYBIT_FORMAT_RGBA_8888 :  return G2D_ABGR_8888;
        case COPYBIT_FORMAT_RGBX_8888 :  return G2D_XBGR_8888;
        case COPYBIT_FORMAT_RGB_565   :  return G2D_RGB_565;
        case COPYBIT_FORMAT_BGRA_8888 :  return G2D_ARGB_8888;
        case COPYBIT_FORMAT_RGBA_5551 :  return G2D_XBGR_1555;
        case COPYBIT_FORMAT_RGBA_4444 :  return G2D_ABGR_4444;
        default :
        LOGE("%s::not matched frame format : %d\n", __func__, format);
                                         break;
    }
    return -1;
}

int doG2D(g2d_rect *src_rect, g2d_rect *dst_rect, g2d_flag *flag)
{
    sec_g2d_t *sec_g2d = &s5p_g2d;
    g2d_params * params  = &(sec_g2d->params);

	if ( sec_g2d == NULL ) return -1;

	params->src_rect = src_rect;
	params->dst_rect = dst_rect;
	params->flag = flag;

    if(ioctl(sec_g2d->dev_fd, G2D_BLIT, params) < 0)
    {
        LOGE("%s::G2D_BLIT fail\n", __func__);
        return -1;
    }

    return 0;
}

static int getG2D_memory(sec_g2d_t *t_g2d)
{
    sec_g2d_t *      sec_g2d = t_g2d;
    int reserved_memory;

    if(ioctl(sec_g2d->dev_fd, G2D_GET_MEMORY, &reserved_memory) < 0)
    {
        LOGE("%s::S3C_G2D_GET_MEMORY fail\n", __func__);
        return -1;
    }

    return reserved_memory;
}


//================================= CEC / EDID ==========================
int isCECFirst = FALSE;
enum CECDeviceType mCecDeviceType = CEC_DEVICE_PLAYER;
unsigned char mCecBuffer[CEC_MAX_FRAME_SIZE];
int mCecLAddr;
int mCecPAddr;
int isCECInit = FALSE;

int initCEC(void)
{
	int isEDIDOpen = FALSE;

	isCECFirst = FALSE;

    if (!EDIDOpen())
    {
        LOGE("EDIDInit() failed!\n");
        goto initCEC_fail;
    }

	isEDIDOpen = TRUE;

    if (!EDIDRead())
    {
        LOGE("EDIDRead() failed!\n");
        goto initCEC_fail;
    }
	isCECInit = TRUE;

    return TRUE;

initCEC_fail :

	if ( isEDIDOpen == FALSE )
	{
		if (!EDIDClose())
			LOGE("EDIDClose() failed!\n");
	}
	return FALSE;
}

int deinitCEC(void)
{
    int ret = TRUE;

    if (!CECClose())
    {
        LOGE("CECClose() failed!\n");
        ret = FALSE;
    }

    if (!EDIDClose())
	{
		LOGE("EDIDClose() failed!\n");
		//LOGD("%s: EDID is closed!!!!", __func__);
		ret = FALSE;
	}
	isCECInit = FALSE;

	return ret;
}

int CECRun()
{
	int size;
	unsigned char lsrc, ldst, opcode;
	int flagCecOpen  = FALSE;

	unsigned char tmpBuf[128];
	int i,addr;
	int hpd;

	hpd = hdmi_cable_status();

	printf("#H:%d\n", hpd);

	//if ( isCECFirst == FALSE || hpd == 0)
	if ( isCECFirst == FALSE )
	{
/*
		for ( addr = 0; addr < 0x7F; addr++ ) 
		{
		printf("### addr:0x%X\n", addr);
			memset(tmpBuf, 0x00, 128);
			DDCRead(0xA1, 0, 128, tmpBuf);
			i= 0;
			do
			{
				LOGV("[%02X:%02X]", i, tmpBuf[i]);
				if ( ((i+1) % 20 ) == 0 ) LOGV("\n");
				i++;
			}
			while ( i < 128 );
		}
	*/

		if ( hpd == 0 )
		{
			if ( isCECInit == TRUE ) deinitCEC();

			//hdmi_gl_set_mode(7209);

			initCEC();
		}

		memset(mCecBuffer, 0, CEC_MAX_FRAME_SIZE);
		mCecLAddr = 0;

		// set to not valid physical address
		mCecPAddr = CEC_NOT_VALID_PHYSICAL_ADDRESS;

		if (!EDIDGetCECPhysicalAddress(&mCecPAddr)) {
			LOGE("Error: EDIDGetCECPhysicalAddress() failed.\n");
			goto initCEC_fail;
		}

		LOGD("Device physical address is %X.%X.%X.%X\n",
				(mCecPAddr & 0xF000) >> 12, (mCecPAddr & 0x0F00) >> 8,
				(mCecPAddr & 0x00F0) >> 4, mCecPAddr & 0x000F);

		if (!CECOpen()) {
			LOGE("CECOpen() failed!!!\n");
			goto initCEC_fail;
		}

		flagCecOpen = TRUE;

		// a logical address should only be allocated when a device
		// has a valid physical address, at all other times a device
		// should take the 'Unregistered' logical address (15)

		// if physical address is not valid device should take
		// the 'Unregistered' logical address (15)

		mCecLAddr = CECAllocLogicalAddress(mCecPAddr, mCecDeviceType);
		if (!mCecLAddr) {
			LOGE("CECAllocLogicalAddress() failed!!!\n");
			goto initCEC_fail;
		}
		isCECFirst = TRUE;

		SendCECPowerStatusReq();
		SendCECActiveSource();
		usleep(1000*1000);
	}

    size = CECReceiveMessage(mCecBuffer, CEC_MAX_FRAME_SIZE, 100000);

    if (!size) { // no data available or ctrl-c
        //LOGE("CECReceiveMessage() failed!\n");
        return TRUE;
    }

    if (size == 1)
        return TRUE;

    lsrc = mCecBuffer[0] >> 4;

    // ignore messages with src address == mCecLAddr
    if (lsrc == mCecLAddr)
        return TRUE;

    opcode = mCecBuffer[1];

    if (CECIgnoreMessage(opcode, lsrc)) {
        LOGE("### ignore message coming from address 15 (unregistered)\n");
        return TRUE;
    }

    if (!CECCheckMessageSize(opcode, size)) {
        LOGE("### invalid message size: %d(opcode: 0x%x) ###\n", size, opcode);
        return TRUE;
    }

    // check if message broadcasted/directly addressed
    if (!CECCheckMessageMode(opcode, (mCecBuffer[0] & 0x0F) == CEC_MSG_BROADCAST ? 1 : 0)) {
        LOGE("### invalid message mode (directly addressed/broadcast) ###\n");
        return TRUE;
    }

    ldst = lsrc;

    //TODO: macroses to extract src and dst logical addresses
    //TODO: macros to extract opcode

    switch (opcode)
    {
        case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
            // responce with "Report Physical Address"
            mCecBuffer[0] = (mCecLAddr << 4) | CEC_MSG_BROADCAST;
            mCecBuffer[1] = CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
            mCecBuffer[2] = (mCecPAddr >> 8) & 0xFF;
            mCecBuffer[3] = mCecPAddr & 0xFF;
            mCecBuffer[4] = mCecDeviceType;
            size = 5;
            break;

        case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
            LOGD("[CEC_OPCODE_REQUEST_ACTIVE_SOURCE]\n");
            // responce with "Active Source"
            mCecBuffer[0] = (mCecLAddr << 4) | CEC_MSG_BROADCAST;
            mCecBuffer[1] = CEC_OPCODE_ACTIVE_SOURCE;
            mCecBuffer[2] = (mCecPAddr >> 8) & 0xFF;
            mCecBuffer[3] = mCecPAddr & 0xFF;
            size = 4;
            LOGD("Tx : [CEC_OPCODE_ACTIVE_SOURCE]\n");
            break;

        case CEC_OPCODE_ABORT:
        case CEC_OPCODE_FEATURE_ABORT:
        default:
            // send "Feature Abort"
            mCecBuffer[0] = (mCecLAddr << 4) | ldst;
            mCecBuffer[1] = CEC_OPCODE_FEATURE_ABORT;
            mCecBuffer[2] = CEC_OPCODE_ABORT;
            mCecBuffer[3] = 0x04; // "refused"
            size = 4;
            break;
    }

    if (CECSendMessage(mCecBuffer, size) != size)
        LOGE("CECSendMessage() failed!!!\n");

    return TRUE;

initCEC_fail :
    if(flagCecOpen == TRUE)
    {
        if (!CECClose())
            LOGE("CECClose() failed!\n");
    }
	EDIDReset();
    return FALSE;
}

void SendCECActiveSource()
{
    unsigned char buffer[5];
    buffer[0] = (mCecLAddr << 4) | CEC_MSG_BROADCAST;
    buffer[1] = CEC_OPCODE_ACTIVE_SOURCE;
    buffer[2] = (mCecPAddr >> 8) & 0xFF;
    buffer[3] = mCecPAddr & 0xFF;

    if (CECSendMessage(buffer, 4) != 4) {
        LOGE("CECSendMessage() failed!\n");
        return 0;
    }

	printf("Send Active Source\n");
}

void SendCECPowerStatusReq()
{
	/* Request power state from TV */
	unsigned char buffer[CEC_MAX_FRAME_SIZE];
	int size;
	buffer[0] = (mCecLAddr << 4);
	buffer[1] = CEC_OPCODE_GIVE_DEVICE_POWER_STATUS;
	size = 2;
	if (CECSendMessage(buffer, size) != size)
        LOGE("CECSendMessage() failed!\n");
	printf("Send PWR Req \n");
}
