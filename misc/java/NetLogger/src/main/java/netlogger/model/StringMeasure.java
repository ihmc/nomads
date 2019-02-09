package netlogger.model;

import netlogger.util.DateUtil;

import java.util.List;

public class StringMeasure
{
    public StringMeasure () {
        // Create a default timestamp in case one isnt provided
        _timestamp = DateUtil.parseDate(System.currentTimeMillis());
        _measureStringsString = "";
    }


    public void setTimestamp (String timestamp) {
        _timestamp = timestamp;
    }

    public String getTimestamp () {
        return _timestamp;
    }


    public void setString (String string) {
        _measureStringsString = string;
    }

    public String getString () {
        return _measureStringsString;
    }

    /**
     * Keylist is not used right now
     * @param keys
     */
    @Deprecated
    public void setKeyList (List<String> keys) {
        _keyList = keys;
    }

    /**
     * Keylist is not currently used right now
     * @return
     */
    @Deprecated
    public List<String> getKeyList () {
        return _keyList;
    }

    public void setNatsTopic (String natsTopic) {
        _natsTopic = natsTopic;
    }

    public String getNatsTopic () {
        return _natsTopic;
    }


    private List<String> _keyList;
    private String _timestamp;
    private String _measureStringsString;
    private String _natsTopic;
}
