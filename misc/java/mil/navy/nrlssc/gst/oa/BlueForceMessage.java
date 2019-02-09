package mil.navy.nrlssc.gst.oa;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.lang.reflect.Field;
import java.text.DateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.zip.CRC32;

public class BlueForceMessage extends OAMessage
{
    public static final int BLUE_FORCE_MAGIC_NUMBER = 0x0A424601;
    public static final int FOV_CIRCLE = 0x1;
    public static final int FOV_AREA   = 0x2;
    public static final int FOR_CIRCLE = 0x4;
    public static final int FOR_AREA   = 0x8;

    private int magicNumber;
    public int getMagicNumber()
    {
        return magicNumber;
    }

    public void setMagicNumber(int magicNumber)
    {
        this.magicNumber = magicNumber;
    }

    private long timeStamp;
    private double platformLatitude, platformLongitude, platformAltitude;
    private double platformVelocityX, platformVelocityY, platformVelocityZ;
    private String platformID;
    private boolean platformOpStatus;
    private String platformOpText;
    private int platformRemainingTime;
    private String sensorID;
    private String sensorMode;
    private boolean sensorOpStatus;
    private String sensorOpText;
    private byte sensorFlags;
    private double sensorFOVPointRadius[];
    private double sensorFOVPointArea[];
    private double sensorFORPointRadius[];
    private double sensorFORPointArea[];
    private int messageCRC;

    public BlueForceMessage()
    {
        magicNumber = BLUE_FORCE_MAGIC_NUMBER;
        timeStamp = System.currentTimeMillis();
        platformLatitude = 0;
        platformLongitude = 0;
        platformAltitude = 0;
        platformVelocityX = 0;
        platformVelocityY = 0;
        platformVelocityZ = 0;
        platformID = "Platform ID";
        platformOpStatus = true;
        platformOpText = "Platform Op Text";
        platformRemainingTime = 1000;
        sensorID = "SensorID";
        sensorMode = "SensorMode";
        sensorOpStatus = true;
        sensorOpText = "Sensor Op Text";
        sensorFlags = FOV_AREA | FOR_AREA;
        sensorFOVPointRadius = new double[4];
        sensorFOVPointArea = new double[10];
        sensorFORPointRadius = new double[4];
        sensorFORPointArea = new double[10];
    }

    @Override
    public void printMessage()
    {
        System.out.println("Current Time: " + DateFormat.getDateInstance().format(new Date(timeStamp)) + 
                           " timeStap: " + timeStamp);
        System.out.format("Position: %8.3f %8.3f %8.3f\n", platformLatitude, 
                          platformLongitude, platformAltitude);
        System.out.format("Velocity: %8.3f %8.3f %8.3f\n", platformVelocityX, 
                          platformVelocityY, platformVelocityZ);
        System.out.println("Platform ID: " + platformID);
        System.out.println("Op Status: " + platformOpStatus);
        System.out.println("Op Status Text: " + platformOpText);
        System.out.println("Remaining Time: " + platformRemainingTime);
        System.out.println("Sensor ID: " + sensorID);
        System.out.println("Sensor Mode: " + sensorMode);
        System.out.println("Sensor Op Status: " + sensorOpStatus);
        System.out.println("Sensor Op Text: " + sensorOpText);
        System.out.format("Flags: %08X\n", sensorFlags);
        // TODO Print the FOV/FOR fields
        for (int i = 0; i < 10; i++) {
            System.out.println("FOV: " + sensorFOVPointArea[i]);
        }
        for (int j = 0; j < 10; j++) {
            System.out.println("FOR: " + sensorFORPointArea[j]);
        }
        System.out.format("CRC from Message: %X\n", messageCRC);
    }

