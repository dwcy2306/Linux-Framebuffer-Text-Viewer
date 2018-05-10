#ifndef __FBAPI_H__
#define __FBAPI_H__

#define FB_DEV	"/dev/fb0"
#define BPP (gfb.var.bits_per_pixel / 8)
#define PITCH	(gfb.fix.line_length)

typedef struct FrameBuffer {
	int         fd;
	void        *start;
	size_t      length;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
} FrameBuffer;

typedef struct Point{
	int x, y;
}point_t;

typedef struct Rectangle{
	int x, y, w, h;
}rec_t;

FrameBuffer gfb;

#ifdef __cplusplus
extern "C" {
#endif
	int fb_open(FrameBuffer *fb);
	void fb_close(FrameBuffer *fb);
	int draw_line(point_t *a, point_t *b, unsigned int color);
#ifdef __cplusplus
}
#endif

#endif
