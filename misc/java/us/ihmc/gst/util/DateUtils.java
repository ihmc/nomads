package us.ihmc.gst.util;

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Giacomo Benincasa        (gbenincasa@ihmc.us)
 */
public class DateUtils
{
    public static Date parseDate (String strDate)
    {
        for (DateFormat dt : _dataFormats) {
            try {
                return (Date)dt.parse (strDate);
            }
            catch (ParseException ex) {}
        }
        Logger.getLogger(DateUtils.class.getName()).log(Level.WARNING, null, "Unparseable date: " + strDate);
        return null;
    }

    public static void main (String[] args)
    {
        Date date;
        date = parseDate ("2010-11-15T00:17:35Z");
        assert (date != null);
        System.out.println (date);

        date = parseDate ("2001-12-31T12:00:00");
        assert (date != null);
        System.out.println (date);
    }

    private static DateFormat[] _dataFormats = { new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'"),
                                                 new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss"),
                                                 new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSSSS") };
}
