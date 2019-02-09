package us.ihmc.aci.netSupervisor.link;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.test.ddam.LinkLatencyBounds;
import us.ihmc.aci.test.ddam.LinkThroughputBounds;
import us.ihmc.aci.test.ddam.LinkType;
import us.ihmc.aci.test.ddam.LinkTypeOrBuilder;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;

/**
 * Created by Emanuele Tagliaferro on 10/28/16.
 */
public class LinkTypeContainer
{

    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     *  Adds a link type to the link type list
     * @param linkType
     */
    public static void addLinkType(LinkType linkType)
    {
        if(_linkTypes == null)
        {
            _linkTypes = new HashMap<>();
        }

        if(linkType != null) {
            _linkTypes.put(linkType.getId(), linkType);
        }
    }

    /**
     * Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Creates the array with the link types from the directory with all the json files
     * @return true if there's at least one link type in the array
     */
    private static boolean createLinkTypeArrayFromFiles()
    {

        boolean linkTypesPresent = true;

        File folder = new File(_jsonDirectory);
        File[] listOfFiles = folder.listFiles();

        ArrayList<LinkType.Builder> linkTypeBuilderList = new ArrayList<LinkType.Builder>();

        if(listOfFiles.length > 0) {
            for (int i = 0; i < listOfFiles.length; i++) {
                if (listOfFiles[i].isFile()) {
                    linkTypeBuilderList.add(readLinkTypeFromJsonFile(listOfFiles[i].getName()));
                }
            }
        }

        if(linkTypeBuilderList.size() == 0) {
            linkTypesPresent = false;
        }
        else {
            _linkTypeBuilderArray = new LinkType.Builder[linkTypeBuilderList.size()];
            _linkTypeBuilderArray = linkTypeBuilderList.toArray(_linkTypeBuilderArray);
        }

        return linkTypesPresent;
    }



    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     *  returns the list of the link type with a specific id
     * @return
     */
    public static LinkType getLinkTypeById(String linkTypeId)
    {
        return _linkTypes.get(linkTypeId);
    }

    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     *  returns the list of link types
     * @return
     */
    public static Map<String, LinkType> getLinkTypes()
    {
        return _linkTypes;
    }


    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     * returns the number of link types in the container object
     * @return
     */
    public static int getLinkTypesNumber()
    {
        if(_linkTypes != null) {
            return _linkTypes.size();
        }

        return 0;
    }

    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     *  Returns the accuracy parameter
     * @return
     */
    public static int getAccuracy()
    {
        return _accuracy;
    }

    /**
     * Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Loads all the link types from the JSON folder
     * @return true if there is at least one link type
     */
    public static boolean loadAllLinkTypesFromFiles()
    {
        _linkTypes = new HashMap<>();

        if(createLinkTypeArrayFromFiles()) {

            log.debug("Non-ordered list:");
            for (int i = 0; i < _linkTypeBuilderArray.length; i++) {
                log.debug("Id: " + _linkTypeBuilderArray[i].getId());
                log.debug("Description: " + _linkTypeBuilderArray[i].getInfo().getDescription());
                log.debug("Version: " + _linkTypeBuilderArray[i].getInfo().getVersion());
                log.debug("Mux: " + _linkTypeBuilderArray[i].getValues().getMuxScheme());
                log.debug("BW: " + _linkTypeBuilderArray[i].getValues().getThroughput());
                log.debug("Lat: " + _linkTypeBuilderArray[i].getValues().getLatency());
                log.debug("Freq: " + _linkTypeBuilderArray[i].getValues().getFrequency());
            }

            Arrays.sort(_linkTypeBuilderArray, new Comparator<LinkTypeOrBuilder>() {

                @Override
                public int compare(final LinkTypeOrBuilder l1, final LinkTypeOrBuilder l2) {
                    if (l1.getValues().getThroughput() > l2.getValues().getThroughput()) {
                        return 1;
                    }
                    else {
                        if (l1.getValues().getThroughput() < l2.getValues().getThroughput()) {
                            return -1;
                        }
                    }
                    return 0;
                }

            });

            log.debug("BW-ordered list:");
            for (int i = 0; i < _linkTypeBuilderArray.length; i++) {
                log.debug("Id: " + _linkTypeBuilderArray[i].getId());
                log.debug("BW: " + _linkTypeBuilderArray[i].getValues().getThroughput());
            }

            setElectionAlgorithmThroughputParameters();

            Arrays.sort(_linkTypeBuilderArray, new Comparator<LinkTypeOrBuilder>() {

                @Override
                public int compare(final LinkTypeOrBuilder l1, final LinkTypeOrBuilder l2) {
                    if (l1.getValues().getLatency() > l2.getValues().getLatency()) {
                        return 1;
                    }
                    else {
                        if (l1.getValues().getLatency() < l2.getValues().getLatency()) {
                            return -1;
                        }
                    }
                    return 0;
                }

            });

            log.debug("Lat-ordered list:");
            for (int i = 0; i < _linkTypeBuilderArray.length; i++) {
                log.debug("Id: " + _linkTypeBuilderArray[i].getId());
                log.debug("Lat: " + _linkTypeBuilderArray[i].getValues().getLatency());
            }

            setElectionAlgorithmLatencyParameters();

            LinkType linkType = null;

            for (int i = 0; i < _linkTypeBuilderArray.length; i++) {
                linkType = _linkTypeBuilderArray[i].build();
                if(linkType != null) {
                    _linkTypes.put(linkType.getId(), linkType);
                }
            }

            for(String id : _linkTypes.keySet()){
                log.debug("" + _linkTypes.get(id));
            }

            return true;
        }

        return false;
    }


