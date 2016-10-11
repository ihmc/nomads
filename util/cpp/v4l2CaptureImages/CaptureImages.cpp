
#include "CaptureImages.h"

#include <getopt.h>
#include <sys/ioctl.h> // used for v4l ioctl
#include <fcntl.h> // used for v4l2 open
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

#include "NLFLib.h"

using namespace NOMADSUtil;

CaptureImages::CaptureImages (void)
{
    bTerminated = false;
}

CaptureImages::~CaptureImages (void)
{
    stop();
}

int CaptureImages::xioctl (int fd, int request, void *arg, const char *error_str)
{
    int r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

bool CaptureImages::init (const char *device,int input_idx,int nwidth,int nheight,int nfmt)
{
    struct v4l2_requestbuffers req;
    int err;

    // Set defaults if not given
    if(!device) device = DEFAULT_VIDEO_DEVICE;
    if(!nfmt) nfmt = DEFAULT_VIDEO_FORMAT;
    if(!nwidth || !nheight){
        nwidth  = DEFAULT_IMAGE_WIDTH;
        nheight = DEFAULT_IMAGE_HEIGHT;
    }
    
    // Open device
    fd = open(device, O_RDWR|O_NONBLOCK);
    if (fd == -1) {
        printf("Could not open video device [%s]\n", device);
        return (false);
    }
    /*else {
        printf ("Open device succeed\n");
    }*/
    
    struct v4l2_format fmt;
 /*   // Query the device format
    mzero(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ((xioctl(fd, VIDIOC_G_FMT, &fmt, "SetFormat") == -1)) {
        printf("ERROR: Could not query format\n");
    }
    printf ("Default width: %d\n", fmt.fmt.pix.width);
    printf ("Default height: %d\n", fmt.fmt.pix.height);
    printf ("Default pixelformat: %d\n", fmt.fmt.pix.pixelformat);
*/    
/*    // Enum format
    struct v4l2_fmtdesc argp;
    mzero(argp);
    argp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while ((xioctl(fd, VIDIOC_ENUM_FMT, &argp, "SetFormat") == 0)) {
        printf ("Format # %d\n", argp.index);
        printf ("Default pixelformat description: %s\n", argp.description);
        printf ("Default pixelformat: %d\n", argp.pixelformat);
        argp.index++;
    }
    printf("Done enumerating formats\n");
*/    
    
    // Change device properties
    // Set video format
    
    mzero(fmt);
    fmt.fmt.pix.width       = nwidth;
    fmt.fmt.pix.height      = nheight;
    fmt.fmt.pix.pixelformat = nfmt;
    //fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED; // This setting does not work for YUV420 images because that is a planar format anyway
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if ((xioctl(fd, VIDIOC_S_FMT, &fmt, "SetFormat") == -1)) {
        printf("ERROR: Could not set format\n");
        return false;
    }
    
/*    
    v4l2_control ctrl;
    mzero(ctrl);
    ctrl.id = V4L2_CID_BRIGHTNESS;
    ctrl.value = 64;
    if(xioctl(fd, VIDIOC_S_CTRL, &ctrl, "SetControl") == -1){
        printf("ERROR: Could not set control: brightness\n");
    }
    else {
        printf ("brightness set to %d\n", ctrl.value);
    }
    mzero(ctrl);
    ctrl.id = V4L2_CID_CONTRAST;
    ctrl.value = 32;
    if(xioctl(fd, VIDIOC_S_CTRL, &ctrl, "SetControl") == -1){
        printf("ERROR: Could not set control: contrast\n");
    }
    else {
        printf ("contrast set to %d\n", ctrl.value);
    }
    mzero(ctrl);
    ctrl.id = V4L2_CID_SATURATION;
    ctrl.value = 0;
    if(xioctl(fd, VIDIOC_S_CTRL, &ctrl, "SetControl") == -1){
        printf("ERROR: Could not set control: saturation\n");
    }
    else {
        printf ("saturation set to %d\n", ctrl.value);
    }
 */
/*    
//    printf ("VIDIOC_G_FMT: field %s type %s\n", fmt.fmt.pix.pixelformat, fmt.fmt.pix.field);
    pixelformat = fmt.fmt.pix.pixelformat;
    //Set input and controls
    // should use VIDIOC_S_INPUT
    if (!(xioctl(fd,VIDIOC_S_INPUT,&input_idx,"SetInput") == 0)) {
        printf("ERROR: Could not set input\n");
    }
    //Set standard
    v4l2_std_id id = V4L2_STD_NTSC;
    // should use VIDIOC_S_STD
    if (!(xioctl(fd,VIDIOC_G_STD,&id,"SetStandard") == 0)) {
        printf("ERROR: Could not set standard\n");
    }
*/    
    // Request mmap-able capture buffers
    mzero(req);
    req.count  = STREAMBUFS;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    //err = ioctl(fd, VIDIOC_REQBUFS, &req);
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            printf ("The device does not support memory mapping\n");
            return false;
        }
        else {
            printf ("ERROR: VIDIOC_REQBUFS\n");
            return false;
        }
    }
    /*else {
        printf ("VIDIOC_REQBUFS succeed\n");
    }*/

    // set up individual buffers
    mzero(img,STREAMBUFS);
    mzero(tempbuf);
    tempbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tempbuf.memory = V4L2_MEMORY_MMAP;
    for(unsigned i=0; i<req.count; i++){
        tempbuf.index = i;
        err = ioctl(fd, VIDIOC_QUERYBUF, &tempbuf);
        if(err < 0){
            printf("QUERYBUF returned error %d\n",errno);
            return(false);
        }
        img[i].length = tempbuf.length;
        img[i].data = (char*)mmap(NULL, tempbuf.length,
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  fd, tempbuf.m.offset);
        if(img[i].data == MAP_FAILED){
            printf("mmap() returned error %d (%s)\n",errno);
            return(false);
        }
        // fill out other fields
        img[i].width        = fmt.fmt.pix.width;
        img[i].height       = fmt.fmt.pix.height;
        img[i].bytesperline = fmt.fmt.pix.bytesperline;
        img[i].index = i;
    }
    for(unsigned i=0; i<req.count; i++){
        tempbuf.index = i;
        if(!(xioctl(fd, VIDIOC_QBUF, &tempbuf, "EnqueueBuffer") == 0)){
            printf("Error queueing initial buffers\n");
            return(false);
        }
    }
  
    // Start streaming
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (!(xioctl(fd, VIDIOC_STREAMON, &type, "StreamOn") == 0)) {
        printf ("Error starting the streaming\n");
        return (false);
    }
    //printf ("Init succeed\n");
    return (true);
    
}