    @Override
    public byte[] serialize() throws IOException
    {
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        DataOutputStream dataStream = new DataOutputStream (byteStream);
        dataStream.writeInt(magicNumber);                       // Magic number
        dataStream.writeLong(timeStamp);                        // Time tag UTC
        dataStream.writeDouble(platformLatitude);		// Platform location lat, lon, alt
        dataStream.writeDouble(platformLongitude);
        dataStream.writeDouble(platformAltitude);
        dataStream.writeDouble(platformVelocityX);		// Platform velocity x, y, z
        dataStream.writeDouble(platformVelocityY);
        dataStream.writeDouble(platformVelocityZ);  
        dataStream.write(leftNullPad(platformID, 16));          // Platform ID
        dataStream.writeBoolean(platformOpStatus);		// Platform Op Status
        dataStream.write(leftNullPad(platformOpText, 16));      // Platform OpStat Amp. Text
        dataStream.writeInt(platformRemainingTime);		// Platform remaining time                                                 
        dataStream.write(leftNullPad(sensorID, 16));            // Sensor ID                                            
        dataStream.write(leftNullPad(sensorMode, 16));          // Sensor Mode
        dataStream.writeBoolean(sensorOpStatus);		// Sensor Op Status
        dataStream.write(leftNullPad(sensorOpText, 16));        // Sensor OpStat Amp Text
        dataStream.writeByte(sensorFlags);                      // Sensor flags

        for (int j = 0; j < 10; j++) {  // Sensor Field of View representation
            // TODO CHECK FLAG AND WRITE THE APPROPRIATE REPRESENTATION
            dataStream.writeDouble(sensorFOVPointArea[j]);
        }
        for (int k = 0; k < 10; k++) {  // Sensor Field of Regard representation
            dataStream.writeDouble(sensorFORPointArea[k]);
        }
        CRC32 checker = new CRC32();
        checker.update(byteStream.toByteArray());
        messageCRC = (int)checker.getValue();
        dataStream.writeInt(messageCRC);		
        return byteStream.toByteArray();
    }

