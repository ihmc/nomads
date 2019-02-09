/**
 * InvokeRequestParser
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.agserve;

import java.io.*;
import java.util.Vector;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import us.ihmc.aci.agserve.AgServeUtils;
import us.ihmc.util.Dime;
import us.ihmc.util.DimeRecord;
import us.ihmc.algos.Statistics;

/**
 *
 */
public class InvokeRequestParser
{
    public InvokeRequestParser (InputStream dimeInputStream) throws IOException
    {
        this (dimeInputStream, null);
    }

    public InvokeRequestParser (InputStream dimeInputStream, ClassLoader classLoader) throws IOException
    {
        _methodName = null;
        _agServeUtils = new AgServeUtils (classLoader);

        long timestamp;

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis();
        }

        //try {
            _dimeMsg = new Dime (dimeInputStream);
	    //} catch (IOException e) {
	    //  e.printStackTrace();
	    // return;
	    // }

        if (BENCHMARK) {
            timestamp = System.currentTimeMillis() - timestamp;
            _dimeParsingStats.update (timestamp);
        }

        this.parse();
    }

    /**
     *
     */
    public String getMethodName()
    {
        return _methodName;
    }

    /**
     *
     */
    public Class[] getParameterTypes()
    {
        return _paramTypes;
    }

    /**
     *
     */
    public Object[] getParameterValues()
    {
        return _paramValues;
    }

    /**
     *
     */
    public boolean isBinaryRequest()
    {
        return _isBinaryRequest;
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private String replaceAll (String original, String toReplace, String newStr)
    {
        StringBuffer sb = new StringBuffer();
        int lastIndex = 0;
        int index;
        while ( (index = original.indexOf(toReplace, lastIndex)) >= 0 ) {
            sb.append (original.substring(lastIndex, index));
            sb.append (newStr);

            lastIndex = index + toReplace.length();
        }

        if (lastIndex < original.length() - 1) {
            sb.append(original.substring(lastIndex, original.length() - 1));
        }

        return sb.toString();
    }

    /**
     *
     */
    private void parseBinaryRequest()
    {
        try {
            _paramValues = _agServeUtils.decodeBinaryRequestParams (_dimeMsg);
            if (_paramValues == null) {
                _paramTypes = null;
                return;
            }

            _paramTypes = new Class[_paramValues.length];
            for (int i = 0; i < _paramValues.length; i++) {
                if (_paramValues[i] == null) {
                    _paramTypes[i] = null;
                }
                else {
                    _paramTypes[i] = _paramValues[i].getClass();
                }
            }
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     * <addRequest>
     *   <a>10</a>
     *   <b>20</b>
     * </addRequest>
     *
     */
    private void parse()
    {
        DimeRecord dr = (DimeRecord) _dimeMsg.getRecords().nextElement();

        if ("binary_invoke_request".equals(dr.getType())) {
            _isBinaryRequest = true;

            this.parseBinaryRequest();
            return;
        }

        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();

            Vector methodParamsInfo = new Vector();

            byte[] docBytes = dr.getPayload();
            ByteArrayInputStream bais = new ByteArrayInputStream (docBytes);

            Document doc = builder.parse (bais);

            // start parsing.
            Element root = doc.getDocumentElement();
            _methodName = root.getNodeName();
            _methodName = this.replaceAll(_methodName, "Request", "");

            NodeList nodeList = root.getChildNodes();

            for (int i = 0; i < nodeList.getLength(); i++) {
                if (nodeList.item(i) instanceof Element) {
                    Element elem = (Element) nodeList.item(i);
                    String paramName = elem.getNodeName();

                    NodeList paramValList = elem.getChildNodes();
                    Node n = paramValList.item(0);
                    String paramValue = n.getNodeValue();

                    ParameterInfo pi = this.getParameterInfo (paramName, paramValue);
                    methodParamsInfo.add (pi);
                }
            }

            int numParams = methodParamsInfo.size();
            _paramTypes = new Class[numParams];
            _paramValues = new Object[numParams];

            for (int i = 0; i < numParams; i++) {
                ParameterInfo pi = (ParameterInfo) methodParamsInfo.get (i);
                _paramTypes[i] = pi.getType();
                _paramValues[i] = pi.getValue();
            }
         }
         catch (Exception ex) {
             ex.printStackTrace();
         }
    }

    /**
     *
     */
    private ParameterInfo getParameterInfo (String paramName, String paramValue)
    {
        try {
            Integer val = new Integer (paramValue);
            ParameterInfo paramInfo = new ParameterInfo(paramName, Integer.TYPE, val);
            return paramInfo;
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }

        return null;
    }

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    public class ParameterInfo
    {
        public ParameterInfo(String name, Class type, Object value)
        {
            _name = name;
            _type = type;
            _value = value;
        }

        /**
         *
         */
        public String getName()
        {
            return _name;
        }

        /**
         *
         */
        public Class getType()
        {
            return _type;
        }

        /**
         *
         */
        public Object getValue()
        {
            return _value;
        }

        private String _name;
        private Class  _type;
        private Object _value;
    } // class ParameterInfo

    // /////////////////////////////////////////////////////////////////////////
    private Dime _dimeMsg;
    private AgServeUtils _agServeUtils;

    private Class[] _paramTypes = null;
    private Object[] _paramValues = null;

    private String _methodName = null;
    private boolean _isBinaryRequest = false;

    private static final boolean BENCHMARK = false;
    public static Statistics _dimeParsingStats;


    // /////////////////////////////////////////////////////////////////////////
    // MAIN METHOD  (for testing purposes only) ////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     *
     */
    public static void main(String[] args) throws Exception
    {
        String xml = "<addRequest>\n"
                   + "<a>10</a>\n"
                   + "<b>20</b>"
                   + "</addRequest>"
                   ;

        Dime dimeMsg = new Dime();
        dimeMsg.addRecord("", xml.getBytes(), "");

        ByteArrayInputStream bais = new ByteArrayInputStream(dimeMsg.getDime());
        InvokeRequestParser parser = new InvokeRequestParser(bais);

        System.out.println("MethodName :: " + parser.getMethodName());
        Object[] vals = parser.getParameterValues();
        Object[] types = parser.getParameterTypes();

        for (int i = 0; i < vals.length; i++) {
            System.out.println("param[" + i +"]: " + types[i] +":" + vals[i]);
        }
    }

    static {
        if (BENCHMARK) {
            _dimeParsingStats = new Statistics();
        }
    }
}
