/*
 * Copyright (C) 2008 The Android Open Source Project
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

/*
 **
 ** @author siva krishna neeli(siva.neeli@samsung.com)
 ** @date   2009-02-27
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "videodev2.h"
#include "s5p_fimc.h"

#include "fimc_lib.h"

#define LOGV printf
#define LOGE printf
#define LOGD printf
#define LOGI printf

/*------------ DEFINE ---------------------------------------------------------*/
#define  FIMC2_DEV_NAME  "/dev/video/fimc1"

#define ALIGN(x, a)    (((x) + (a) - 1) & ~((a) - 1))

static s5p_fimc_t	s5p_fimc;

int		fimc_hdmi_path_disable_flag = 0;
unsigned int	fimc_hdmi_ui_layer_status = 0;
int src_req_buf_flag = 0;

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/

void *fimc_get_output_phybuf() 
{
	return s5p_fimc.out_buf.phys_addr;
}

int fimc_v4l2_set_src(int fd, unsigned int hw_ver, s5p_fimc_img_info *src)
{
	struct v4l2_format		fmt;
	struct v4l2_cropcap		cropcap;
	struct v4l2_crop		crop;
	struct v4l2_requestbuffers	req;    
	int				ret_val;

	/* 
	 * To set size & format for source image (DMA-INPUT) 
	 */

	fmt.type 			= V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width 		= src->full_width;
	fmt.fmt.pix.height 		= src->full_height;
	fmt.fmt.pix.pixelformat		= src->color_space;
	fmt.fmt.pix.field 		= V4L2_FIELD_NONE; 

	ret_val = ioctl (fd, VIDIOC_S_FMT, &fmt);
	if (ret_val < 0) {
		LOGE("VIDIOC_S_FMT failed : ret=%d\n", ret_val);
		return -1;
	}

	/* 
	 * crop input size
	 */

	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if(hw_ver == 0x50) {
		crop.c.left = src->start_x;
		crop.c.top = src->start_y;
	} else {
		crop.c.left   = 0;
		crop.c.top    = 0;
	}

	crop.c.width  = src->width;
	crop.c.height = src->height;

	ret_val = ioctl(fd, VIDIOC_S_CROP, &crop);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_CROP (%d)\n",ret_val);
		return -1;
	}

	/* 
	 * input buffer type
	 */

	if ( !src_req_buf_flag )
	{
		req.count       = 1;
		req.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		req.memory      = V4L2_MEMORY_USERPTR;

		ret_val = ioctl (fd, VIDIOC_REQBUFS, &req);
		if (ret_val < 0) {
			LOGE("Error in VIDIOC_REQBUFS (%d)\n", ret_val);
			return -1;
		}
		src_req_buf_flag = 1;
	}

	return ret_val;
}

int fimc_v4l2_set_dst(int fd, s5p_fimc_img_info *dst, int rotation, unsigned int addr)
{
	struct v4l2_format		sFormat;
	struct v4l2_control		vc;
	struct v4l2_framebuffer		fbuf;
	int				ret_val;

	/* 
	 * set rotation configuration
	 */

	vc.id = V4L2_CID_ROTATION;
	vc.value = rotation;

	ret_val = ioctl(fd, VIDIOC_S_CTRL, &vc);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_CTRL - rotation (%d)\n",ret_val);
		return -1;
	}

	/*
	 * set size, format & address for destination image (DMA-OUTPUT)
	 */
	ret_val = ioctl(fd, VIDIOC_G_FBUF, &fbuf);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_G_FBUF (%d)\n", ret_val);
		return -1;
	}

	fbuf.base 			= (void *)addr;
	fbuf.fmt.width 			= dst->full_width;
	fbuf.fmt.height 		= dst->full_height;
	fbuf.fmt.pixelformat 		= dst->color_space;

	ret_val = ioctl (fd, VIDIOC_S_FBUF, &fbuf);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_FBUF (%d)\n",ret_val);        
		return -1;
	}

	/* 
	 * set destination window
	 */

	sFormat.type 			= V4L2_BUF_TYPE_VIDEO_OVERLAY;
	sFormat.fmt.win.w.left 		= dst->start_x;
	sFormat.fmt.win.w.top 		= dst->start_y;
	sFormat.fmt.win.w.width 	= dst->width;
	sFormat.fmt.win.w.height 	= dst->height;

	ret_val = ioctl(fd, VIDIOC_S_FMT, &sFormat);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_FMT (%d)\n",ret_val);
		return -1;
	}

	return 0;
}

