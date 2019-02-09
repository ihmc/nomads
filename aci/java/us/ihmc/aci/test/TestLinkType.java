package us.ihmc.aci.test;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import us.ihmc.aci.test.ddam.*;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;

import static us.ihmc.aci.nodemon.util.Utils.writeToFile;

public class TestLinkType{


    /**
     * create a protobuf object linktype
     * @param data a string of data used to build the protobuf obj
     * @return the proto obj
     */
    private static LinkType createLink(String[] data){
        InfoType it = InfoType.newBuilder().setVersion(data[0]).setDescription(data[1]).build();
        LinkValues lv = LinkValues.newBuilder().setThroughput(Double.parseDouble(data[3])).setLatency(Integer.parseInt(data[4])).setFrequency(Integer.parseInt(data[5])).setMuxScheme(data[2]).build();
        LinkType lt = LinkType.newBuilder().setId(data[6]).setInfo(it).setValues(lv).build();

        return lt;
    }


    public static boolean createLinkTypeArrayFromFiles(){

        boolean linkTypesPresent = true;

        File folder = new File(_jsonDirectory);
        File[] listOfFiles = folder.listFiles();

        ArrayList<LinkType.Builder> linkTypeList = new ArrayList<LinkType.Builder>();

        if(listOfFiles.length > 0) {
            for (int i = 0; i < listOfFiles.length; i++) {
                if (listOfFiles[i].isFile()) {
                    linkTypeList.add(readJsonFile(listOfFiles[i].getName()));
                }
            }
        }

        if(linkTypeList.size() == 0){
            linkTypesPresent = false;
        }else{
            _linkTypeArray = new LinkType.Builder[linkTypeList.size()];
            _linkTypeArray = linkTypeList.toArray(_linkTypeArray);
        }

        return linkTypesPresent;
    }

    /**
     * reads the json file and changes it in a protobuf object LinkTypeList
     * @return
     */
    private static LinkType.Builder readJsonFile(String file){
        BufferedReader br = null;
        try {
            FileReader fr = new FileReader(_jsonDirectory + file);
            br = new BufferedReader(fr);
        }catch(Exception e){
            System.out.println("The file doesn't exist.");
        }
        String output = "";
        String newline = "";
        try {
            newline = br.readLine();
            while(newline != null){
                output = output + newline;
                newline = br.readLine();
            }
        } catch (IOException e) {
            System.out.println("End of file");
        }

        if(!output.equals("")){
            try {
                LinkType.Builder lt = LinkType.newBuilder();
                JsonFormat.parser().merge(output, lt);
                return lt;
            } catch (InvalidProtocolBufferException e) {
                System.out.println("Error during the parsing from Json...");
                e.printStackTrace();
            }
        }else{
            System.out.println("Error reading json file.");
        }

        return null;
    }

    /**
     * Finds the link type from the link type given by json file
     * @param throughput
     * @param latency
     * @return
     */
    private static String parametricWinnerSelection(double throughput, int latency){
        int points = 0, max = -100;
        String idMax = "UNKNOWN";

        LinkType linkType;

        for(String linkTypeId : _linkTypes.keySet()){
            linkType = _linkTypes.get(linkTypeId);
            points = pointsForThroughput(throughput, _linkTypes.size() * _accuracy, linkType);
            points = points + pointsForLatency(latency, _linkTypes.size() * _accuracy, linkType);
            if(points > max){
                max = points;
                idMax = linkTypeId;
            }
        }

        return idMax;
    }

    /**
     * Sets the throughput parameters for the election algorithm on every link type object
     * @return
     */
    private static boolean setElectionAlgorithmThroughputParameters(){
        int points = 0, max = -100;
        double previousBw = 0, actualBw;
        LinkThroughputBounds linkAlgorithmThroughput;
        double differenceCoef;
        int linkTypesCount = _linkTypeArray.length * _accuracy;

        for(int i = 0; i < _linkTypeArray.length; i++){
            actualBw = _linkTypeArray[i].getValues().getThroughput();
            differenceCoef = (actualBw - previousBw)/linkTypesCount;
            linkAlgorithmThroughput = LinkThroughputBounds.newBuilder().setLowerThroughput(previousBw)
                    .setThroughputStep(differenceCoef).build();
            _linkTypeArray[i].getBoundsBuilder().setLinkThroughputBounds(linkAlgorithmThroughput);
            previousBw = actualBw;
        }

        return true;
    }

