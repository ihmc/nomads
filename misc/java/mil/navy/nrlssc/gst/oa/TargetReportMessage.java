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

public class TargetReportMessage extends OAMessage {
    public static final int TARGET_REPORT_MAGIC_NUMBER = 0x0A545201;

    private int magicNumber;
    private long timeStamp;
    private String platformID;
    private String sensorID;
    private long reportID;
    private long targetID;
    private double targetPositionLat, targetPositionLon, targetPositionAlt;
    private double targetPositionError[] = new double[9];
    private double targetVelocityX, targetVelocityY, targetVelocityZ;
    private double targetVelocityError[] = new double[9];
    private String targetType;
    private String targetAnnotation;
    private String combatID;
    private int CRC32;
    
    public TargetReportMessage() {
        magicNumber = TARGET_REPORT_MAGIC_NUMBER;
        timeStamp = System.currentTimeMillis();
        platformID = "PlatformID";
        sensorID = "SensorID";
        reportID = 0;
        targetID = 0;
        targetPositionLat = 0;
        targetPositionLon = 0;
        targetPositionAlt = 0;
        targetPositionError = new double[9];
        targetVelocityX = 0;
        targetVelocityY = 0;
        targetVelocityZ = 0;
        targetVelocityError = new double[9];
        targetType = "????????";
        targetAnnotation = "Annotation";
        combatID = "???";
    }
	
    @Override
    public boolean deserialize(byte[] messageBuffer)
    {
        // TODO Auto-generated method stub
        ByteArrayInputStream byteInput = new ByteArrayInputStream(messageBuffer);
        DataInputStream dataInput = new DataInputStream(byteInput);
        try {
            byte readBuffer[] = new byte[128];
            if (readAndCheckMagicNumber (dataInput)) {
                System.out.println("Target Report Message");
            }
            else {
                System.out.println("Magic number not matched");
                return false;
            }

            timeStamp = dataInput.readLong();
            dataInput.read(readBuffer, 0, 16);
            platformID = new String(readBuffer).trim();
            dataInput.read(readBuffer, 0, 16);
            sensorID = new String(readBuffer).trim();

            reportID = (long)dataInput.readInt(); // TODO need to interpret these as unsigned
            targetID = (long)dataInput.readInt();

            targetPositionLat = dataInput.readDouble();
            targetPositionLon = dataInput.readDouble();
            targetPositionAlt = dataInput.readDouble();

            for (int j = 0; j < 9; j++) {
                targetPositionError[j] = dataInput.readDouble();
            }

            targetVelocityX = dataInput.readDouble();
            targetVelocityY = dataInput.readDouble();
            targetVelocityZ = dataInput.readDouble();

            for (int k = 0; k < 9; k++) {
                targetVelocityError[k] = dataInput.readDouble();
            }

            // Very lazy, alternative to zeroing
            readBuffer = new byte[8];
            dataInput.read(readBuffer, 0, 8);
            targetType = new String(readBuffer).trim();
            readBuffer = new byte[128];
            dataInput.read(readBuffer, 0, 128);
            targetAnnotation = new String(readBuffer).trim();
            readBuffer = new byte[3];
            dataInput.read(readBuffer, 0, 3);
            combatID = new String(readBuffer).trim();

            CRC32 checker = new CRC32();
            checker.update(messageBuffer, 0, messageBuffer.length - byteInput.available());
            CRC32 = dataInput.readInt();
            if (checker.getValue() == CRC32) {
                System.out.println("Passed CRC32");
            } else {
                System.out.println("Failed CRC32");
                return false;
            }
        } catch (IOException ioe) {
            ioe.printStackTrace();
            return false;
        }
        return true;
    }

    public static boolean readAndCheckMagicNumber (DataInputStream dataInput) throws IOException
    {
        return (dataInput.readInt() == TARGET_REPORT_MAGIC_NUMBER);
    }

    @Override
    public void printMessage()
    {
        System.out.format("Magic Number: %08X\n", magicNumber);
        System.out.println("Current Time: " + DateFormat.getDateInstance().format(new Date(timeStamp)));
        System.out.println("Platform ID: " + platformID);
        System.out.println("Sensor ID: " + sensorID);
        System.out.format("Position: %8.3f %8.3f %8.3f\n", targetPositionLat, 
                                                 targetPositionLon, 
                                                 targetPositionAlt);
        System.out.format("Velocity: %8.3f %8.3f %8.3f\n", targetVelocityX, 
                                                 targetVelocityY, 
                                                 targetVelocityZ);
        System.out.println("Target type: " + targetType);
        System.out.println("Annotation: " + targetAnnotation);
        System.out.println("Combat ID: " + combatID);
        System.out.println("CRC from Message: " + CRC32);
    }

