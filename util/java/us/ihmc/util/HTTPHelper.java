/*
 * HTTPHelper.java
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

/**
 * HTTPHelper
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision: 1.8 $
 *              $Date: 2014/11/07 17:58:06 $
 */

package us.ihmc.util;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;

import us.ihmc.comm.CommHelper;

/**
 *
 */
public class HTTPHelper
{
    public HTTPHelper()
    {
    }

    public HTTPHelper (CommHelper commHelper)
    {
        _commHelper = commHelper;
    }

    public void setCommHelper (CommHelper commHelper)
    {
        _commHelper = commHelper;
    }

    /**
     *
     */
    public void sendRequest (String method, String resourceName, Hashtable params)
        throws Exception
    {
        String line = method + " " + resourceName + " HTTP/1.1";
        log ("sending request: [" + line + "]");
        _commHelper.sendLine (line);

        if (params != null) {
            Enumeration keyEnum = params.keys();
            while (keyEnum.hasMoreElements()) {
                String key = (String) keyEnum.nextElement();
                String value = (String) params.get (key);
                log ("--> " + key + ": " + value);
                _commHelper.sendLine (key + ": " + value);
            }
        }

        _commHelper.sendLine ("");
    }

    public void sendPOSTRequest (String resourceName, Hashtable params)
        throws Exception
    {
        this.sendRequest ("POST", resourceName, params);
    }

    /**
     *
     */
    public void sendBlankLine()
        throws Exception
    {
        _commHelper.sendLine ("");
    }

    /**
     *
     */
    public void expectResponse (int expectedCode)
        throws Exception
    {
        // expect       "HTTP/1.1 <respCode> <respMessage>"
        // followed by: \n\r

        String lineAux = _commHelper.receiveLine();
        StringTokenizer st = new StringTokenizer (lineAux);
        if (st.countTokens() < 2) {
            throw new Exception ("HTTP Protocol Error");
        }

        //ignore the "HTTP/1.1" string
        st.nextToken();

        int respCode = Integer.MAX_VALUE;
        try {
            respCode = Integer.parseInt(st.nextToken());
        }
        catch (Exception ex) {
            throw new Exception ("HTTP Protocol Error");
        }

        if (respCode != expectedCode) {
            String errMessage = respCode + " ";
            while (st.hasMoreTokens()) {
                errMessage += st.nextToken();
                if (st.hasMoreTokens()) {
                    errMessage += " ";
                }
            }

            throw new Exception ("Expected " + expectedCode + ". Error occurred: \"" + errMessage + "\"");
        }
        
        lineAux = _commHelper.receiveLine();
        if (!"".equals(lineAux)) {
            throw new Exception ("Expected blank line. Obtained: [" + lineAux + "]");
        }
    }

    /**
     *
     */
    public Hashtable readHeaderFields()
        throws Exception
    {
        String lineAux;
        Hashtable headerFields = new Hashtable();

        while ( (lineAux = _commHelper.receiveLine()) != null ) {
            if (lineAux.equals("")) {
                // end of header.
                break;
            }

            StringTokenizer st = new StringTokenizer(lineAux, ":");

            String key = st.nextToken().trim();
            String value = st.nextToken().trim();

            log("received " + key + ": " + value);
            headerFields.put(key, value);
        }

        return headerFields;
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private void log (String msg)
    {
        if (DEBUG) {
            System.out.println("[HTTPHelper] " + msg);
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    private final boolean DEBUG = false;

    // /////////////////////////////////////////////////////////////////////////
    private CommHelper _commHelper = null;
}
