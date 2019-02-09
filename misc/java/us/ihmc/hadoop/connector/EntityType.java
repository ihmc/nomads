package us.ihmc.hadoop.connector;

import java.text.DecimalFormat;

/**
 * EntityType.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class EntityType
{
    public String type;
    public double confidence;

    public EntityType ()
    {

    }

    public EntityType (String type, double confidence)
    {
        this.type = type;
        this.confidence = confidence;
    }

    public String getNormalizedConfidence ()
    {
        DecimalFormat formatter = new DecimalFormat("0.000");
        return formatter.format(confidence * 100) + "%";
    }
}
