package us.ihmc.cot.parser;

import org.apache.log4j.Logger;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * A date-time value as per the Cursor on Target specification. This class ensures proper parsing
 * and string exporting are properly formated.
 * 
 * @author Joe Bergeron
 */
public class CotTime {

    private static class SimpleDateFormatThread extends ThreadLocal<SimpleDateFormat> {
        final String format;
        final Locale locale;
        final TimeZone tz;

        public SimpleDateFormatThread(String format, Locale locale, TimeZone tz) {
            this.format = format;
            this.locale = locale;
            this.tz = tz;
        }

        @Override
        protected SimpleDateFormat initialValue() {
            SimpleDateFormat sdf = new SimpleDateFormat(format, locale);
            sdf.setTimeZone(tz);
            return sdf;
        }

        /**
         * see SimpleDateFormat.format(Date)
         */
        public String format(Date date) {
            return this.get().format(date);
        }

        /**
         * see SimpleDateFormat.setTimeZone(TimeZone)
         */
        public void setTimeZone(TimeZone tz) {
            this.get().setTimeZone(tz);
        }

        /**
         * see SimpleDateFormat.parse(String)
         */
        public Date parse(String source) throws ParseException {
            return this.get().parse(source);
        }
    };

    private static final String TAG = "CotTime";

    private static final Logger Log = Logger.getLogger(CotTime.class);

    private long _dateTime;
    private String _toString;
    private final static SimpleDateFormatThread _COT_TIME_FORMAT =
            new SimpleDateFormatThread("yyyy-MM-dd'T'HH:mm:ss.S'Z'",
                    Locale.US,
                    TimeZone.getTimeZone("UTC"));
    private final static SimpleDateFormatThread _LIBERAL_COT_TIME_FORMAT =
            new SimpleDateFormatThread("yyyy-MM-dd'T'HH:mm:ss'Z'",
                    Locale.US,
                    TimeZone.getTimeZone("UTC"));

    /**
     * Create a CoT time in milliseconds since January 1st 1970 00:00:00 UTC.
     * 
     * @param milleseconds offset in milliseconds from Unix Epoch
     */
    public CotTime(long milleseconds) {
        _dateTime = milleseconds;
    }

    /**
     * Create a CoT time from a java Date object
     * 
     * @param date
     */
    public CotTime(Date date) {
        _dateTime = date.getTime();
    }

    public long millisecondDiff(CotTime other) {
        return _dateTime - other._dateTime;
    }

    /**
     * Copy Constructor.
     * 
     * @param time
     */
    public CotTime(CotTime time) {
        _dateTime = time._dateTime;
    }

    /**
     * Get a CoT time that is a positive or negative number of seconds from this Cot time
     * 
     * @param seconds number of seconds to add (may be negative)
     * @return
     */
    public CotTime addSeconds(int seconds) {
        return new CotTime(_dateTime + seconds * 1000);
    }

    /**
     * Get a CoT time that is a positive or negative number of minutes from this Cot time
     * 
     * @param minutes number of minutes to add (may be negative)
     * @return
     */
    public CotTime addMinutes(int minutes) {
        return addSeconds(minutes * 60);
    }

    /**
     * Get a CoT time that is a positive or negative number of hours from this Cot time
     * 
     * @param hours number of hours to add (may be negative)
     * @return
     */
    public CotTime addHours(int hours) {
        return addMinutes(hours * 60);
    }

    /**
     * Get a CoT time that is a positive or negative number of days from this Cot time
     * 
     * @param days number of days to add (may be negative)
     * @return
     */
    public CotTime addDays(int days) {
        return addHours(days * 24);
    }


    /**
     * Create a CoT time that represents the current system time
     */
    public CotTime() {
        _dateTime = new Date().getTime();
    }

    /**
     * Get this CoT time in milliseconds since January 1st 1970 00:00:00 UTC
     * 
     * @return
     */
    public long getMilliseconds() {
        return _dateTime;
    }

    static File directory = null;

    /**
     * Designate for refactoring. Set a reasonable log directory for bad_time, until this is
     * refactored.
     */
    static public void setLoggerDirectory(final File dir) {
        directory = dir;
        Log.debug("publish bad_time messages to: " + directory);
    }