int fimc_v4l2_set_dst_vout(int fd, s5p_fimc_img_info *dst, int rotation, unsigned int addr_y, unsigned int addr_cb )
{
	struct v4l2_format		sFormat;
	struct v4l2_control		vc;
	struct v4l2_framebuffer		fbuf;
	int				ret_val;
	struct fimc_buf		fimc_dst_buf;

	/* 
	 * set rotation configuration
	 */

	vc.id = V4L2_CID_ROTATION;
	vc.value = rotation;

	ret_val = ioctl(fd, VIDIOC_S_CTRL, &vc);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_CTRL - rotation (%d)\n",ret_val);
		return -1;
	}


	/*
	 * set size, format & address for destination image (DMA-OUTPUT)
	 */
	ret_val = ioctl(fd, VIDIOC_G_FBUF, &fbuf);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_G_FBUF (%d)\n", ret_val);
		return -1;
	}

	fbuf.base 			= (void *)addr_y;
	fbuf.fmt.width 			= dst->full_width;
	fbuf.fmt.height 		= dst->full_height;
	fbuf.fmt.pixelformat 		= dst->color_space;

	ret_val = ioctl (fd, VIDIOC_S_FBUF, &fbuf);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_FBUF (%d)\n",ret_val);        
		return -1;
	}

	/* 
	 * set destination window
	 */

	sFormat.type 			= V4L2_BUF_TYPE_VIDEO_OVERLAY;
	sFormat.fmt.win.w.left 		= dst->start_x;
	sFormat.fmt.win.w.top 		= dst->start_y;
	sFormat.fmt.win.w.width 	= dst->width;
	sFormat.fmt.win.w.height 	= dst->height;

	ret_val = ioctl(fd, VIDIOC_S_FMT, &sFormat);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_FMT (%d)\n",ret_val);
		return -1;
	}

	vc.id = V4L2_CID_OVERLAY_C_ADDR;
	vc.value = addr_cb;

	ret_val = ioctl(fd, VIDIOC_S_CTRL, &vc);
	if (ret_val < 0) {
		LOGE("Error in video VIDIOC_S_CTRL - overlay_c_addr (%d)\n",ret_val);
		return -1;
	}

	return 0;
}


int fimc_v4l2_stream_on(int fd, enum v4l2_buf_type type)
{
	if (-1 == ioctl (fd, VIDIOC_STREAMON, &type)) {
		LOGE("Error in VIDIOC_STREAMON\n");
		return -1;
	}

	return 0;
}

int fimc_v4l2_queue(int fd, struct fimc_buf *fimc_buf)
{
	struct v4l2_buffer	buf;
	int			ret_val;

	buf.type	= V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory	= V4L2_MEMORY_USERPTR;
	buf.m.userptr	= (unsigned long)fimc_buf;
	buf.length	= 0;
	buf.index	= 0;

	ret_val = ioctl (fd, VIDIOC_QBUF, &buf);
	if (0 > ret_val) {
		LOGE("Error in VIDIOC_QBUF : (%d) \n", ret_val);
		return -1;
	}

	return 0;
}

int fimc_v4l2_dequeue(int fd)
{
	struct v4l2_buffer	buf;

	buf.type        = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory      = V4L2_MEMORY_USERPTR;

	if (-1 == ioctl (fd, VIDIOC_DQBUF, &buf)) {
		LOGE("Error in VIDIOC_DQBUF\n");
		return -1;
	}

	return buf.index;
}

int fimc_v4l2_stream_off(int fd)
{
	enum v4l2_buf_type	type;

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type)) {
		LOGE("Error in VIDIOC_STREAMOFF\n");
		return -1;
	}

	return 0;
}

int fimc_v4l2_clr_buf(int fd)
{
	struct v4l2_requestbuffers req;

	req.count   = 0;
	req.type    = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory  = V4L2_MEMORY_USERPTR;

	if (ioctl (fd, VIDIOC_REQBUFS, &req) == -1) {
		LOGE("Error in VIDIOC_REQBUFS");
	}

	return 0;
}