    /**
     * Sets the latency parameters for the election algorithm on every link type object
     * @return
     */
    private static boolean setElectionAlgorithmLatencyParameters(){
        int previousLat = 0, actualLat, nextLat = 0;
        LinkLatencyBounds linkAlgorithmLatency;
        int upperDifferenceCoef, lowerDifferenceCoef;
        int linkTypesCount = _linkTypeArray.length * _accuracy;

        if(_linkTypeArray.length > 0) {
            nextLat = _linkTypeArray[0].getValues().getLatency();
        }

        for(int i = 0; i < _linkTypeArray.length; i++){
            actualLat = nextLat;
            lowerDifferenceCoef = (actualLat - previousLat)/linkTypesCount;
            if((i+1) == _linkTypeArray.length) {
                nextLat = actualLat * 100;
            }else{
                nextLat = _linkTypeArray[i+1].getValues().getLatency();
            }
            upperDifferenceCoef = (nextLat - actualLat)/linkTypesCount;
            linkAlgorithmLatency = LinkLatencyBounds.newBuilder().setLowerLatency(previousLat)
                    .setUpperLatency(nextLat).setLowerLatencyStep(lowerDifferenceCoef)
                    .setUpperLatencyStep(upperDifferenceCoef).build();
            _linkTypeArray[i].getBoundsBuilder().setLinkLatencyBounds(linkAlgorithmLatency);
            previousLat = actualLat;
        }

        return true;
    }

    /**
     * parametric method to give a score on bw for a type of network
     * @param bwChecked bandwidth given from the worldstate
     * @param count number of link types
     * @param linkType linkType to analize
     * @return
     */
    private static int pointsForThroughput(double bwChecked, double count, LinkType linkType){
        double upperBandwidth, lowerBandwidth;
        double step = linkType.getBounds().getLinkThroughputBounds().getThroughputStep();
        int points = 100;
        upperBandwidth = linkType.getValues().getThroughput();
        for(int i=1; i <= count; i++){
            lowerBandwidth = upperBandwidth - (step * i);
            if((bwChecked <= upperBandwidth) && (bwChecked > lowerBandwidth)){
                return (points / i);
            }
            upperBandwidth = lowerBandwidth;
        }
        if(bwChecked <= upperBandwidth){
            return (int)(points / (count + 1));
        }
        points = - (int)(points/count);
        return points;
    }

    /**
     * parametric method to give a score on latency for a type of network
     * @param latChecked latency given from the worldstate
     * @param count number of link types
     * @param linkType linkType to analize
     * @return
     */
    private static int pointsForLatency(int latChecked, double count, LinkType linkType){
        int upperLatency, lowerLatency;
        int step = linkType.getBounds().getLinkLatencyBounds().getLowerLatencyStep();
        int points = 100;
        upperLatency = linkType.getValues().getLatency();
        for(int i=1; i <= count; i++){
            lowerLatency = upperLatency - (step * i);
            if((latChecked <= upperLatency) && (latChecked > lowerLatency)){
                return (points / i);
            }
            upperLatency = lowerLatency;
        }

        points = 100;
        lowerLatency = linkType.getValues().getLatency();
        step = linkType.getBounds().getLinkLatencyBounds().getUpperLatencyStep();
        for (int i = 1; i <= count; i++) {
            upperLatency = lowerLatency + (step * i);
            if ((latChecked <= upperLatency) && (latChecked > lowerLatency)) {
                return (points / i);
            }
            lowerLatency = upperLatency;
        }
        return (int)(points / (count + 1));
    }

    public static String getJsonString(LinkType linkType){
        StringBuilder sb = new StringBuilder();
        try {
            sb.append(JsonFormat.printer().print(linkType));
        }catch(Exception e){
            System.out.println("Error during the parsing to Json...");
        }

        return sb.toString();
    }