    /**
     * Format a CoT time string from a java Date
     * 
     * @param date
     * @return
     */
    public static String formatTime(Date date) {
        String time = "2009-09-15T21:00:00.00Z"; // A not so random date, done in case the date
                                                 // library is sick - Andrew
        // Log.e(TAG, "Trying to read time now");
        try {
            time = _COT_TIME_FORMAT.format(date);
        } catch (IllegalArgumentException e) {
            String msg = "Error in formatTime - the input time is probably not formated correctly or is not a valid time"
                    +
                    "\r\n" + date.toString();// +
            /*
             * "/r/nYear: " + date.getYear() + "/r/nMonth: " + date.getMonth() + "/r/nDay: " +
             * date.getDay() + "/r/nHours: " + date.getHours() + "/r/nMinutes: " + date.getMinutes()
             * + "/r/nMinutes: " + date.getSeconds() + "/r/nSeconds: Unavailable");
             */

            // XXX Designate for refactoring. Should happen higher up?
            Log.error(msg);
            if (directory != null) {
                PrintWriter w = null;
                try {
                    w = new PrintWriter(new File(directory, "bad_time.txt"));
                    w.append(msg + "\r\n");
                    e.printStackTrace(w);
                } catch (FileNotFoundException e1) {
                    e1.printStackTrace();
                }
                if (w != null)
                    w.close();
            }
        }
        return time;
    }

    /**
     * Format a CoT time string
     * 
     * @param time
     * @return
     */
    public static String formatTime(CotTime time) {
        return formatTime(time.getDate());
    }

    /**
     * Parse a CoT time String. The parser is somewhat lenient in that it accepts a string with or
     * without the millisecond decimal portion.
     * 
     * @param formattedTime
     * @return
     * @throws ParseException
     */
    public static CotTime parseTime(String formattedTime) throws ParseException {
        Date date = null;
        try {
            date = _COT_TIME_FORMAT.parse(formattedTime);
        } catch (Exception ex) {
            date = _LIBERAL_COT_TIME_FORMAT.parse(formattedTime);
        }
        return new CotTime(date.getTime());
    }

    private static final Calendar CachedCalendar = new GregorianCalendar();
    static {
        CachedCalendar.setTimeZone(TimeZone.getTimeZone("GMT"));
        CachedCalendar.setLenient(false);
        CachedCalendar.clear();
    }

    /**
     * Assuming no sign and standard integers without issues.
     */
    public static int parseNonZeroInt(final String s) {
        final int len = s.length();
        int i = 0;
        int tmp = 0;
        int num = 0;

        while (i < len) {
            tmp = s.charAt(i++) - '0';
            if (tmp < 0 || tmp > 9)
                throw new NumberFormatException("string " + s
                        + " contains a non digit in position " + i);
            num = num * 10 + tmp;
        }

        return num;
    }

    /**
     * Parse a CoT time String in the form: 2009-09-15T21:00:00.00Z without using simple date
     * parser. Thread-safe
     * 
     * @param date
     * @return
     * @throws ParseException
     */
    synchronized public static CotTime parseTime2(final String date) throws ParseException {
        try {
            if (date.length() > 19) {
                int y = parseNonZeroInt(date.substring(0, 4));
                int m = parseNonZeroInt(date.substring(5, 7));
                --m;
                int d = parseNonZeroInt(date.substring(8, 10));
                int h = parseNonZeroInt(date.substring(11, 13));
                int mm = parseNonZeroInt(date.substring(14, 16));
                int s = parseNonZeroInt(date.substring(17, 19));

                int ms = 0;
                if (date.length() > 20)
                    ms = parseNonZeroInt(date.substring(20, 21));

                CachedCalendar.set(y, m, d, h, mm, s);

                // Log.d(TAG, "original date = " + date );
                // Log.d(TAG, "y = " + y + " m = " + m + " d = " + d + " h = " + h + " mm = " + mm +
                // " s = " + s + " ms " + ms);

                return new CotTime(CachedCalendar.getTime().getTime() + ms);
            }
        } catch (Exception e) {
            Log.debug("exception occured parsing: " + date);
        }

        // fall back to the original implementation.
        return parseTime(date);
    }

    /**
     * Convert to a java Date
     * 
     * @return
     */
    public Date getDate() {
        return new Date(_dateTime);
    }

    /**
     * Get the string representation of this CoT time (same as formatTime()).
     */
    public String toString() {
        if (_toString == null) {
            _toString = formatTime(this);
        }
        return _toString;
    }
}