int fimc_handle_oneshot(int fd, struct fimc_buf *fimc_buf)
{
	int		ret;

	ret = fimc_v4l2_stream_on(fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	if(ret < 0) {
		return -1;
	}

	ret = fimc_v4l2_queue(fd, fimc_buf);  
	if(ret < 0) {
		return -1;
	}

#if 0
	ret = fimc_v4l2_dequeue(fd);
	if(ret < 0) {
		return -1;
	}
#endif

	ret = fimc_v4l2_stream_off(fd);
	if(ret < 0) {
		return -1;
	}

	ret = fimc_v4l2_clr_buf(fd);
	if(ret < 0) {
		return -1;
	}

	return 0;
}

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/

static int createPP(void)
{	
	struct v4l2_capability 	cap;
	struct v4l2_format	fmt;
	struct v4l2_control     vc;
	int			ret, index;

	/* open device file */
	if(s5p_fimc.dev_fd == 0) {
		s5p_fimc.dev_fd = open(FIMC2_DEV_NAME, O_RDWR);
		if(s5p_fimc.dev_fd < 0) {
			LOGE("%s::Post processor open error\n", __func__);
			return -1;
		}
	}

	/* check capability */
	ret = ioctl(s5p_fimc.dev_fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		LOGE("VIDIOC_QUERYCAP failed\n");
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		LOGE("%d has no streaming support\n", s5p_fimc.dev_fd);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
		LOGE("%d is no video output\n", s5p_fimc.dev_fd);
		return -1;
	}

	/*
	 * malloc fimc_outinfo structure
	 */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(s5p_fimc.dev_fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOGE("[%s] Error in video VIDIOC_G_FMT\n", __FUNCTION__);
		return -1;
	}

	/*
	 * get baes address of the reserved memory for fimc2
	 */
	vc.id = V4L2_CID_RESERVED_MEM_BASE_ADDR;
	vc.value = 0;

	ret = ioctl(s5p_fimc.dev_fd, VIDIOC_G_CTRL, &vc);
	if (ret < 0) {
		LOGE("Error in video VIDIOC_G_CTRL - V4L2_CID_RESERVED_MEM_BAES_ADDR (%d)\n",ret);
		return -1;
	}

	s5p_fimc.out_buf.phys_addr = vc.value;
	LOGI("[%s] out_buf.phys_addr=%p\n", __func__, s5p_fimc.out_buf.phys_addr);

	vc.id = V4L2_CID_FIMC_VERSION;
	vc.value = 0;

	ret = ioctl(s5p_fimc.dev_fd, VIDIOC_G_CTRL, &vc);
	if (ret < 0) {
		LOGE("Error in video VIDIOC_G_CTRL - V4L2_CID_FIMC_VERSION (%d), FIMC version is set with default\n",ret);
		vc.value = 0x43;
	}
	s5p_fimc.hw_ver = vc.value;

	printf(">>>FIMC_HW_VER:%x\n", vc.value);

	return 0;
}

static int destroyPP(void)
{
	if(s5p_fimc.out_buf.phys_addr != NULL) {
		s5p_fimc.out_buf.phys_addr = NULL;
		s5p_fimc.out_buf.length = 0;
	}

	/* close */
	if(s5p_fimc.dev_fd != 0)
		close(s5p_fimc.dev_fd);

	s5p_fimc.dev_fd = 0;

	return 0;
}

/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/

void fimc_initialize()
{
	s5p_fimc_params_t *params = &(s5p_fimc.params);

	src_req_buf_flag = 0;

	/* open */
	createPP();

	/* set post processor configuration */
	params->src.full_width  = 0;
	params->src.full_height = 0;
	params->src.start_x     = 0;
	params->src.start_y     = 0;
	params->src.width       = 0;
	params->src.height      = 0;
	params->src.color_space = 0;

	params->dst.color_space = V4L2_PIX_FMT_NV12T;
}

void fimc_pp_overlay_process(s5p_fimc_params_t 	*params, void *dst_phy_addr )
{
	//s5p_fimc_params_t 	*params = &(s5p_fimc.params);

	struct fimc_buf		fimc_src_buf;
	if(params->src.color_space <= 0) {
		LOGE("%s source image param is not set", __func__);
		return;
	}

	/* set configuration related to source (DMA-INPUT)
	 *   - set input format & size 
	 *   - crop input size 
	 *   - set input buffer
	 *   - set buffer type (V4L2_MEMORY_USERPTR)
	 */
	fimc_v4l2_set_src(s5p_fimc.dev_fd, s5p_fimc.hw_ver, &params->src); 

	/* set configuration related to destination (DMA-OUT)
	 *   - set input format & size 
	 *   - crop input size 
	 *   - set input buffer
	 *   - set buffer type (V4L2_MEMORY_USERPTR)
	 */

	if ( dst_phy_addr == NULL ) fimc_v4l2_set_dst(s5p_fimc.dev_fd, &params->dst, 0, s5p_fimc.out_buf.phys_addr); 
	else fimc_v4l2_set_dst(s5p_fimc.dev_fd, &params->dst, 0, dst_phy_addr); 

	fimc_v4l2_stream_on(s5p_fimc.dev_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
	fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
	fimc_src_buf.base[2] = params->src.buf_addr_phy_cr;

	fimc_v4l2_queue(s5p_fimc.dev_fd, &fimc_src_buf);
	fimc_v4l2_dequeue(s5p_fimc.dev_fd);
	fimc_v4l2_stream_off(s5p_fimc.dev_fd);
}

void fimc_pp_video_process(s5p_fimc_params_t *params, void *dst_phy_y, void *dst_phy_cb )
{
	//s5p_fimc_params_t 	*params = &(s5p_fimc.params);

	struct fimc_buf		fimc_src_buf;
	if(params->src.color_space <= 0) {
		LOGE("%s source image param is not set", __func__);
		return;
	}

	/* set configuration related to source (DMA-INPUT)
	 *   - set input format & size 
	 *   - crop input size 
	 *   - set input buffer
	 *   - set buffer type (V4L2_MEMORY_USERPTR)
	 */
	fimc_v4l2_set_src(s5p_fimc.dev_fd, s5p_fimc.hw_ver, &params->src); 

	/* set configuration related to destination (DMA-OUT)
	 *   - set input format & size 
	 *   - crop input size 
	 *   - set input buffer
	 *   - set buffer type (V4L2_MEMORY_USERPTR)
	 */
	fimc_v4l2_set_dst_vout(s5p_fimc.dev_fd, &params->dst, 0, dst_phy_y, dst_phy_cb); 

	fimc_v4l2_stream_on(s5p_fimc.dev_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	fimc_src_buf.base[0] = params->src.buf_addr_phy_rgb_y;
	fimc_src_buf.base[1] = params->src.buf_addr_phy_cb;
	fimc_src_buf.base[2] = params->src.buf_addr_phy_cr;

	fimc_v4l2_queue(s5p_fimc.dev_fd, &fimc_src_buf);
	fimc_v4l2_dequeue(s5p_fimc.dev_fd);
	fimc_v4l2_stream_off(s5p_fimc.dev_fd);
}

void fimc_stop()
{
	fimc_v4l2_clr_buf(s5p_fimc.dev_fd);
}

void fimc_deinitialize()
{
	destroyPP();
}

void fimc_suspend()
{
	fimc_hdmi_path_disable_flag = 1;
}

void fimc_resume()
{
	fimc_hdmi_path_disable_flag = 0;

}


#define V4L2_PIX_FMT_NV12T    v4l2_fourcc('T', 'V', '1', '2')
#define VBUF_TYPE V4L2_BUF_TYPE_VIDEO_CAPTURE

#define V4L2_CID_ROTATION       (V4L2_CID_PRIVATE_BASE + 0)
#define V4L2_CID_PADDR_Y        (V4L2_CID_PRIVATE_BASE + 1)
#define V4L2_CID_PADDR_CB       (V4L2_CID_PRIVATE_BASE + 2)
#define V4L2_CID_PADDR_CR       (V4L2_CID_PRIVATE_BASE + 3)
#define V4L2_CID_PADDR_CBCR     (V4L2_CID_PRIVATE_BASE + 4)
#define V4L2_CID_STREAM_PAUSE           (V4L2_CID_PRIVATE_BASE + 53)


static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
		struct v4l2_control ctrl;
		int ret;

		ctrl.id = id;
		ctrl.value = value;

		ret = ioctl(fp, VIDIOC_S_CTRL, &ctrl);
		if (ret < 0) {
				printf("ERR(%s):VIDIOC_S_CTRL failed\n", __FUNCTION__);
				return ret;
		}

		return ctrl.value;
}

unsigned int getPhyAddrY(int fp, int index)
{
		unsigned int addr_y;

		addr_y = fimc_v4l2_s_ctrl(fp, V4L2_CID_PADDR_Y, index);
		return addr_y;
}

unsigned int getPhyAddrC(int fp, int index)
{
		unsigned int addr_c;

		addr_c = fimc_v4l2_s_ctrl(fp, V4L2_CID_PADDR_CBCR, index);
		return addr_c;
}

