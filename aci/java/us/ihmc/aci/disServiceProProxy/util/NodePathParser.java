package us.ihmc.aci.disServiceProProxy.util;

import java.io.*;

import us.ihmc.aci.util.dspro.NodePath;

/**
 * Parses NodePath files stored in the format
 *
 * @author Giacomo Benincasa    gbenincasa@ihmc.us
 */
public class NodePathParser
{
    public static NodePath parse (String pathFileName)
    {
        return parse(new File(pathFileName));
    }


    public static NodePath parse (File pathFile)
    {
        NodePath np;
        try {
            np = parse(new FileInputStream(pathFile));
        }
        catch (FileNotFoundException e) {
            System.err.println("Error: " + e.getMessage());
            return null;
        }

        return np;
    }

    public static NodePath parse (InputStream in)
    {
        try {
            BufferedReader br = new BufferedReader(new InputStreamReader(in));
            String line;

            if ((line = br.readLine()) != null) {
                // Parse the first line
                String[] tokens = line.split(",");
                if (tokens.length != 4) {
                    throw new Exception("The Node Path header must be in the form: <pathID, realTime, pathType, " +
                            "probability");
                }
                String pathID = tokens[0].trim();
                String realTime = tokens[1].trim();
                if (!realTime.equalsIgnoreCase("AbsTime") && !realTime.equalsIgnoreCase("RelTime")) {
                    throw new Exception("Allowed values for realTime are: {AbsTime, RelTime}");
                }
                short pathType = Short.parseShort(tokens[2].trim());
                switch (pathType) {
                    case NodePath.MAIN_PATH_TO_OBJECTIVE:
                    case NodePath.ALTERNATIVE_PATH_TO_OBJECTIVE:
                    case NodePath.MAIN_PATH_TO_BASE:
                    case NodePath.ALTERNATIVE_PATH_TO_BASE:
                    case NodePath.FIXED_LOCATION:
                        break;
                    default:
                        throw new Exception("The pathType must be {" + NodePath.MAIN_PATH_TO_OBJECTIVE + " | "
                                + NodePath.ALTERNATIVE_PATH_TO_OBJECTIVE + " | "
                                + NodePath.MAIN_PATH_TO_BASE + " | "
                                + NodePath.ALTERNATIVE_PATH_TO_BASE + " | "
                                + NodePath.FIXED_LOCATION + "}");
                }
                float probability = Float.parseFloat(tokens[3].trim());
                if (probability < 0.0 || probability > 1.0) {
                    throw new Exception("Values of probability must be in ]0.0, 1.0[");
                }
                // Create NodePath
                // Define variables
                long time = 0;
                float latitude, longitude, altitude;
                String location, note;
                NodePath path = new NodePath(pathID, pathType, probability);
                long now = System.currentTimeMillis();
                while ((line = br.readLine()) != null) {
                    // Set default values for way point
                    time = 0;
                    latitude = longitude = altitude = 0;
                    location = note = "";
                    tokens = line.split(",");
                    boolean bEOF = false;
                    for (int j = 0; j < tokens.length; j++) {
                        switch (j) {
                            case 0: {
                                if (Integer.parseInt(tokens[j]) == -1) {
                                    bEOF = true;
                                    break;
                                }
                                long entryTime = Long.valueOf(tokens[j].trim());
                                if (realTime.equalsIgnoreCase("RelTime")) {
                                    time = (entryTime * 1000) + now;
                                }
                                else {
                                    time = entryTime * 1000;
                                }
                                break;
                            }
                            case 1: {
                                latitude = Float.parseFloat(tokens[j].trim());
                                break;
                            }
                            case 2: {
                                longitude = Float.parseFloat(tokens[j].trim());
                                break;
                            }
                            case 3: {
                                altitude = Float.parseFloat(tokens[j].trim());
                                break;
                            }
                            case 4: {
                                location = tokens[j].trim();
                                break;
                            }
                            case 5: {
                                note = tokens[j].trim();
                                break;
                            }
                        }
                        if (bEOF) {
                            break;
                        }
                    }
                    if (!bEOF) {
                        path.appendWayPoint(latitude, longitude, altitude, location, note, time);
                    }
                }

                return path;
            }
            br.close();
        }
        catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
        return null;
    }

}
