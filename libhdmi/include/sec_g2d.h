/* linux/drivers/media/video/samsung/g2d/fimg2d_3x.h
 *
 * Copyright  2008 Samsung Electronics Co, Ltd. All Rights Reserved.
 *		      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _SEC_G2D_DRIVER_H_
#define _SEC_G2D_DRIVER_H_

#define G2D_SFR_SIZE        0x1000

#define TRUE      (1)
#define FALSE     (0)

#define G2D_MINOR  240

#define G2D_IOCTL_MAGIC 'G'

#define G2D_BLIT                        _IO(G2D_IOCTL_MAGIC,0)
#define G2D_GET_VERSION                 _IO(G2D_IOCTL_MAGIC,1)
#define G2D_GET_MEMORY                  _IOR(G2D_IOCTL_MAGIC,2, unsigned int)
#define G2D_GET_MEMORY_SIZE             _IOR(G2D_IOCTL_MAGIC,3, unsigned int)
#define G2D_DMA_CACHE_INVAL	        _IOWR(G2D_IOCTL_MAGIC,4, struct g2d_dma_info)
#define G2D_DMA_CACHE_CLEAN	        _IOWR(G2D_IOCTL_MAGIC,5, struct g2d_dma_info)
#define G2D_DMA_CACHE_FLUSH	        _IOWR(G2D_IOCTL_MAGIC,6, struct g2d_dma_info)

#define G2D_MAX_WIDTH   (2048)
#define G2D_MAX_HEIGHT  (2048)

#define G2D_ALPHA_VALUE_MAX (255)

typedef enum
{
	G2D_ROT_0 = 0,
	G2D_ROT_90,
	G2D_ROT_180,
	G2D_ROT_270,
	G2D_ROT_X_FLIP,
	G2D_ROT_Y_FLIP
} G2D_ROT_DEG;

typedef enum
{
	G2D_ALPHA_BLENDING_MIN    = 0,   // wholly transparent
	G2D_ALPHA_BLENDING_MAX    = 255, // 255
	G2D_ALPHA_BLENDING_OPAQUE = 256, // opaque
} G2D_ALPHA_BLENDING_MODE;
    
typedef enum
{
	G2D_COLORKEY_NONE = 0,
	G2D_COLORKEY_SRC_ON,
	G2D_COLORKEY_DST_ON,
	G2D_COLORKEY_SRC_DST_ON,
}G2D_COLORKEY_MODE;

typedef enum
{
	G2D_BLUE_SCREEN_NONE = 0,
	G2D_BLUE_SCREEN_TRANSPARENT,
	G2D_BLUE_SCREEN_WITH_COLOR,
}G2D_BLUE_SCREEN_MODE;

typedef enum
{
	G2D_ROP_SRC = 0,
	G2D_ROP_DST,
	G2D_ROP_SRC_AND_DST,
	G2D_ROP_SRC_OR_DST,
	G2D_ROP_3RD_OPRND,
	G2D_ROP_SRC_AND_3RD_OPRND,
	G2D_ROP_SRC_OR_3RD_OPRND,
	G2D_ROP_SRC_XOR_3RD_OPRND,
	G2D_ROP_DST_OR_3RD,
}G2D_ROP_TYPE;

typedef enum
{
	G2D_THIRD_OP_NONE = 0,
	G2D_THIRD_OP_PATTERN,
	G2D_THIRD_OP_FG,
	G2D_THIRD_OP_BG
}G2D_THIRD_OP_MODE;

typedef enum
{
	G2D_BLACK = 0,
	G2D_RED,
	G2D_GREEN,
	G2D_BLUE,
	G2D_WHITE, 
	G2D_YELLOW,
	G2D_CYAN,
	G2D_MAGENTA
}G2D_COLOR;

typedef enum
{
	G2D_RGB_565 = 0,

	G2D_ABGR_8888,
	G2D_BGRA_8888,
	G2D_ARGB_8888,
	G2D_RGBA_8888,

	G2D_XBGR_8888,
	G2D_BGRX_8888,
	G2D_XRGB_8888,
	G2D_RGBX_8888,

	G2D_ABGR_1555,
	G2D_BGRA_5551,
	G2D_ARGB_1555,
	G2D_RGBA_5551,

	G2D_XBGR_1555,
	G2D_BGRX_5551,
	G2D_XRGB_1555,
	G2D_RGBX_5551,

	G2D_ABGR_4444,
	G2D_BGRA_4444,
	G2D_ARGB_4444,
	G2D_RGBA_4444,

	G2D_XBGR_4444,
	G2D_BGRX_4444,
	G2D_XRGB_4444,
	G2D_RGBX_4444,
	
	G2D_PACKED_BGR_888,
	G2D_PACKED_RGB_888,

	G2D_MAX_COLOR_SPACE
}G2D_COLOR_SPACE;

typedef struct
{
	unsigned int    x;
	unsigned int    y;
	unsigned int    w;
	unsigned int    h;
	unsigned int    full_w;
	unsigned int    full_h;
	int             color_format;
	unsigned int    phys_addr;
	unsigned char * virt_addr;
} g2d_rect;

typedef struct
{
	unsigned int    rotate_val;
	unsigned int    alpha_val;

	unsigned int    blue_screen_mode;     //true : enable, false : disable
	unsigned int    color_key_val;        //screen color value
	unsigned int    color_switch_val;     //one color

	unsigned int    src_color;            // when set one color on SRC
			
	unsigned int    third_op_mode;
	unsigned int    rop_mode;
	unsigned int    mask_mode;
} g2d_flag;

typedef struct 
{
	g2d_rect * src_rect;
	g2d_rect * dst_rect;
	g2d_flag * flag;

	/*
	u32     src_base_addr;        //Base address of the source image
	u32     src_full_width;       //source image full width
	u32     src_full_height;      //source image full height
	u32     src_start_x;          //coordinate start x of source image
	u32     src_start_y;          //coordinate start y of source image
	u32     src_work_width;       //source image width for work
	u32     src_work_height;      //source image height for work
	u32     src_color_space;

	u32     dst_base_addr;        //Base address of the destination image
	u32     dst_full_width;       //destination screen full width
	u32     dst_full_height;      //destination screen full width
	u32     dst_start_x;          //coordinate start x of destination screen
	u32     dst_start_y;          //coordinate start y of destination screen
	u32     dst_work_width;       //destination screen width for work
	u32     dst_work_height;      //destination screen height for work
	u32     dst_color_space;

	// Coordinate (X, Y) of clipping window
	u32     cw_x1, cw_y1;
	u32     cw_x2, cw_y2;

	u32     rotate_val;

	u32     alpha_mode;           //true : enable, false : disable
	u32     alpha_val;

	u32     blue_screen_mode;     //true : enable, false : disable
	u32     color_key_val;        //screen color value
	u32     color_switch_val;     //one color

	u32     fill_src_one_color;
	u32     fill_src_one_color_val;
		
	u32     third_op_mode;
	u32     rop_mode;
	u32     mask_mode;
	*/
	

} g2d_params;

struct g2d_dma_info {
	unsigned long addr;
	unsigned int  size;
};

typedef struct _sec_g2d_t
{
	    int dev_fd;
		g2d_params     params;
}sec_g2d_t;



#endif /*_SEC_G2D_DRIVER_H_*/

