/*
 * HTTPHelper.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * @version     $Revision: 1.20 $
 *              $Date: 2016/06/09 20:02:46 $
 */

package us.ihmc.util;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.StringTokenizer;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

/**
 *
 */
public class HTTPHelper
{
    public HTTPHelper (CommHelper commHelper)
    {
        _commHelper = Objects.requireNonNull (commHelper, "The CommHelper instance cannot be null");
    }

    public HTTPHelper (Socket socket)
    {
        Objects.requireNonNull (socket, "The socket cannot be null");
        _commHelper = new CommHelper (socket);
    }

    public InputStream getInputStream()
    {
        return _commHelper.getInputStream();
    }

    public OutputStream getOutputStream()
    {
        return _commHelper.getOutputStream();
    }

    /**
     *
     */
    public void sendRequest (String method, String resourceName, Map<HeaderFields, String> params) throws CommException
    {
        String line = method + " " + resourceName + " HTTP/1.1";
        log ("sending request: [" + line + "]");
        _commHelper.sendLine (line);

        if (params != null) {
            for (HeaderFields key : params.keySet()) {
                String value = params.get (key);
                log ("--> " + key + ": " + value);
                _commHelper.sendLine (key + ": " + value);

            }
        }

        _commHelper.sendLine ("");
    }

    public void sendPOSTRequest (String resourceName, Map<HeaderFields, String> params) throws CommException
    {
        this.sendRequest("POST", resourceName, params);
    }

    /**
     *
     */
    public void sendBlankLine() throws CommException
    {
        _commHelper.sendLine("");
    }

    public void sendResponseCode (int responseCode) throws CommException
    {
        _commHelper.sendLine (getResponseCodeMessage (responseCode));
        sendBlankLine();
    }

    String getResponseCodeMessage (int codeNum)
    {
        return "HTTP/1.1 " + getCodeMessage (codeNum);
    }

    private String getCodeMessage (int code)
    {
        String codeMessage;
        int[] shortcut = {0, LEVEL_200, LEVEL_300, LEVEL_400, LEVEL_500, RESPONSE_CODES};
        int i, pos;

        // Below 100 is illegal for HTTP status
        if (code < 100) {
            return http_status_string[LEVEL_500];
        }

        for (i = 0; i < 5; i++) {
            code -= 100;
            if (code < 100) {
                pos = (code + shortcut[i]);
                if (pos < shortcut[i + 1]) {
                    return http_status_string[pos];
                }
                else {
                    // status unknown
                    return http_status_string[LEVEL_500];
                }
            }
        }
        // 600 or above is also illegal

        return http_status_string[LEVEL_500];
    }

    /**
     *
     */
    public void expectResponse (int expectedCode) throws CommException, ProtocolException
    {
        // expect       "HTTP/1.1 <respCode> <respMessage>"
        // followed by: \n\r

        String lineAux = _commHelper.receiveLine();
        StringTokenizer st = new StringTokenizer (lineAux);
        if (st.countTokens() < 2) {
            throw new ProtocolException ("HTTP Protocol Error");
        }

        //ignore the "HTTP/1.1" string
        st.nextToken();

        int respCode = Integer.MAX_VALUE;
        try {
            respCode = Integer.parseInt(st.nextToken());
        }
        catch (NumberFormatException ex) {
            throw new ProtocolException ("HTTP Protocol Error " + ex.getMessage());
        }

        if (respCode != expectedCode) {
            String errMessage = respCode + " ";
            while (st.hasMoreTokens()) {
                errMessage += st.nextToken();
                if (st.hasMoreTokens()) {
                    errMessage += " ";
                }
            }

            throw new ProtocolException ("Expected " + expectedCode + ". Error occurred: \"" + errMessage + "\"");
        }
        
        lineAux = _commHelper.receiveLine();
        if (!"".equals(lineAux)) {
            throw new ProtocolException ("Expected blank line. Obtained: [" + lineAux + "]");
        }
    }

    public int getExpectedResponseCode() throws CommException, ProtocolException
    {
        // expect       "HTTP/1.1 <respCode> <respMessage>"
        // followed by: \n\r

        String lineAux = _commHelper.receiveLine();
        StringTokenizer st = new StringTokenizer (lineAux);
        if (st.countTokens() < 2) {
            throw new ProtocolException ("HTTP Protocol Error");
        }

        //ignore the "HTTP/1.1" string
        st.nextToken();

        int respCode = Integer.MAX_VALUE;
        try {
            respCode = Integer.parseInt (st.nextToken());
        }
        catch (NumberFormatException ex) {
            throw new ProtocolException ("HTTP Protocol Error " + ex.getMessage());
        }

        lineAux = _commHelper.receiveLine();
        if (!lineAux.isEmpty()) {
            throw new ProtocolException ("Expected blank line. Obtained: [" + lineAux + "]");
        }

        return respCode;
    }

