import java.io.*;
import java.util.StringTokenizer;

public class DeltaTime
{
    public static void main(String[] args) throws Exception
    {
        if (args.length != 2) {
            System.out.println("usage: DeltaTime <sourceFile> <targetFile>");
            System.exit(1);
        }

        String source = args[0];
        String target = args[1];

        FileReader fr = new FileReader(source);
        BufferedReader br = new BufferedReader(fr);

        FileWriter fw = new FileWriter(target);
        BufferedWriter bw = new BufferedWriter(fw);

        String line;
        long firstTimestamp = -1;

        while ( (line = br.readLine()) != null ) {
            if (line.trim().equals("")) {
                continue;
            }

            StringTokenizer st = new StringTokenizer(line);
            String strTimestamp = st.nextToken();
            long timestamp = Long.parseLong( strTimestamp );

            if (firstTimestamp == -1) {
                firstTimestamp = timestamp;
            }

            timestamp = timestamp - firstTimestamp;
            line = line.replaceAll("^" + strTimestamp, Long.toString(timestamp));

            bw.write(line);
            bw.newLine();
            bw.flush();
        }

        fr.close();
        br.close();

        fw.close();
        bw.close();

        System.out.println("\ndone!");
    }
}
/*
 * vim: et ts=4 sw=4
 */
