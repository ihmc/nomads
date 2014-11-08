/*
 * ====================================================================
 *
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:  
 *       "This product includes software developed by the 
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The names "The Jakarta Project", "Tomcat", and "Apache Software
 *    Foundation" must not be used to endorse or promote products derived
 *    from this software without prior written permission. For written 
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *
 * [Additional notices, if required by prior licensing conditions]
 *
 */

package us.ihmc.net.puretls;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.security.PublicKey;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Vector;

import us.ihmc.net.ServerSocketFactory;
import us.ihmc.util.ByteArray;
import us.ihmc.util.CertInfo;
import us.ihmc.util.CertsFileManager;
import us.ihmc.util.ConfigLoader;
//import us.ihmc.util.ConfigHelper;

import COM.claymoresystems.cert.X509Cert;
import COM.claymoresystems.ptls.SSLContext;
import COM.claymoresystems.ptls.SSLDebug;
import COM.claymoresystems.ptls.SSLException;
import COM.claymoresystems.ptls.SSLServerSocket;
import COM.claymoresystems.ptls.SSLSocket;
import COM.claymoresystems.sslg.SSLPolicyInt;

/**
 * SSL server socket factory--wraps PureTLS
 *
 * @author Eric Rescorla
 * @author Tom Cowin (mods for Nomads, etc.)
 *
 * some sections of this file cribbed from SSLSocketFactory - Eric
 * (the JSSE socket factory)
 *
 */
 
public class PureTLSServerSocketFactory implements ServerSocketFactory
{
	
	public PureTLSServerSocketFactory() 
	{
	}

    public ServerSocket createServerSocket (int port) throws IOException
    {
	    init ();
	    return new SSLServerSocket (_SSLContext, port);
    }

    public ServerSocket createServerSocket (int port, int backlog) throws IOException
    {
	    init ();
	    ServerSocket tmp;
	
	    try {
	        tmp = new SSLServerSocket (_SSLContext, port, backlog);
	    }
	    catch (IOException e){
	        throw e;
	    }
	    return tmp;
    }

    public ServerSocket createServerSocket (int port, int backlog, InetAddress ifAddress) throws IOException
    {
	    init ();
	    return new SSLServerSocket (_SSLContext, port, backlog, ifAddress);
    }

    public Socket acceptSocket (ServerSocket socket) throws IOException
    {
	    try {
	        if (_debug > 0) {
                System.out.println ("Waiting for connection");
            }
	        SSLSocket sock = (SSLSocket) socket.accept();
            postConnectionCheck (sock);
            if (_debug >= 1) {
	            System.out.println ("Accepted connection");
            }
	        return sock;
	    } catch (SSLException e) {
                throw new SocketException ("SSL handshake error" + e.toString());
	    }
    }

    public void handshake (Socket sock) throws IOException
    {
	    ((SSLSocket)sock).handshake();
    }
    
    public static void setMode (int newMode)
    {
        _mode = newMode;
    }
	
	public int cachePeersCert (SSLSocket sslSocket) {
		if (PureTLSUtils.cachePeersCert (sslSocket, _X509Certificates) == 0) {
		    CertsFileManager.writeFile (_cachedCertsPath, _X509Certificates);
			return 0;
		}
		else {
			return (-1);
		}
	}
	
	public PublicKey getPeersCachedPublicKey (SSLSocket sslSocket) {
		return PureTLSUtils.getPeersCachedPublicKey (sslSocket, _X509Certificates);
	}
	
	public Hashtable getX509CertificateCache() {
		return _X509Certificates;
	}
    