    /**
     *
     */
    public Map<HeaderFields, String> readHeaderFields() throws CommException
    {
        String lineAux;
        Map<HeaderFields, String> headerFields = new HashMap<HeaderFields, String>();

        while ( (lineAux = _commHelper.receiveLine()) != null ) {
            if (lineAux.equals ("")) {
                // end of header.
                break;
            }

            HeaderFields key;
            String value;
            if (lineAux.startsWith (HeaderFields.RequestMethod.name())) {
                StringTokenizer st = new StringTokenizer (lineAux, " ");
                log ("received " + lineAux);
                key = HeaderFields.RequestMethod;
                value = st.nextToken();
                headerFields.put (key, value);
                key = HeaderFields.ResourceName;
                value = st.nextToken();
                headerFields.put (key, value);
                key = HeaderFields.Version;
                value = st.nextToken().replace ("HTTP/", "");
                headerFields.put (key, value);
            }
            else {
                StringTokenizer st = new StringTokenizer (lineAux, ":");
                key = HeaderFields.valueOf (st.nextToken().trim());
                value = st.nextToken().trim();
                log ("received " + key + ": " + value);
                headerFields.put (key, value);
            }
        }

        return headerFields;
    }

    public byte[] receiveContent (int contentLength) throws ProtocolException, CommException
    {
        if (contentLength < 0) {
            throw new ProtocolException ("The content length is negative");
        }

        return _commHelper.receiveBlob (contentLength);
    }

    public void sendField (HTTPHelper.HeaderFields name, String value) throws CommException, ProtocolException
    {
        if ((name == null) || (value == null)) {
            throw new ProtocolException ("Null parameters");
        }

        _commHelper.sendLine (name + ": " + value);
    }

    public void sendContent (String result) throws ProtocolException, CommException
    {
        if (result == null) {
            throw new ProtocolException ("Null results");
        }

        _commHelper.sendLine (result);
    }

    public void sendContent (byte[] buf, int length) throws ProtocolException, CommException
    {
        if (buf == null) {
            throw new ProtocolException ("Null buffer");
        }

        _commHelper.sendBlob (buf, 0, length);
    }

    public void setExpectedResponseCode (int code)
    {
        _expectCode = true;
        _expectedHTTPCode = code;
    }

    private void log (String msg)
    {
        if (DEBUG) {
            System.out.println("[HTTPHelper] " + msg);
        }
    }


    private final boolean DEBUG = false;
    private final CommHelper _commHelper;
    private boolean _expectCode = false;
    private int _expectedHTTPCode;

    public enum HeaderFields
    {
        Version,
        RequestMethod,
        ResourceName,
        Action,
        ActionType,
        ContentType,
        ContentLength,
        RequestorUUID,
        Expect,
        NodeUUID,
        InstanceUUID,
        SrcNodeUUID,
        DestNodeUIID,
        ServiceName,
        MethodName,
        MethodSignature,
        ResInfoQueryType,
        Asynchronous,
        VMContainer
    }

    private static final String[] http_status_string = {
            "100 Continue",
            "101 Switching Protocols",
            "102 Processing",
            "200 OK",
            "201 Created",
            "202 Accepted",
            "203 Non-Authoritative Information",
            "204 No Content",
            "205 Reset Content",
            "206 Partial Content",
            "207 Multi-Status",
            "300 Multiple Choices",
            "301 Moved Permanently",
            "302 Found",
            "303 See Other",
            "304 Not Modified",
            "305 Use Proxy",
            "306 unused",
            "307 Temporary Redirect",
            "400 Bad Request",
            "401 Authorization Required",
            "402 Payment Required",
            "403 Forbidden",
            "404 Not Found",
            "405 Method Not Allowed",
            "406 Not Acceptable",
            "407 Proxy Authentication Required",
            "408 Request Time-out",
            "409 Conflict",
            "410 Gone",
            "411 Length Required",
            "412 Precondition Failed",
            "413 Request Entity Too Large",
            "414 Request-URI Too Large",
            "415 Unsupported Media Type",
            "416 Requested Range Not Satisfiable",
            "417 Expectation Failed",
            "418 unused",
            "419 unused",
            "420 unused",
            "421 unused",
            "422 Unprocessable Entity",
            "423 Locked",
            "424 Failed Dependency",
            "425 No code",
            "426 Upgrade Required",
            "500 Internal Server Error",
            "501 Method Not Implemented",
            "502 Bad Gateway",
            "503 Service Temporarily Unavailable",
            "504 Gateway Time-out",
            "505 HTTP Version Not Supported",
            "506 Variant Also Negotiates",
            "507 Insufficient Storage",
            "508 unused",
            "509 unused"
    };

    private static final int LEVEL_200 = 3;
    private static final int LEVEL_300 = 11;
    private static final int LEVEL_400 = 19;
    private static final int LEVEL_500 = 46;
    private static final int RESPONSE_CODES = 56;
}
