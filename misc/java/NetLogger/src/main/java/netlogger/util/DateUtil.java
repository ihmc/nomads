package netlogger.util;/*
Inspired by http://cokere.com/RFC3339Date.txt
All rights deserve to Chad Okere
*/

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class DateUtil
{
    public synchronized static String parseRFC3339Date (String dateString) throws java.text.ParseException, IndexOutOfBoundsException {
        Date d;
        String timestampString = "";


        //if there is no time zone, we don't need to do any special parsing.
        if (dateString.endsWith("Z")) {
            String removedPartialSeconds = dateString.split("\\.")[0];

            try {
                SimpleDateFormat s = new SimpleDateFormat("yyyy-MM-dd'T'hh:mm:ss", Locale.ENGLISH);//spec for RFC3339 with a 'Z'
                s.setTimeZone(TimeZone.getTimeZone("GMT"));
                d = s.parse(removedPartialSeconds);
                timestampString = hour12.format(d).toLowerCase();
            } catch (java.text.ParseException pe) {//try again with optional decimals
                SimpleDateFormat s = new SimpleDateFormat("yyyy-MM-dd'T'hh:mm:ss.SSSSSS'Z'", Locale.ENGLISH);//spec for RFC3339 with a 'Z' and fractional seconds
                s.setTimeZone(TimeZone.getTimeZone("GMT"));
                d = s.parse(dateString);

                timestampString = hour12.format(d);
            }
        }
        return timestampString;
    }

    public synchronized static String parseDate (long millis) {
        String timestampString = "";
        Date date = new Date(millis);
        timestampString = hour12.format(date);

        return timestampString;
    }

    private static DateFormat hour12 = new SimpleDateFormat("h:mm:ss a MM/dd/yyyy");
}