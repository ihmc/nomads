/*
 * HTTPSUtils.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.util;

import javax.net.ssl.*;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URL;
import java.net.URLConnection;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.X509Certificate;

/**
 * Class that exposes methods util to a HTTPS request
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class HTTPSUtils
{
    /**
     * Creates the <code>InputStream</code> associated to a HTTPS request
     * @param url URL for the HTTPS request
     * @return the <code>InputStream</code> associated to a HTTPS request
     * @throws NoSuchAlgorithmException
     * @throws KeyManagementException
     * @throws IOException
     */
    public static InputStream getInputStreamFromHttps (URL url)
            throws NoSuchAlgorithmException, KeyManagementException, IOException
    {
        // Create a trust manager that does not validate certificate chains
        final TrustManager[] trustAllCerts = new TrustManager[] { new X509TrustManager()
        {
            @Override
            public void checkClientTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public void checkServerTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public X509Certificate[] getAcceptedIssuers() {
                return null;
            }
        } };

        // Install the all-trusting trust manager
        final SSLContext sslContext = SSLContext.getInstance ("SSL");
        sslContext.init (null, trustAllCerts, new java.security.SecureRandom());
        // Create an ssl socket factory with our all-trusting manager
        final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

        // All set up, we can get a resource through https now:
        final URLConnection urlCon = url.openConnection();
        // Tell the url connection object to use our socket factory which bypasses security checks
        ((HttpsURLConnection) urlCon).setSSLSocketFactory (sslSocketFactory);

        return urlCon.getInputStream();
    }


    public static InputStream getInputStreamFromHttps (URL url, String username, String password) throws
            KeyManagementException, NoSuchAlgorithmException
    {
        // Create a trust manager that does not validate certificate chains
        final TrustManager[] trustAllCerts = new TrustManager[] { new X509TrustManager()
        {
            @Override
            public void checkClientTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public void checkServerTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public X509Certificate[] getAcceptedIssuers() {
                return null;
            }
        } };

        // Install the all-trusting trust manager
        final SSLContext sslContext = SSLContext.getInstance ("SSL");
        sslContext.init (null, trustAllCerts, new java.security.SecureRandom());
        // Create an ssl socket factory with our all-trusting manager
        final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

        try {
            HttpsURLConnection http = (HttpsURLConnection) url.openConnection();
            //Authenticator.setDefault (new MyAuthenticator());
            //http.setAllowUserInteraction (true);
            http.setRequestMethod ("POST");
            http.setSSLSocketFactory (sslSocketFactory);
            //http.connect();

            return http.getInputStream();

        }
        catch (IOException ioe) {
            System.out.println ("Exception in getInputStreamFromHttps:\n" + ioe);
            return null;
        }
    }

    public static URLConnection getURLConnectionFromHTTPS (String url, String requestMode) throws
            KeyManagementException, NoSuchAlgorithmException
    {
        final TrustManager[] trustAllCerts = new TrustManager[] { new X509TrustManager()
        {
            @Override
            public void checkClientTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public void checkServerTrusted (final X509Certificate[] chain, final String authType) {
            }
            @Override
            public X509Certificate[] getAcceptedIssuers() {
                return null;
            }
        } };

        // Install the all-trusting trust manager
        final SSLContext sslContext = SSLContext.getInstance ("SSL");
        sslContext.init (null, trustAllCerts, new java.security.SecureRandom());
        // Create an ssl socket factory with our all-trusting manager
        final SSLSocketFactory sslSocketFactory = sslContext.getSocketFactory();

        try {
            URL urlRequest = new URL (url);
            URLConnection conn = urlRequest.openConnection();
            ((HttpsURLConnection)conn).setSSLSocketFactory (sslSocketFactory);
            ((HttpsURLConnection)conn).setRequestMethod (requestMode);

            return conn;
        }
        catch (MalformedURLException e) {
            e.printStackTrace();
            return null;
        }
        catch (ProtocolException e) {
            e.printStackTrace();
            return null;
        }
        catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }
}