    @Override
    public boolean deserialize (byte messageBuffer[])
    {
        ByteArrayInputStream byteInput = new ByteArrayInputStream(messageBuffer);
        DataInputStream dataInput = new DataInputStream(byteInput);
        byte readBuffer[] = new byte[16];
        try {
            if (readAndCheckMagicNumber (dataInput)) {
                System.out.println("Blue Force Message identified.");
            }
            else {
                return false;
            }
            timeStamp = dataInput.readLong();
            platformLatitude = dataInput.readDouble();
            platformLongitude = dataInput.readDouble();
            platformAltitude = dataInput.readDouble();
            platformVelocityX = dataInput.readDouble();
            platformVelocityY = dataInput.readDouble();
            platformVelocityZ = dataInput.readDouble();
            dataInput.read(readBuffer, 0, 16);
            platformID = new String(readBuffer);
            platformOpStatus = dataInput.readBoolean();
            dataInput.read(readBuffer, 0, 16);
            platformOpText = new String(readBuffer);
            platformRemainingTime = dataInput.readInt();
            dataInput.read(readBuffer, 0, 16);
            sensorID = new String(readBuffer);
            dataInput.read(readBuffer, 0, 16);
            sensorMode = new String(readBuffer);
            sensorOpStatus = dataInput.readBoolean();
            dataInput.read(readBuffer, 0, 16);
            sensorOpText = new String(readBuffer);
            sensorFlags = dataInput.readByte();

            for (int j = 0; j < 10; j++) {
                double value = dataInput.readDouble();
                if (j < 3) {
                    sensorFOVPointRadius[j] = value;
                }
                sensorFOVPointArea[j] = value;
            }
            for (int k = 0; k < 10; k++) {
                double value = dataInput.readDouble();
                if (k < 3) {
                    sensorFORPointRadius[k] = value;
                }
                sensorFORPointArea[k] = value;
            }
            CRC32 checker = new CRC32();
            checker.update(messageBuffer, 0, messageBuffer.length - byteInput.available());
            messageCRC = dataInput.readInt();
            if ((int)checker.getValue() == messageCRC) {
                System.out.println("Passed CRC32");
            }
            else {
                System.out.format("Failed CRC32, Computed: %X, Message %X\n", checker.getValue(), messageCRC);
                return false;   
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return true;
    }

    public static boolean readAndCheckMagicNumber (DataInputStream dataInput) throws IOException
    {
        return (dataInput.readInt() == BLUE_FORCE_MAGIC_NUMBER);
    }

    public double[] getSensorFOVPointArea()
    {
        return sensorFOVPointArea;
    }

    public void setSensorFOVPointArea(double[] sensorFOVPointArea)
    {
        this.sensorFOVPointArea = sensorFOVPointArea;
    }

    public double[] getSensorFORPointRadius()
    {
        return sensorFORPointRadius;
    }

    public void setSensorFORPointRadius(double[] sensorFORPointRadius)
    {
        this.sensorFORPointRadius = sensorFORPointRadius;
    }

    public long getTimeStamp()
    {
        return timeStamp;
    }

    public void setTimeStamp(long timeStamp)
    {
        this.timeStamp = timeStamp;
    }

    public double getPlatformLatitude()
    {
        return platformLatitude;
    }

    public void setPlatformLatitude(double platformLatitude)
    {
        this.platformLatitude = platformLatitude;
    }

    public double getPlatformLongitude()
    {
        return platformLongitude;
    }

    public void setPlatformLongitude(double platformLongitude)
    {
        this.platformLongitude = platformLongitude;
    }

    public double getPlatformAltitude()
    {
        return platformAltitude;
    }

    public void setPlatformAltitude(double platformAltitude)
    {
        this.platformAltitude = platformAltitude;
    }

    public double getPlatformVelocityX()
    {
        return platformVelocityX;
    }

    public void setPlatformVelocityX(double platformVelocityX)
    {
        this.platformVelocityX = platformVelocityX;
    }

    public double getPlatformVelocityY()
    {
        return platformVelocityY;
    }

    public void setPlatformVelocityY(double platformVelocityY)
    {
        this.platformVelocityY = platformVelocityY;
    }

    public double getPlatformVelocityZ() {
        return platformVelocityZ;
    }

    public void setPlatformVelocityZ(double platformVelocityZ)
    {
        this.platformVelocityZ = platformVelocityZ;
    }

    public String getPlatformID()
    {
        return platformID;
    }

    public void setPlatformID(String platformID)
    {
        this.platformID = platformID;
    }

    public boolean isPlatformOpStatus()
    {
        return platformOpStatus;
    }

    public void setPlatformOpStatus(boolean platformOpStatus)
    {
        this.platformOpStatus = platformOpStatus;
    }

    public String getPlatformOpText()
    {
        return platformOpText;
    }

    public void setPlatformOpText(String platformOpText)
    {
        this.platformOpText = platformOpText;
    }

    public int getPlatformRemainingTime()
    {
        return platformRemainingTime;
    }

    public void setPlatformRemainingTime(int platformRemainingTime)
    {
        this.platformRemainingTime = platformRemainingTime;
    }

    public String getSensorID()
    {
        return sensorID;
    }

    public void setSensorID(String sensorID)
    {
        this.sensorID = sensorID;
    }

    public byte getSensorFlags()
    {
        return sensorFlags;
    }

    public void setSensorFlags(byte sensorFlags)
    {
        this.sensorFlags = sensorFlags;
    }

    public double[] getSensorFOVPointRadius()
    {
        return sensorFOVPointRadius;
    }

    public void setSensorFOVPointRadius(double[] sensorFOVPointRadius)
    {
        this.sensorFOVPointRadius = sensorFOVPointRadius;
    }

    public double[] getSensorFORPointArea()
    {
        return sensorFORPointArea;
    }

    public void setSensorFORPointArea(double[] sensorFORPointArea)
    {
        this.sensorFORPointArea = sensorFORPointArea;
    }

    public int getCRC()
    {
        return messageCRC;
    }

    public void setCRC(int messageCRC)
    {
        this.messageCRC = messageCRC;
    }

    public String getSensorMode()
    {
        return sensorMode;
    }

    public void setSensorMode(String sensorMode)
    {
        this.sensorMode = sensorMode;
    }

    public boolean isSensorOpStatus()
    {
        return sensorOpStatus;
    }

    public void setSensorOpStatus(boolean sensorOpStatus)
    {
        this.sensorOpStatus = sensorOpStatus;
    }

    public String getSensorOpText()
    {
        return sensorOpText;
    }

    public void setSensorOpText(String sensorOpText)
    {
        this.sensorOpText = sensorOpText;
    }

    /**
     * This only indicates whether or not two BlueForceMessages possess the same
     * time stamp. Its intended use is in sorting a list of BlueForceMessages
     * based on their time stamps.
     */
    @Override
    public boolean equals(Object obj)
    {
        if(obj instanceof BlueForceMessage)
            return ((BlueForceMessage)obj).getTimeStamp() == this.getTimeStamp();
        else
            return false;
    }

    @Override
    public String toString() {
        String s = "";
        for (Field field : this.getClass().getDeclaredFields()) {
            try {
                s += (field.getName()
                     + " - " + field.getType()
                     + " - " + field.get(this)
                     + "\n");
            } catch (IllegalArgumentException ex) {
                Logger.getLogger(OAMessage.class.getName()).log(Level.SEVERE, null, ex);
            } catch (IllegalAccessException ex) {
                Logger.getLogger(OAMessage.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        return s;
    }
	
}
