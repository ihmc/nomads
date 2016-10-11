/* 
 * File:   captureImages.h
 * Author: ebenvegnu
 *
 * Created on July 24, 2008, 3:44 PM
 */

#ifndef _CAPTUREIMAGES_H
#define	_CAPTUREIMAGES_H
#endif	/* _CAPTUREIMAGES_H */

#include "Thread.h"

#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define DEFAULT_VIDEO_DEVICE  "/dev/video"
#define VIDEO_STANDARD        "NTSC"
#define DEFAULT_IMAGE_WIDTH   320
#define DEFAULT_IMAGE_HEIGHT  240

// Consider to use RGB instead of YUV 4:2:2
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV
#define DEFAULT_VIDEO_FORMAT  V4L2_PIX_FMT_YUYV

// if you get a message like "DQBUF returned error", "DQBUF error: invalid"
// then you need to use a higher value for STREAMBUFS or process frames faster
#define STREAMBUFS 4

class CaptureImages : public NOMADSUtil::Thread
{
    public:
        class Image {
          public:
            char *data;        // image data
            size_t length;     // data buffer size
            int width,height;  // image dimensions
            int bytesperline;  // image pitch, usually width*sizeof(pixel)
            timeval timestamp; // timestamp for when the frame was captured
            char field;        // which field of interlaced video this is (0,1)
          public:
            Image()
              {data=NULL; length=0;}
            int size() const
                {return(bytesperline * height);}
            double getTimeSec() const
                {return(timestamp.tv_sec + timestamp.tv_usec*1E-6);}
            bool copy(const Image &img);
            char index;
        };
        
        class RawImageFileHdr {
          public:
            uint16_t type;         // from ImageType enum
            uint8_t  reserved;     // reserved, default to zero
            uint8_t  field;        // which field of video this is from {0,1}
            uint16_t width,height; // image dimensions
            timeval  timestamp;
          };
    
    private:
        struct v4l2_buffer tempbuf;
        int pixelformat;        // format of image data
        Image img[STREAMBUFS];  // buffers for images    
        int fd;
        bool bTerminated;
        
    public:    
        CaptureImages (void);
        ~CaptureImages (void);
        
    public:
        bool init(const char *device,int input_idx,int nwidth,int nheight,int nfmt);
        int xioctl(int fd,int request,void *data,const char *error_str = NULL);
        const Image *captureFrame(void);
        bool writeFrame(const CaptureImages::Image *image,int out_fd);
        bool releaseFrame(const Image *_img);
        int stop (void);
        void run(void);
        void closeDevice (void);
        void listSupportedInput (void);
	void listInputs (void);
        void *getFrame (void);
        void *readFrame (void);
        
    template <class data>
    inline void mzero(data &d)
    {
      memset(&d,0,sizeof(d));
    }

    template <class data>
    inline void mzero(data *d,int n)
    {
      memset(d,0,sizeof(data)*n);
    }
    
    
};