bool CaptureImages::releaseFrame(const CaptureImages::Image *_img)
{
  if(!_img) return(false);
  tempbuf.index = _img->index;
  if (xioctl(fd, VIDIOC_QBUF, &tempbuf, "EnqueueBuffer") == 0) {
      return true;
  }
  return false;
}

const CaptureImages::Image *CaptureImages::captureFrame(void)
{
    //while(true){
        // get the frame
        xioctl(fd, VIDIOC_DQBUF, &tempbuf, "DequeueBuffer");

        // poll to see if a another frame is already available
        // if so, break out now
        //if(!vid.isFrameReady()) break;

        // otherwise, drop this frame
        //vid.enqueueBuffer(tempbuf);
    //}
    int i = tempbuf.index;
    img[i].timestamp = tempbuf.timestamp;
    img[i].field = (tempbuf.field == V4L2_FIELD_BOTTOM);
    
    return(&(img[i]));    
}

bool CaptureImages::writeFrame(const CaptureImages::Image *image,int out_fd)
{
  if(!image || out_fd<0) return(false);

  // only YUV422 supported for now
 /* if(image->bytesperline != image->width*2) {
      printf ("writeFrame: only YUV422 supported for now\n");
      return(false);
  }
*/
  // create header
  RawImageFileHdr hdr;
  mzero(hdr);
  //hdr.type      = ImageTypeRawYUV;
  hdr.type      = 1;
  hdr.field     = image->field;
  hdr.width     = image->width;
  hdr.height    = image->height;
  hdr.timestamp = image->timestamp;

  // write header and contents
  int hw = ::write(out_fd,&hdr,sizeof(hdr));
  int bw = ::write(out_fd,image->data,image->size());

  bool ok = (hw==sizeof(hdr) && bw==image->size());
  return(ok);
}


void CaptureImages::closeDevice (void)
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type, "StreamOff");

    for(int i=0; i<STREAMBUFS; i++){
      if(img[i].data){
        munmap(img[i].data,img[i].length);
      }
    }

    if(fd >= 0) {
        close(fd);
    }
    fd = -1;
}