    /**
     * Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Creates a Builder for a link type from a json file (to extend with the parameters for the election algorithm)
     * @param file the name of the json file
     * @return the LinkType to build
     */
    private static LinkType.Builder readLinkTypeFromJsonFile(String file)
    {
        BufferedReader br = null;
        try {
            FileReader fr = new FileReader(_jsonDirectory + file);
            br = new BufferedReader(fr);
        }
        catch(Exception e) {
            log.error("The file doesn't exist.");
        }
        String output = "";
        String newline = "";
        try {
            newline = br.readLine();
            while(newline != null) {
                output = output + newline;
                newline = br.readLine();
            }
        }
        catch (IOException e) {
            log.debug("End of file");
        }

        if(!output.equals("")) {
            try {
                LinkType.Builder lt = LinkType.newBuilder();
                JsonFormat.parser().merge(output, lt);
                return lt;
            }
            catch (InvalidProtocolBufferException e) {
                log.error("Error during the parsing from Json...");
                e.printStackTrace();
            }
        }
        else {
            log.error("Error reading json file.");
        }

        return null;
    }

    /**
     * Sets the accuracy for the election algorithm
     * @param accuracy
     */
    public static void setAccuracy(int accuracy)
    {
        _accuracy = accuracy;
    }

    /**
     * Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Sets the latency parameters for the election algorithm on every link type object
     */
    private static void setElectionAlgorithmLatencyParameters()
    {
        int previousLat = 0, actualLat, nextLat = 0;
        LinkLatencyBounds linkAlgorithmLatency;
        int upperDifferenceCoef, lowerDifferenceCoef;
        int linkTypesCount = _linkTypeBuilderArray.length * _accuracy;

        if(_linkTypeBuilderArray.length > 0) {
            nextLat = _linkTypeBuilderArray[0].getValues().getLatency();
        }

        for(int i = 0; i < _linkTypeBuilderArray.length; i++){
            actualLat = nextLat;
            if((i+1) == _linkTypeBuilderArray.length) {
                nextLat = actualLat * 100;
            }
            else {
                nextLat = _linkTypeBuilderArray[i+1].getValues().getLatency();
            }
            lowerDifferenceCoef = (actualLat - previousLat)/linkTypesCount;
            upperDifferenceCoef = (nextLat - actualLat)/linkTypesCount;
            linkAlgorithmLatency = LinkLatencyBounds.newBuilder().setLowerLatency(previousLat)
                    .setUpperLatency(nextLat).setLowerLatencyStep(lowerDifferenceCoef)
                    .setUpperLatencyStep(upperDifferenceCoef).build();
            _linkTypeBuilderArray[i].getBoundsBuilder().setLinkLatencyBounds(linkAlgorithmLatency);
            previousLat = actualLat;
        }
    }

    /**
     * Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Sets the throughput parameters for the election algorithm on every link type object
     */
    private static void setElectionAlgorithmThroughputParameters()
    {
        double previousBw = 0, actualBw;
        LinkThroughputBounds linkAlgorithmThroughput;
        double differenceCoef;
        int linkTypesCount = _linkTypeBuilderArray.length * _accuracy;

        for(int i = 0; i < _linkTypeBuilderArray.length; i++) {
            actualBw = _linkTypeBuilderArray[i].getValues().getThroughput();
            differenceCoef = (actualBw - previousBw)/linkTypesCount;
            linkAlgorithmThroughput = LinkThroughputBounds.newBuilder().setLowerThroughput(previousBw)
                    .setThroughputStep(differenceCoef).build();
            _linkTypeBuilderArray[i].getBoundsBuilder().setLinkThroughputBounds(linkAlgorithmThroughput);
            previousBw = actualBw;
        }
    }

    /**
     *  Author Emanuele Tagliaferro on 10/28/2016.
     *
     * Sets the json directory of the link type files
     * @param jsonDirectory
     */
    public static void setJsonDirectory(String jsonDirectory)
    {
        _jsonDirectory = jsonDirectory;
    }

    private static LinkType.Builder[] _linkTypeBuilderArray;
    private static Map<String, LinkType> _linkTypes;
    private static int _accuracy;
    private static String _jsonDirectory;

    private static final Logger log = LoggerFactory.getLogger(LinkTypeContainer.class);

}