    @Override
    public byte[] serialize() throws IOException
    {
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        DataOutputStream dataStream = new DataOutputStream(
                        byteStream);
        dataStream.writeInt(magicNumber);					// Magic number
        dataStream.writeLong(timeStamp);	// Time tag UTC
                                                                                // Platform ID	
        dataStream.write(leftNullPad(platformID, 16));
                                                                                // Sensor ID
        dataStream.write(leftNullPad(sensorID, 16));
        dataStream.writeInt((int) reportID); 			// Report Unique ID
        dataStream.writeInt((int) targetID);			// Target Unique ID
        dataStream.writeDouble(targetPositionLat);		// Target location lat, lon, alt
        dataStream.writeDouble(targetPositionLon);
        dataStream.writeDouble(targetPositionAlt);

        for (int j = 0; j < 9; j++) {   	// Target Location Error
                dataStream.writeDouble(targetPositionError[j]);
        }

        dataStream.writeDouble(targetVelocityX);		// Platform velocity x, y, z
        dataStream.writeDouble(targetVelocityY);
        dataStream.writeDouble(targetVelocityZ);

        for (int k = 0; k < 9; k++) {   	// Target Velocity Error
                dataStream.writeDouble(targetVelocityError[k]);
        }									// Target Type
        dataStream.write(leftNullPad(targetType, 8));
                                                                                // Annotation
        dataStream.write(leftNullPad(targetAnnotation, 128));
                                                                                // Combat ID
        dataStream.write(leftNullPad(combatID, 3));

        CRC32 checker = new CRC32();
        checker.update(byteStream.toByteArray());
        System.out.println("Computed CRC: " + checker.getValue());
        CRC32 = (int)checker.getValue();
        dataStream.writeInt(CRC32);
        return byteStream.toByteArray();
    }

    public int getMagicNumber()
    {
        return magicNumber;
    }

    public void setMagicNumber(int magicNumber)
    {
        this.magicNumber = magicNumber;
    }

    public long getTimeStamp()
    {
        return timeStamp;
    }

    public void setTimeStamp(long timeStamp)
    {
        this.timeStamp = timeStamp;
    }

    public String getPlatformID()
    {
        return platformID;
    }

    public void setPlatformID(String platformID)
    {
        this.platformID = platformID;
    }

    public String getSensorID()
    {
        return sensorID;
    }

    public void setSensorID(String sensorID)
    {
        this.sensorID = sensorID;
    }

    public long getReportID()
    {
        return reportID;
    }

    public void setReportID(long reportID)
    {
        this.reportID = reportID;
    }

    public long getTargetID()
    {
        return targetID;
    }

    public void setTargetID(long targetID)
    {
        this.targetID = targetID;
    }

    public double getTargetPositionLat()
    {
        return targetPositionLat;
    }

    public void setTargetPositionLat(double targetPositionLat)
    {
        this.targetPositionLat = targetPositionLat;
    }

    public double getTargetPositionLon()
    {
        return targetPositionLon;
    }

    public void setTargetPositionLon(double targetPositionLon)
    {
        this.targetPositionLon = targetPositionLon;
    }

    public double getTargetPositionZ()
    {
        return targetPositionAlt;
    }

    public void setTargetPositionZ(double targetPositionAlt)
    {
        this.targetPositionAlt = targetPositionAlt;
    }

    public double[] getTargetPositionError()
    {
        return targetPositionError;
    }

    public void setTargetPositionError(double[] targetPositionError)
    {
        this.targetPositionError = targetPositionError;
    }

    public double getTargetVelocityX()
    {
        return targetVelocityX;
    }

    public void setTargetVelocityX(double targetVelocityX)
    {
        this.targetVelocityX = targetVelocityX;
    }

    public double getTargetVelocityY()
    {
        return targetVelocityY;
    }

    public void setTargetVelocityY(double targetVelocityY)
    {
        this.targetVelocityY = targetVelocityY;
    }

    public double getTargetVelocityZ()
    {
        return targetVelocityZ;
    }

    public void setTargetVelocityZ(double targetVelocityZ)
    {
        this.targetVelocityZ = targetVelocityZ;
    }

    public double[] getTargetVelocityError() {
        return targetVelocityError;
    }

    public void setTargetVelocityError(double[] targetVelocityError)
    {
        this.targetVelocityError = targetVelocityError;
    }

    public String getTargetType()
    {
        return targetType;
    }

    public void setTargetType(String targetType)
    {
        this.targetType = targetType;
    }

    public String getTargetAnnotation()
    {
        return targetAnnotation;
    }

    public void setTargetAnnotation(String targetAnnotation)
    {
        this.targetAnnotation = targetAnnotation;
    }

    public String getCombatID()
    {
        return combatID;
    }

    public void setCombatID(String combatID)
    {
        this.combatID = combatID;
    }

    public long getCRC32()
    {
        return CRC32;
    }

    public void setCRC32(int cRC32)
    {
        CRC32 = cRC32;
    }

    @Override
    public String toString() {
        String s = "";
        for (Field field : this.getClass().getDeclaredFields()) {
            try {
                //field.setAccessible(true); // if you want to modify private fields
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
