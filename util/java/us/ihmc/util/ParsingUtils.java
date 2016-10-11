/*
 * ParsingUtils.java
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

package us.ihmc.util;

import java.io.File;
import java.io.IOException;
import java.io.Reader;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class ParsingUtils
{
    public static Float doubleToFloat (double d)
    {
        return (new Double (d)).floatValue();
    }

    public static Float readFloat (String string)
    {
        return new Float ((new Double (Double.parseDouble(string))).floatValue());
    }

    public static Document parseDOM (File file) throws ParserConfigurationException, SAXException, IOException
    {
        return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse (file);
    }

    public static Document parseDOM (String string) throws ParserConfigurationException, SAXException, IOException
    {
        return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse (string);
    }

    public static Document parseDOM (Reader reader) throws ParserConfigurationException, SAXException, IOException
    {
        return DocumentBuilderFactory.newInstance().newDocumentBuilder().parse (new ReaderInputStream(reader));
    }

    public static XPathExpression getExpression (String expression)
    {
        try {
            return XPathFactory.newInstance().newXPath().compile (expression);
        } catch (XPathExpressionException ex) {
            return null;
        }
    }
}
