package us.ihmc.aci.util.dspro.soi;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.regex.Pattern;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class SOIIDGen {

    private static final Pattern ID_PATTERN = Pattern.compile("[A-Z, 0-9]{3}[\\d]{4,16}");
    private static final Pattern POSTFIX = Pattern.compile("[\\d]{4,16}");
    private static final Pattern PREFIX = Pattern.compile("[A-Z, 0-9]{1,3}");
    private static final Pattern START_OF_ID_PATTERN = Pattern.compile("[A-Z]{3}[\\d]{1,16}");
    private static final Pattern START_OF_ID_PATTERN_WITH_DASH = Pattern.compile("[A-Z]{1,3}-[\\d]{1,16}");
    private static final Pattern START_OF_ID_PATTERN_WITH_UNDERSCORE = Pattern.compile("[A-Z]{1,3}[_]{1}[\\d]{1,16}");

    private static int getCharPosInAlphabet(char c) {
        int temp = (int)c;
        int temp_integer = 64; //for upper case
        if(temp<=90 & temp>=65) {
            return temp-temp_integer;
        }
        return 0;
    }

    private static byte[] createChecksum(String s) throws NoSuchAlgorithmException, IOException {
       InputStream fis = new ByteArrayInputStream(s.getBytes());
       byte[] buffer = new byte[1024];
       MessageDigest complete = MessageDigest.getInstance("MD5");
       int numRead;

       do {
           numRead = fis.read(buffer);
           if (numRead > 0) {
               complete.update(buffer, 0, numRead);
           }
       } while (numRead != -1);

       fis.close();
       return complete.digest();
   }

   private static String getMD5Checksum(String s) throws NoSuchAlgorithmException, IOException {
       byte[] b = createChecksum(s);
       String result = "";

       for (int i=0; i < b.length; i++) {
           result += Integer.toString( ( b[i] & 0xff ) + 0x100, 16).substring( 1 );
       }
       return result;
   }

    private static String hashId(String dsproId) throws NoSuchAlgorithmException, IOException {
        String mac = getMD5Checksum(dsproId);
        final Pattern digit = Pattern.compile("\\d");
        final Pattern character = Pattern.compile("[A-Z]");
        final char[] alphabet = "ABCDEFGHIJ".toCharArray();
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < Math.min(mac.length(), 12); i++) {
            char c = mac.charAt(i);
            String ch = new StringBuilder().append(c).toString();
            ch = ch.toUpperCase();
            if (i < 3) {
                // must be a char
                if (digit.matcher(ch).matches()) {
                    result.append(alphabet[Integer.parseInt(ch)]);
                }
                else {
                    result.append(ch);
                }
            }
            else {
                // must be a digit
                if (character.matcher(ch).matches()) {
                    result.append(getCharPosInAlphabet(c));
                }
                else {
                    result.append(ch);
                }
            }
        }
        return result.toString();
    }

    public static String genId(String dsproId) throws NoSuchAlgorithmException, IOException {
        if (dsproId.length() <= 19) {
            String initialId = dsproId.toUpperCase();
            if (ID_PATTERN.matcher(initialId).matches()) {
                return initialId;
            }
            if (POSTFIX.matcher(initialId).matches()) {
                StringBuilder sb = new StringBuilder("AAA").append(initialId);
                while (sb.length() < 7) {
                    sb = sb.append("0");
                }
                return sb.toString();
            }
            if (PREFIX.matcher(initialId).matches()) {
                StringBuilder sb = new StringBuilder(initialId);
                while (sb.length() < 3) {
                    sb = sb.append("A");
                }
                return sb.append("0000").toString();
            }
            boolean isStartOfIdWithDash = START_OF_ID_PATTERN_WITH_DASH.matcher(initialId).matches();
            boolean isStartOfIdWithUnderscore = START_OF_ID_PATTERN_WITH_UNDERSCORE.matcher(initialId).matches();
            if (isStartOfIdWithDash || isStartOfIdWithUnderscore || START_OF_ID_PATTERN.matcher(initialId).matches()) {
                StringBuilder sb = null;
                if(isStartOfIdWithDash || isStartOfIdWithUnderscore) {
                    // rmeove dash
                    String separator = isStartOfIdWithDash ? "-" : "_";
                    String[] parts = initialId.split(separator);
                    StringBuilder pre = new StringBuilder(parts[0]);
                    while (pre.length() < 3) {
                        pre.append("A");
                    }
                    StringBuilder post = new StringBuilder(parts[1]);
                    while (post.length() < 4) {
                        post.append("0");
                    }
                    sb = new StringBuilder(pre).append(post);
                }
                else {
                    sb = new StringBuilder(initialId);
                }
                while (sb.length() < 12) {
                    sb = sb.append("0");
                }
                return sb.toString();
            }
        }
        return hashId(dsproId);
    }  
}
