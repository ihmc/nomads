#ifndef INCL_DTLS_CONST_H
#define INCL_DTLS_CONST_H

namespace IHMC_DTLS
{
	const static int CERTIFICATE_SIZE                 = 3000;
	const static int DEFAULT_MTU                      = 1400;
	const static int HANDSHAKE_ATTEMPTS               = 5;
	const static int HANDSHAKE_INTERVAL_BEFORE_RESEND = 2000;
    const static int NO_RCV                           = 0;
}
#endif   // #ifndef INCL_DTLS_CONST_H