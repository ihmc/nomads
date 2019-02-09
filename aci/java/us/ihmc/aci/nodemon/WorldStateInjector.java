package us.ihmc.aci.nodemon;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * WorldStateInjector.java
 * <p/>
 * Class <code>WorldStateInjector</code> manages loading of extra information about the topology from file.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class WorldStateInjector implements Runnable, Controller
{
    public WorldStateInjector (NodeMon nodeMon)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _isRunning = new AtomicBoolean(false);
        GROUPS_CONFIG_ENABLE = Config.getBooleanValue(
                Conf.NodeMon.GROUPS_CONFIG_ENABLE,
                DefaultValues.NodeMon.GROUPS_CONFIG_ENABLE);
        TOPOLOGY_CONFIG_ENABLE = Config.getBooleanValue(
                Conf.NodeMon.TOPOLOGY_CONFIG_ENABLE,
                DefaultValues.NodeMon.TOPOLOGY_CONFIG_ENABLE);
        TOPOLOGY_CONFIG_DIR = Config.getStringValue(
                Conf.NodeMon.TOPOLOGY_CONFIG_DIR,
                DefaultValues.NodeMon.TOPOLOGY_CONFIG_DIR);
        GROUPS_CONFIG_DIR = Config.getStringValue(
                Conf.NodeMon.GROUPS_CONFIG_DIR,
                DefaultValues.NodeMon.GROUPS_CONFIG_DIR);
        WORLDSTATE_PRELOAD_ENABLE = Config.getBooleanValue(
                Conf.NodeMon.WORLDSTATE_PRELOAD_ENABLE,
                DefaultValues.NodeMon.WORLDSTATE_PRELOAD_ENABLE);
        WORLDSTATE_PRELOAD_DIR = Config.getStringValue(
                Conf.NodeMon.WORLDSTATE_PRELOAD_DIR,
                DefaultValues.NodeMon.WORLDSTATE_PRELOAD_DIR);
    }

    @Override
    public void run ()
    {
        _isRunning.set(true);

        if (TOPOLOGY_CONFIG_ENABLE) {

            List<File> jsonFiles;
            jsonFiles = listFilesForFolder(new File(TOPOLOGY_CONFIG_DIR));
            for (File json : jsonFiles) {
                try {
                    TimeUnit.MILLISECONDS.sleep(100);
                    Topology topology = parseTopology(json);
                    Container c = Container.newBuilder()
                            .setDataType(DataType.TOPOLOGY)
                            .setDataNodeId(_nodeMon.getWorldState().getLocalNodeId())
                            .setTopology(topology)
                            .build();
                    if (topology != null) {
                        _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
                    }
                }
                catch (InterruptedException | InvalidProtocolBufferException e) {
                    log.warn("Read invalid JSON: ", e);
                }
            }
        }

        if (GROUPS_CONFIG_ENABLE) {

            List<File> jsonFiles;
            jsonFiles = listFilesForFolder(new File(GROUPS_CONFIG_DIR));


            for (File json : jsonFiles) {
                log.debug("Parsing file: " + json.getAbsolutePath());
                try {
                    TimeUnit.MILLISECONDS.sleep(100);
                    Group group = parseGroup(json);
                    Container c = Container.newBuilder()
                            .setDataType(DataType.GROUP)
                            .setDataNodeId(_nodeMon.getWorldState().getLocalNodeId())
                            .addGroups(group)
                            .build();

                    if (group != null) {
                        log.debug("Updating GROUP for local node: " + _nodeMon.getWorldState().getLocalNodeId());
                        log.debug("Inject GROUP: " + JsonFormat.printer().print(group));
                        _nodeMon.updateData(_nodeMon.getWorldState().getLocalNodeId(), c);
                    }
                }
                catch (Exception e) {
                    log.warn("Read invalid JSON: ", e);
                }
            }
        }

        if (WORLDSTATE_PRELOAD_ENABLE) {
            List<File> jsonFiles;
            jsonFiles = listFilesForFolder(new File(WORLDSTATE_PRELOAD_DIR));

            for (File json : jsonFiles) {
                try {
                    TimeUnit.MILLISECONDS.sleep(500);
                    Node node = parseNode(json);
                    if (node != null) {
                        Container c = Container.newBuilder()
                                .setDataType(DataType.NODE)
                                .setDataNodeId(_nodeMon.getWorldState().getLocalNodeId())
                                .setNode(node)
                                .build();

                        _nodeMon.updateData(node.getId(), c);
                        _nodeMon.getWorldState().updateClients(node.getId(), c);
                    }

                }
                catch (InterruptedException | InvalidProtocolBufferException e) {
                    log.warn("Read invalid JSON: ", e);
                }

            }
        }


    }

    private Node parseNode (File json) throws InvalidProtocolBufferException
    {
        Objects.requireNonNull(json, "JSON file can't be null");
        byte[] buf = Utils.readFile(json.getAbsolutePath());
        if (buf == null) {
            log.warn("Unable to read file: " + json.getAbsolutePath());
            return null;
        }

        String jsonString = new String(buf);
        ReadableNode.Builder nodeBuilder = ReadableNode.newBuilder();
        JsonFormat.parser().merge(jsonString, nodeBuilder);

        return ProtoUtils.toNode(nodeBuilder.build());
    }

    private Topology parseTopology (File json) throws InvalidProtocolBufferException
    {
        Objects.requireNonNull(json, "JSON file can't be null");
        byte[] buf = Utils.readFile(json.getAbsolutePath());
        if (buf == null) {
            log.warn("topology.json config file not found, no configuration set");
            return null;
        }

        String jsonString = new String(buf);
        ReadableTopology.Builder topologyBuilder = ReadableTopology.newBuilder();
        JsonFormat.parser().merge(jsonString, topologyBuilder);

        return ProtoUtils.toTraffic(topologyBuilder.build());
    }

    private Group parseGroup (File json) throws InvalidProtocolBufferException
    {
        Objects.requireNonNull(json, "JSON file can't be null");
        byte[] buf = Utils.readFile(json.getAbsolutePath());
        if (buf == null) {
            log.warn("group.json config file not found, no configuration set");
            return null;
        }

        String jsonString = new String(buf);
        Group.Builder groupBuilder = Group.newBuilder();
        JsonFormat.parser().merge(jsonString, groupBuilder);

        return groupBuilder.build();
    }

    public List<File> listFilesForFolder (final File folder)
    {
        if (folder == null) {
            return null;
        }
        List<File> files = new ArrayList<>();
        for (final File fileEntry : folder.listFiles()) {
            if (fileEntry.isDirectory()) {
                listFilesForFolder(fileEntry);
            }
            else {
                files.add(fileEntry);
            }
        }

        return files;
    }


    @Override
    public void start ()
    {
        (new Thread(this, "WorldStateInjector")).start();
    }

    @Override
    public boolean isRunning ()
    {
        return _isRunning.get();
    }

    @Override
    public void stop ()
    {
        _isRunning.set(false);
    }


    private final boolean WORLDSTATE_PRELOAD_ENABLE;
    private final boolean TOPOLOGY_CONFIG_ENABLE;
    private final boolean GROUPS_CONFIG_ENABLE;
    private final String GROUPS_CONFIG_DIR;
    private final String TOPOLOGY_CONFIG_DIR;
    private final String WORLDSTATE_PRELOAD_DIR;
    private final NodeMon _nodeMon;
    private final AtomicBoolean _isRunning;
    private final Logger log = Logger.getLogger(WorldStateInjector.class);

}