int CaptureImages::stop (void)
{
    bTerminated = true;
    return 0;
}

void CaptureImages::run (void)
{
    const char *video_device = "/dev/video0";
    const int input_idx = 1;
    const int width  = 320;
    const int height = 240;
    const int num_frames = 60;
    
    if (!(init(video_device,input_idx,width,height,PIXEL_FORMAT))) {
        return;
    }
    while (!bTerminated) {
        // capture and process a frame
        const CaptureImages::Image *img = captureFrame();
        if(img != NULL){
            // do something with the data here: img->data, img->width, img->height 
            

            
            releaseFrame(img);
        }
        sleepForMilliseconds(2000);
    }
}

void CaptureImages::listSupportedInput (void)
{
    struct v4l2_input input;
    struct v4l2_standard standard;
    v4l2_std_id std_id;

    memset (&input, 0, sizeof (input));

    if (0 != ioctl (fd, VIDIOC_G_INPUT, &input.index)) {
        printf ("Error: VIDIOC_G_INPUT\n");
    }
    
    if (0 != ioctl (fd, VIDIOC_S_INPUT, &input.index)) {
        printf ("Error: VIDIOC_S_INPUT\n");
    }
    else {
	printf ("Input set to %i\n", input.index);
    }

    if (0 != ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
        printf ("Error: VIDIOC_ENUMINPUT\n");
    }
    
/*    if (0 == (input.std & V4L2_STD_PAL_BG)) {
	printf ("B/G PAL not supported\n");
	printf ("input.std = %s\n", input.std);
    }
    if (0 == (input.std & V4L2_STD_NTSC)) {
	printf ("NTSC not supported\n");
    }
*/
    printf ("input name: %s\t input type %i\n", input.name, input.type);
    
    printf ("Current input supports:\n");

    memset (&standard, 0, sizeof (standard));
    standard.index = 0;

    if (0 != ioctl (fd, VIDIOC_G_STD, &std_id)) {
	printf ("Error: VIDIOC_G_STD\n");
    }


    int res = ioctl (fd, VIDIOC_ENUMSTD, &standard);    
    while (res == 0) {
        if (standard.id & std_id) {
     	    //printf ("Name of the standard %s\n", standard.name);
	    //printf ("Frame period %i/%i\n", standard.frameperiod.numerator, standard.frameperiod.denominator);
            printf ("Video standard: %s\n", standard.name);
        }
        standard.index++;
	res = ioctl (fd, VIDIOC_ENUMSTD, &standard);
    }

    /* EINVAL indicates the end of the enumeration, which cannot be
       empty unless this device falls under the USB exception. */
    if (errno != EINVAL || standard.index == 0) {
            printf ("Error: VIDIOC_ENUMSTD\n");
    }
}

void CaptureImages::listInputs (void)
{
  struct v4l2_input input;
  int current = -1;
  if(ioctl(fd, VIDIOC_G_INPUT, &current) < 0) return;

  int index = 0;
  printf("Inputs:\n");
  while(true){
    memset(&input, 0, sizeof (input));
    input.index = index;
    int ret = ioctl(fd, VIDIOC_ENUMINPUT, &input);
    if(ret < 0) break;
    char ch = (index == current)? '#' : ' ';
    printf("  %d: (%c) \"%s\"\n",index,ch,input.name);
    index++;
  }
  
}
  void * CaptureImages::getFrame()
  {
      for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;
        FD_ZERO (&fds);
        FD_SET (fd, &fds);
        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        r = select (fd + 1, &fds, NULL, NULL, &tv);
        if (-1 == r) {
            if (EINTR == errno)
                continue;
            printf ("ERROR!!!!\n");
        }
        if (0 == r) {
            fprintf (stderr, "select timeout\n");
            printf ("ERROR!!!!\n");
        }
        void *p=readFrame();
        if (p!=NULL)
        return p;
        /* EAGAIN - continue select loop. */
    }

  }
  
  void *CaptureImages::readFrame()
  {
      struct v4l2_buffer buf;
      mzero(buf);
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      xioctl(fd, VIDIOC_DQBUF, &buf, "DequeueBuffer");
      if (-1 == (xioctl(fd, VIDIOC_QBUF, &buf, "EnqueueBuffer"))) {
          printf ("ERROR: EnqueueBuffer\n");
      }
      return img[buf.index].data;
  }
  



