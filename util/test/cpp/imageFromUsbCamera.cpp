#include "v4l2CaptureImages/CaptureImages.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "v4l2CaptureImages/PixelFormats.h"

#include "StrClass.h"

#include "NLFLib.h"

/* Include of ppm-jpeg conversion stuff */
#include "cdjpeg.h"

// Include stuff to send the image to the image visualizer
#include "CommHelper2.h"
#include "TCPSocket.h"

// CMVision stuff
#include "vision.h"

#define PIXEL_FORMAT_ V4L2_PIX_FMT_YUV420 // Format supported by Quickcam Pro 4000
//#define PIXEL_FORMAT_ V4L2_PIX_FMT_YUYV   // CMVision format supported by Quickcam Pro 5000

#define CONNTECTION_ATTEMPT 3


int main (int argc, char **argv)
{

// Monitoring server     
    TCPSocket *_socket;
    CommHelper2 *_ch;
    _socket = new TCPSocket();
    
    const char *_monitoringServerIP;
    unsigned short _monitoringServerPort;
    //_monitoringServerIP = "192.168.39.80"; // Erika's mac on bezdrat
    _monitoringServerIP = "10.2.8.31"; // Erika's mac
    //_monitoringServerIP = "127.0.0.1"; // Local machine
    _monitoringServerPort = 4400;
    
    int rc = -1;
    //Try at most three times to connect to the image visualizer
    for (int i = 0; i < CONNTECTION_ATTEMPT; i++)
    {
        rc = _socket->connect((const char*) _monitoringServerIP, _monitoringServerPort);
        if (rc == 0) {
            //Connection has succeed
            break;
        }
        sleepForMilliseconds (500);
    }
    if (rc != 0) {
        printf ("Error connecting to monitoring server, rc = %d\n", rc);
        delete (_socket);
        _socket = NULL;
        return -1;
    }
    _ch = new CommHelper2();
    _ch->init(_socket);
    printf ("Connected to monitoring server at: %s %d\n", _monitoringServerIP, _monitoringServerPort);
// END monitoring server    
   
// Capture image stuff    
    const char *video_device = "/dev/video0";
    const int input_idx = 0;
    const int width  = 320;
    const int height = 240;
    const int bytesperline = 480;   // magic number comes from the camera itself (check v4l-info from command line) 480 for quickcamPro4000
    //const int bytesperline = 640;   // 640 for Quickcam Pro 5000
    const int num_frames = 60;
    
    CaptureImages im;
    
    if (!(im.init(video_device,input_idx,width,height,PIXEL_FORMAT_))) {
        printf ("INIT FUNCTION DID NOT WORK. EXIT\n");
        return -1;
    }
    else {
        printf ("Webcam init succeed.\n");
    }
// Webcam correctly initialized 
/* TODO: reinsert this code and fix compilation errors!!!!    
// CMVision stuff
    LowVision vision;
    char tmap_file[64];
    snprintf(tmap_file,64,"thresh.%d%d%d.tmap.gz",bits_y,bits_u,bits_v);
    printf ("******* %s\n", tmap_file);
    vision.init("colors.txt",tmap_file,width,height);
// END CMVision stuff
*/    
    printf ("Go on with capture...\n");
    int n = 0;
    char ppmfile [15];
    bool writeRes;
    int64 i64StartTime;
    int64 i64TempTime;
    
    while (true) {
        i64StartTime = getTimeInMilliseconds();
        // Get frame
        void *frame = im.getFrame();
        i64TempTime = getTimeInMilliseconds();
        printf ("Image %d is now available. Elapsed time = %llu\n", n, i64TempTime-i64StartTime);
        i64StartTime = i64TempTime;
/* TODO: reinsert this code and fix compilation errors!!
// Process with CMVision
        vision_image cmv_img;
        cmv_img.buf    = (pixel*)(frame);
        cmv_img.width  = width;
        cmv_img.height = height;
        cmv_img.pitch  = bytesperline;   
        //cmv_img.field  = img->field;
        if (vision.processFrame(cmv_img)) {
            i64TempTime = getTimeInMilliseconds();
            printf ("vision.processFrame succeed. Elapsed time = %llu\n", i64TempTime-i64StartTime);
            i64StartTime = i64TempTime;
        }
        else {
            printf ("ERROR!!!! vision.processFrame\n");
        }
// END CMVision
*/
 
// Convert to rgb and save in PPM file        
        // Convert to rgb
        unsigned char *rgb;
        rgb = (unsigned char*)malloc(width * height * 3);
        if(rgb == NULL) {
            printf ("Something wrong happened with the rgb byte array\n");
        }
        //convert_yuv_to_rgb_buffer ((unsigned char *)frame, rgb, width, height);  // Use this for Quickcam Pro 5000
        rgb = yuv420p_to_rgb24(width, height, (unsigned char*)frame, (unsigned char*)rgb);  // Use this for Quickcam Pro 4000
        i64TempTime = getTimeInMilliseconds();
        printf ("Image converted to rgb. Elapsed time = %llu\n", i64TempTime-i64StartTime);
        i64StartTime = i64TempTime;
        // Save in rgb format. PPM file
        sprintf (ppmfile, "my_img%d.ppm", n);
        FILE *file = fopen(ppmfile, "wb");
        if (file != NULL) {
            fprintf(file,"P6\n%u %u\n255\n",width,height);
            fwrite((unsigned char *)(rgb),3,width*height,file);
            fclose(file);
        }
        i64TempTime = getTimeInMilliseconds();
        printf ("New rgb image available at %s. Elapsed time = %llu\n", ppmfile, i64TempTime-i64StartTime);
        i64StartTime = i64TempTime;
 // Done converting and saving
        
// Convert the image to jpeg
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        //int file_index;
        cjpeg_source_ptr src_mgr;
        FILE * input_file;
        FILE * output_file;
        JDIMENSION num_scanlines;

        // Initialize the JPEG compression object with default error handling.
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);                                             //jpeglib.h
        /* Add some application-specific error messages (from cderror.h) */
        //jerr.addon_message_table = cdjpeg_message_table;
        jerr.first_addon_message = JMSG_FIRSTADDONCODE;
        jerr.last_addon_message = JMSG_LASTADDONCODE;

        /* Initialize JPEG parameters.*/
        cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
        jpeg_set_defaults(&cinfo);

        // Open the input file.
        if ((input_file = fopen(ppmfile, READ_BINARY)) == NULL) {
            fprintf(stderr, "Can't open %s\n", ppmfile);
            exit(EXIT_FAILURE);
        }

        // Open the output file.
        char jpegfile [15];
        sprintf (jpegfile, "my_img.jpeg");
        if ((output_file = fopen(jpegfile, WRITE_BINARY)) == NULL) {
            fprintf(stderr, "Can't open %s\n", jpegfile);
            exit(EXIT_FAILURE);
        }

        // Figure out the input file format, and set up to read it.
        //src_mgr = select_file_type(&cinfo, input_file);
        src_mgr = jinit_read_ppm(&cinfo);
        src_mgr->input_file = input_file;
        src_mgr->input_file = input_file;

        /* Read the input file header to obtain file size & colorspace. */
        (*src_mgr->start_input) (&cinfo, src_mgr);

        /* Now that we know input colorspace, fix colorspace-dependent defaults */
        jpeg_default_colorspace(&cinfo);

        /* Specify data destination for compression */
        jpeg_stdio_dest(&cinfo, output_file);

        // Start compressor
        jpeg_start_compress(&cinfo, TRUE);

        /* Process data */
        while (cinfo.next_scanline < cinfo.image_height) {
            num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
            (void) jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
        }

        /* Finish compression and release memory */
        (*src_mgr->finish_input) (&cinfo, src_mgr);
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        /* Close files, if we opened them */
        if (input_file != stdin)
            fclose(input_file);
        if (output_file != stdout)
            fclose(output_file);
// END CONVERSION
        
        i64TempTime = getTimeInMilliseconds();
        printf ("Image converted to jpeg. Elapsed time = %llu\n", i64TempTime-i64StartTime);
        i64StartTime = i64TempTime;
  
        // Read the jpeg file to send it to the monitoring server
        long lSize;
        char * buffer;
        size_t result;
        FILE * pFile;

        pFile = fopen ( jpegfile , "rb" );
        if (pFile==NULL) {
            printf ("Can't open %s\n", jpegfile);
            return -1;
        }
        // obtain file size:
        fseek (pFile , 0 , SEEK_END);
        lSize = ftell (pFile);
        //printf("lSize dimension of the file: %d\n", lSize);
        rewind (pFile);
        // allocate memory to contain the whole file:
        buffer = (char*) malloc (sizeof(char)*lSize);
        if (buffer == NULL) {
            printf ("Error creating buffer\n");
            return -1;
        }
        // copy the file into the buffer:
        result = fread (buffer,1,lSize,pFile);
        if (result != lSize) {
            printf ("Could not read jpeg file\n");
            return -1;
        }
        else {
            //printf ("Result = %d == lSize\n",result);
        }
        // the whole file is now loaded in the memory buffer
        // terminate
        fclose (pFile);
        
        // Send buffer
        _ch->sendBlock (buffer, lSize);
        //printf("Buffer dimension: %d\n", sizeof(buffer));
        i64TempTime = getTimeInMilliseconds();
        printf ("Image sent to the visualizer. Elapsed time = %llu\n", i64TempTime-i64StartTime);
        i64StartTime = i64TempTime;
        
        free (buffer);
        
        sleepForMilliseconds(2000);
        free (rgb);
    }
    im.closeDevice();
    
    return 0;
}