    private void postConnectionCheck (SSLSocket sslSocket) 
    {
        byte[] clientCert;
        
        try {
            //the certificate chain as a Vector of X509Certs, null if unavailable 
            //The root is at 0 and the user cert is at n-1
            Vector certificateChain = sslSocket.getCertificateChain();
            String IPAddr = PureTLSUtils.getPeersIPAddr (sslSocket); 
            
            if (_debug >= 1) {
                int cipherSuite = sslSocket.getCipherSuite();
                System.out.println ("Remote IP address is : " + IPAddr);
                System.out.println ("Cipher suite: " + SSLPolicyInt.getCipherSuiteName (cipherSuite));
            }
            
            if (certificateChain != null) {
                if (_debug >= 1) {
                    System.out.println ("Cert chain");
                }
                int certChainLength = certificateChain.size();
                for (int i = 0; i < certChainLength; i++) {
                    X509Cert cert = (X509Cert) certificateChain.elementAt (i);
                    if (_debug >= 1) {
                        System.out.println ("Cert at position " + i + ":");
                    }
                    
                    //added to check if valid cert (i.e., mapping from IP Addr to Cert)
                    if (i == certChainLength - 1) {
                        clientCert = cert.getDER();                        
                        String commonName = PureTLSUtils.getCommonName (cert);
                        if (_mode == CERTCACHING) {
                        }
                        else if (_mode == CERTUIDCROSSVALIDATING) {
                            if (_X509Certificates.containsKey (commonName)) {
                                CertInfo ci = (CertInfo) _X509Certificates.get (commonName);
                                if (!ByteArray.areEqual (ci.getDERCert(), clientCert)){
                                    System.out.println ("Client Cert for commonName " + commonName + " does not match stored value! exiting....");
                                    System.exit(1);
                                }
                                if (_debug >= 1) {
                                    System.out.println ("Client Cert matches stored value...continuing.");
                                }
                            }
                            else {
                                System.out.println ("commonName " + commonName + " has no stored cert...exiting.");
                                System.exit(1);
                            }
                        }
                        if (_debug >= 3) {
                            ByteArray.printByteArrayAsHexColumns (clientCert);
                        }
                    }
                    if (_debug >= 1) {
                        System.out.println ("Subject " + cert.getSubjectName().getNameString());
                    }
                    if (_debug >= 2) {
                        System.out.println ("Serial " + cert.getSerial());
                        System.out.println ("Issuer " + cert.getIssuerName().getNameString());
                        System.out.println ("Validity " + cert.getValidityNotBefore() + "-" + cert.getValidityNotAfter());
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void init () throws IOException
    {
	    if (_SSLContext != null)
	        return;
	
        String tmpString =  null;

		try {
            if (_debug >= 3) {
                SSLDebug.setDebug (SSLDebug.DEBUG_ALL);
            }
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            //if ((_properties = ConfigHelper.getSystemProperties()) == null) {
			//	throw new IOException ("Unable to get systemProperties");
			//}
			//if ((tmpString = (String) _properties.getProperty ("keystore")) != null) {
            if ((tmpString = cloader.getProperty("keystore")) != null) {
                _serverKeyStoreFile = tmpString;//how to differentiate here? need to?
				_clientKeyStoreFile = tmpString;
			}
			
            if ((tmpString = cloader.getProperty("keypass")) != null) {
			//if ((tmpString = _properties.getProperty ("keypass")) != null) {
				_keyPass = tmpString;
			}
			
            if ((tmpString = cloader.getProperty("rootfile")) != null) {
			//if ((tmpString = _properties.getProperty ("rootfile")) != null) {
				_rootFile = tmpString;
			}

            if ((tmpString = cloader.getProperty("randomfile")) != null) {
			//if ((tmpString = _properties.getProperty ("randomfile")) != null) {
				_randomFile = tmpString;
			}

            if ((tmpString = cloader.getProperty("secureprotocol")) != null) {
			//if ((tmpString = _properties.getProperty ("secureprotocol")) != null) {
				_protocol = tmpString;
			}
			
            if ((tmpString = cloader.getProperty("certsfile")) != null) {
			//if ((tmpString = _properties.getProperty ("certsfile")) != null) {
				_cachedCertsFile = tmpString;
			}
			
            if ((tmpString = cloader.getProperty("clientauth")) != null) {
			//if ((tmpString = (String) _properties.getProperty ("clientauth")) != null) {
				if (tmpString.equalsIgnoreCase ("true")) {
					_clientAuth = true;
				} else if (tmpString.equalsIgnoreCase ("false")) {
					_clientAuth = false;
				} else {
					throw new IOException ("Invalid value '" + tmpString + "' for 'clientauth' parameter:");
				}
			}
            
 			String baseDir = null;
			//if ((baseDir = _properties.getProperty ("HomeDir")) == null) {
            if ((baseDir = cloader.getHomeDirectory()) == null) {
                throw new IOException ("Unable to get value of HomeDir from systemProperties");
			}
			
			String securityDir = baseDir + File.separator + "lib" + File.separator + "security";
			
			String clientKeyStorePath =  securityDir + File.separator + _clientKeyStoreFile;
			String serverKeyStorePath = securityDir + File.separator + _serverKeyStoreFile;
			String rootCertPath = securityDir + File.separator + _rootFile;
			String randomPath = securityDir + File.separator + _randomFile;
			_cachedCertsPath = securityDir + File.separator + _cachedCertsFile;
			
			_X509Certificates = CertsFileManager.readFile (_cachedCertsPath);        
           _SSLContext = new SSLContext();
            try {
                _SSLContext.loadRootCertificates (rootCertPath);
            } catch (IOException iex) {
                iex.printStackTrace();
            }
			//try only using client file - with CPKV, need to assume that you have a one-one correspondence between host and key.
            _SSLContext.loadEAYKeyFile (clientKeyStorePath, _keyPass);

			try {
				_SSLContext.useRandomnessFile (randomPath, _keyPass);
				SSLPolicyInt policy = new SSLPolicyInt();
				policy.requireClientAuth (_clientAuth);
				policy.handshakeOnConnect (true);
				policy.waitOnClose (false);
				_SSLContext.setPolicy (policy);
			} catch (Exception e) {
				e.printStackTrace();
			}
            
	    } catch (Exception e) {
            e.printStackTrace();
	        throw new IOException (e.getMessage());
	    }
    }
    
    public static final int NORMAL = 0;
    public static final int CERTCACHING = 1;
    public static final int CERTUIDCROSSVALIDATING = 2;
    private static int _mode = NORMAL;
    private static Hashtable _X509Certificates;
    //private static Properties _properties = null;
    private COM.claymoresystems.ptls.SSLContext _SSLContext = null;
    private static String _protocol = "TLS";
    static boolean _clientAuth = true;
    private static String _keyPass = "foobar";    
    private static String _clientKeyStoreFile = "client.pem";
    private static String _serverKeyStoreFile = "server.pem";
    private static String _rootFile = "rootcert.pem";
    private static String _randomFile = "random_server-" + new Long (System.currentTimeMillis()).toString() + ".pem";
    private static String _cachedCertsFile = "cached_certs.txt";
    private static String _cachedCertsPath = "cached_certs.txt";

    private static final int _debug = 0;
}

    
    