    public static void main(String[] args) throws Exception{
        _jsonDirectory = "/home/nomads/Desktop/json_files/";
        _accuracy = 2;

        String[][] inputTypes = {{"1","","","100000000","10","1000","LAN"},
                {"12","","","1000000","200","1000","SATCOM"},
                {"13","","","56000","100","1000","HF"}};


        /*
        String[][] inputTypes = {{"","","","1000000000","100","1000","GOOD"},
                {"","","","1000000","400","1000","BAD"}};
*/
        double bwFound = 700000;
        int latFound = 180;

        if(args.length >= 2){
            bwFound = Double.parseDouble(args[0]);
            latFound = Integer.parseInt(args[1]);
        }

        LinkType lt = null;

        boolean allWritten = true;

        for(int i=0; i < inputTypes.length; i++){
            lt = createLink(inputTypes[i]);
            allWritten = allWritten && writeToFile(_jsonDirectory, _jsonDirectory + lt.getId().toLowerCase() + ".json", getJsonString(lt).getBytes());
        }

        if(allWritten){
            if(createLinkTypeArrayFromFiles()) {

                System.out.println("Non-ordered list:");
                for (int i = 0; i < _linkTypeArray.length; i++) {
                    System.out.println("Id: " + _linkTypeArray[i].getId());
                    System.out.println("Description: " + _linkTypeArray[i].getInfo().getDescription());
                    System.out.println("Version: " + _linkTypeArray[i].getInfo().getVersion());
                    System.out.println("Mux: " + _linkTypeArray[i].getValues().getMuxScheme());
                    System.out.println("BW: " + _linkTypeArray[i].getValues().getThroughput());
                    System.out.println("Lat: " + _linkTypeArray[i].getValues().getLatency());
                    System.out.println("Freq: " + _linkTypeArray[i].getValues().getFrequency());
                }

                Arrays.sort(_linkTypeArray, new Comparator<LinkTypeOrBuilder>() {

                    @Override
                    public int compare(final LinkTypeOrBuilder l1, final LinkTypeOrBuilder l2) {
                        if (l1.getValues().getThroughput() > l2.getValues().getThroughput()) {
                            return 1;
                        } else {
                            if (l1.getValues().getThroughput() < l2.getValues().getThroughput()) {
                                return -1;
                            }
                        }
                        return 0;
                    }

                });

                System.out.println("BW-ordered list:");
                for (int i = 0; i < _linkTypeArray.length; i++) {
                    System.out.println("Id: " + _linkTypeArray[i].getId());
                    System.out.println("BW: " + _linkTypeArray[i].getValues().getThroughput());
                }


                setElectionAlgorithmThroughputParameters();

                Arrays.sort(_linkTypeArray, new Comparator<LinkTypeOrBuilder>() {

                    @Override
                    public int compare(final LinkTypeOrBuilder l1, final LinkTypeOrBuilder l2) {
                        if (l1.getValues().getLatency() > l2.getValues().getLatency()) {
                            return 1;
                        } else {
                            if (l1.getValues().getLatency() < l2.getValues().getLatency()) {
                                return -1;
                            }
                        }
                        return 0;
                    }

                });

                System.out.println("Lat-ordered list:");
                for (int i = 0; i < _linkTypeArray.length; i++) {
                    System.out.println("Id: " + _linkTypeArray[i].getId());
                    System.out.println("Lat: " + _linkTypeArray[i].getValues().getLatency());
                }

                setElectionAlgorithmLatencyParameters();

                _linkTypes = new HashMap<>();

                for (int i = 0; i < _linkTypeArray.length; i++) {
                    _linkTypes.put(_linkTypeArray[i].getId(),_linkTypeArray[i].build());
                }

                for(String id : _linkTypes.keySet()){
                    System.out.println(_linkTypes.get(id));
                }

                System.out.println("Link type: " + parametricWinnerSelection(bwFound,latFound));
            }

        }else{
            System.out.println("Error writing json file.");
        }


    }



    private static String _jsonDirectory;


    //the string is the id of the link type
    private static LinkType.Builder[] _linkTypeArray;
    private static Map<String, LinkType> _linkTypes;
    private static int _accuracy;
}