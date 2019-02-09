/**
 * InvokeResponseEncoder
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 *
 */

package us.ihmc.aci.agserve;

import us.ihmc.util.Dime;
import java.io.OutputStream;

/**
 *
 */
public class InvokeResponseEncoder
{
    public InvokeResponseEncoder()
    {
        _agServeUtils = new AgServeUtils();
    }

    /**
     *
     */
    public byte[] encode (String methodName, Object response, boolean isBinaryRequest)
    {
        byte[] resp = null;

        if (isBinaryRequest) {
            try {
                Dime dimeMsg = _agServeUtils.encodeObjectInDime (response);
                resp = dimeMsg.getDime();
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        else {
            StringBuffer sb = new StringBuffer();
            sb.append("<" + methodName + "Response>\n");
            sb.append("<return>");

            if (response != null) {
                if (response instanceof Integer) {
                    int res = ((Integer) response).intValue();
                    sb.append(res);
                }
            }

            sb.append("</return>\n");
            sb.append("</" + methodName + "Response>");

            String xmlResp = sb.toString();

            Dime dimeMsg = new Dime();
            dimeMsg.addRecord (methodName + "Response",
                               xmlResp.getBytes(),
                               "");

            resp = dimeMsg.getDime();
        }

        return resp;
    } //encodeResponse()


    /**
     *
     */
    public void encode (String methodName, Object response, boolean isBinaryRequest, OutputStream os)
        throws Exception
    {
        if (isBinaryRequest) {
            try {
                _agServeUtils.encodeObjectInDime (response, os);
                os.flush();
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        else {
            StringBuffer sb = new StringBuffer();
            sb.append("<" + methodName + "Response>\n");
            sb.append("<return>");

            if (response != null) {
                if (response instanceof Integer) {
                    int res = ((Integer) response).intValue();
                    sb.append(res);
                }
            }

            sb.append("</return>\n");
            sb.append("</" + methodName + "Response>");

            String xmlResp = sb.toString();

            Dime dimeMsg = new Dime();
            dimeMsg.addRecord (methodName + "Response",
                               xmlResp.getBytes(),
                               "");

            dimeMsg.getDime(os);
            os.flush();
        }
    } //encodeResponse()


    // /////////////////////////////////////////////////////////////////////////
    private AgServeUtils _agServeUtils;
}
