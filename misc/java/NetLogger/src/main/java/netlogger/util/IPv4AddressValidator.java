package netlogger.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class IPv4AddressValidator
{
    private Pattern _pattern;
    private Matcher _matcher;

    private static final String IPv4ADDRESS_PATTERN =
            "^([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
                    "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
                    "([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\." +
                    "([01]?\\d\\d?|2[0-4]\\d|25[0-5])$";

    public IPv4AddressValidator () {
        _pattern = Pattern.compile(IPv4ADDRESS_PATTERN);
    }

    public boolean validateIP (String ipv4Address) {
        _matcher = _pattern.matcher(ipv4Address);
        return _matcher.matches();
    }
}
