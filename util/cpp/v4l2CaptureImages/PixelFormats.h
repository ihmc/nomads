#include <stdlib.h>
#include <stdio.h>

unsigned char *yuv420p_to_rgb24(int width, int height,
				unsigned char *pIn0, unsigned char *pOut0=NULL);
unsigned char *yuv420_to_rgb24(int width, int height,
			       unsigned char *pIn0, unsigned char *pOut0=NULL);
unsigned char *yuv411p_to_rgb24(int width, int height,
				unsigned char *pIn0, unsigned char *pOut0=NULL);
unsigned char *yuv420p_to_yuyv422(int width, int height,
				unsigned char *pIn0, unsigned char *pOut0=NULL);

/*********************************************************************************************/
/* CONVERTER FROM YUYV TO RGB FORMAT - This code comes from Yuv2rgb.tar.gz Logitech software */
/*********************************************************************************************/
int convert_yuv_to_rgb_pixel(int y, int u, int v);
int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height);

//unsigned char *YUV420_to_RGB24(unsigned char *b1,int width,int height,unsigned char *b2=NULL);
