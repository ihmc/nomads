package us.ihmc.aci.disServiceProProxy.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.HashMap;

import us.ihmc.aci.util.dspro.MetadataElement;
import us.ihmc.util.ByteArray;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class DPMDMetadataParser
{
    public static HashMap<String, Object> parse (byte[] dpmd) throws IOException
    {
        return parse (ByteArray.byteArrayToString(dpmd, 0, dpmd.length));
    }

    public static HashMap<String, Object> parse (String dpmd) throws IOException
    {
        return parse (new StringReader(dpmd));
    }

    public static HashMap<String, Object> parse (File dpmdFile) throws IOException
    {
        return parse (new FileReader(dpmdFile));
    }

    public static HashMap<String, Object> parse (Reader reader) throws IOException
    {
        BufferedReader br = new BufferedReader(reader);
        String line, key, value;
        String[] tokens;
        HashMap<String, Object> properties = new HashMap<String, Object>();

        while ((line = br.readLine()) != null) {
            tokens = line.split (":");

            key = tokens[0].trim();
            value = tokens[1].trim();

            Object o;
            try {
                switch (MetadataElement.valueOf(key)) {
                    case Left_Upper_Latitude:
                    case Right_Lower_Latitude:
                    case Left_Upper_Longitude:
                    case Right_Lower_Longitude: {
                        o = new Float(value);
                        break;
                    }
                    default: {
                        o = value;
                    }
                }
                properties.put(key, o);
            }
            catch (IllegalArgumentException ex) {
                // Perform coordinates to bounding box conversion (dspro likes
                // left upper/right lower bounding box...)
                if (key.equalsIgnoreCase("Latitude")) {
                    double lat = Double.parseDouble (value);
                    properties.put (MetadataElement.Left_Upper_Latitude.toString(), new Float(lat + 0.00001));
                    properties.put (MetadataElement.Right_Lower_Latitude.toString(), new Float (lat - 0.00001));
                }
                else if(key.equalsIgnoreCase("Longitude")) {
                    double lon = Double.parseDouble (value);
                    properties.put (MetadataElement.Left_Upper_Longitude.toString(), new Float(lon - 0.00001));
                    properties.put (MetadataElement.Right_Lower_Longitude.toString(), new Float(lon + 0.00001));
                }
                else {
                    // Unknown property
                    System.err.println("Metadata does not have any attribute named " + key);
                }
            }
        }

        return properties;
    }
}
