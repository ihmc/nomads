#include "HTTPClient.h"

#define CAMERA_URL "http://10.100.0.138"

using namespace NOMADSUtil;

int main (int argc, char **argv)
{
	const char *url = CAMERA_URL;
	HTTPClient::getDataIntoFile ("./test.jpg", url);
}
