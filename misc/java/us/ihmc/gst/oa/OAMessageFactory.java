package us.ihmc.gst.oa;

import java.io.IOException;
import mil.navy.nrlssc.gst.oa.BlueForceMessage;
import mil.navy.nrlssc.gst.oa.OAMessage;
import mil.navy.nrlssc.gst.oa.TargetReportMessage;
import us.ihmc.gst.util.GHubDataInputStream;

/**
 *
 * @author GiacomoBenincasa         (gbenincasa@ihmc.us)
 */
public class OAMessageFactory
{
    public enum Binary {
        BFT,
        TARGET_REPORT
    }

    public enum XML {
        BFT_Salute,
        BFT_Status,
        Sensor,
        Update
    }

    public static OAMessage getOAMessage (GHubDataInputStream dataInput, int dataLen) throws IOException
    {
        // Binary Messages
        dataInput.mark (dataLen);
        boolean isMsg = BlueForceMessage.readAndCheckMagicNumber (dataInput);
        dataInput.reset();
        if (isMsg) {
            return new BlueForceMessage();
        }

        dataInput.mark (dataLen);
        isMsg = TargetReportMessage.readAndCheckMagicNumber (dataInput);
        dataInput.reset();
        if (isMsg) {
            return new TargetReportMessage();
        }

        // XML Messages
        OAMessage msg = null;
        dataInput.mark (dataLen);
        String xml = new String (dataInput.readByteArray (dataLen));
        if (xml.contains (BlueForceSaluteMessage.MAGIC_STRING)) {
            msg = new BlueForceSaluteMessage();
        }
        else if (xml.contains (BlueForceStatusMessage.MAGIC_STRING)) {
            msg = new BlueForceStatusMessage();
        }
        else if (xml.contains (EntityIdentMessage.MAGIC_STRING)) {
            msg = new EntityIdentMessage();
        }
        else if (xml.contains (SensorStatusMessage.MAGIC_STRING)) {
            msg = new SensorStatusMessage();
        }
        else if (xml.contains (UpdateMessage.MAGIC_STRING)) {
            msg = new UpdateMessage();
        }
        dataInput.reset();

        return msg;
    }
}
