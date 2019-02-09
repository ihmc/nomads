package us.ihmc.aci.netSupervisor.algorithms;

import com.google.protobuf.Duration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.ddam.Description;
import us.ihmc.aci.ddam.Link;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.ddam.Stat;
//import us.ihmc.aci.nodemon.WorldState;
/**
 * Created by Roberto on 2/26/2016.
 */
public class SimpleAlgorithm {
    /*
    public static Link GuessConnectionSimpleAlg(Node n1, Node n2, WorldState localWorldState)
    {
//use a config file
        double LAN_BW_H     = 0;
        double LAN_RTT_L    = 10;
        double LAN_RTT_H    = 0;

        double SATCOM_BW_H  = 1000000000;
        double SATCOM_RTT_L = 200;
        double SATCOM_RTT_H  = 0;

        double HF_BW_H      = 56000000;
        double HF_RTT_L     = 10;
        double HF_RTT_H     = 100;

        double throughput;
        int packetLoss;
        int latency;

        double doubleIncoming;
        double doubleOutgoing;

        timeValidity =  Duration.getDefaultInstance();
        timeValidity.newBuilderForType().setSeconds(5);


        String ConnType;
        latency = 0;

        //latency = 110;
        //Type ConnType;
        //Protos.Link newLink;
        //get the Node Monitor World State


        //log.warn ("Manual override");
        //ConnType = Type.SATCOM;
        //throughput  = 98;
        //packetLoss  = 10;
        //latency     = 110;
        //return Protos.Link.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).build();


        Link link = null;
        Description linkDescription = null;
        if((n1.getId() == null) || (n2.getId() == null) || (localWorldState == null)) {
            if(n1.getId() == null) {
                log.warn("n1 point to null?");
            }
            if(n2.getId() == null) {
                log.warn("n2 point to null?");
            }
            if(localWorldState == null) {
                log.warn("localWorldState is null!");
            }
            return null;
        }
        else {
            int tp1 = localWorldState.getIncomingTraffic(n2.getId(),n2.getId(), n1.getId(),timeValidity);
            log.debug("Get traffic node 2 received from node 1: " + tp1);

            int tp2 = localWorldState.getOutgoingTraffic(n1.getId(),n1.getId(), n2.getId(),timeValidity);
            log.debug("Get traffic node 1 sent to node 2: " + tp2);



            if (tp1 != 0) {
                doubleIncoming = tp1;
                log.info(n1.getId() + " in is " + doubleIncoming);
                if (tp2 != 0) {
                    doubleOutgoing = tp2;
                    log.info(n2.getId() + " out is " + doubleOutgoing);
                    throughput = doubleIncoming;
                    packetLoss = (int) ((doubleOutgoing - doubleIncoming) * 100 / doubleOutgoing);
                    log.info("PL between" + n1.getId() + " and " + n2.getId() +  " is " + doubleOutgoing);

                    latency = localWorldState.getLatency(n2.getId(),n2.getId(), n1.getId(),timeValidity);
                    log.info("LATENCY between: " + n1.getId() + " " + n2.getId() + " is " + latency);

                    //log.info("MANUALLY SET LATENCY between " + n1.getId() + " " + n2.getId() + " to " + latency);
                    //latency = 110;

                    if (latency > 0) {
                        // case 1 and 2


			log.info("MANUALLY OVERRIDE");
                        throughput =    5000000;
                        latency    =    40; 


                        int pointLANB = 0;
                        int pointSATB = 0;
                        int pointHFB = 0;

                        int pointLANL = 0;
                        int pointSATL = 0;
                        int pointHFL = 0;

                        pointLANB 	= 	countBW(throughput, 1) + pointLANB;
                        pointSATB 	= 	countBW(throughput, 2) + pointSATB;
                        pointHFB 	= 	countBW(throughput, 3) + pointHFB;

                        log.info("Lan B points: " + 	pointLANB);
                        log.info("Sat B points: " + 	pointSATB);
                        log.info("Hf B points: " + 	pointHFB);


                        pointLANL = countLAT(latency, 1) + pointLANL;
                        pointSATL = countLAT(latency, 2) + pointSATL;
                        pointHFL = countLAT(latency, 3) + pointHFL;

                        log.info("Lan L points: " + pointLANL);
                        log.info("Sat L points: " + pointSATL);
                        log.info("Hf L points: " + pointHFL);

                        log.info("Lan Sum: " + (pointLANB + pointLANL));
                        log.info("Sat Sum: " + (pointSATB + pointSATL));
                        log.info("Hf Sum: " + (pointHFB + pointHFL));



                        if ((throughput >= SATCOM_BW_H) || (latency <= LAN_RTT_L)) {
                            log.info("Guess between " + n1.getName() + " " + n2.getName() + "is LAN ");
                            ConnType = "LAN";
                            linkDescription = Description.newBuilder().setType(ConnType).build();
                            return Link.newBuilder().setDescription(linkDescription).build();
                        } else {
                            //case 3
                            if ((throughput < SATCOM_BW_H) && (throughput >= HF_BW_H)) {
                                if ((latency >= HF_RTT_H) && (latency <= SATCOM_RTT_L)) {
                                    ConnType = "SATCOM";
                                    linkDescription = Description.newBuilder().setType(ConnType).build();
                                    return Link.newBuilder().setDescription(linkDescription).build();
                                } else {
                                    if ((latency <= HF_RTT_H) && (latency > LAN_RTT_L)) {
                                        log.info("Guess between " + n1.getName() + " " + n2.getName() + "is LAN ");
                                        ConnType = "SATCOM";
                                        linkDescription = Description.newBuilder().setType(ConnType).build();
                                        return Link.newBuilder().setDescription(linkDescription).build();
                                    }
                                }
                            }
                            else {
                                if (throughput < HF_BW_H) {
                                    //CASE 4
                                    if (latency > HF_RTT_H) {
                                        log.info("Guess between " + n1.getName() + " " + n2.getName() + "is SATCOM ");
                                        ConnType = "SATCOM";
                                        linkDescription = Description.newBuilder().setType(ConnType).build();
                                        return Link.newBuilder().setDescription(linkDescription).build();
                                    }
                                    //case 5
                                    if ((latency <= HF_RTT_H) && (latency > LAN_RTT_L)) {
                                        log.info("Guess between " + n1.getName() + " " + n2.getName() + "is HF ");
                                        ConnType = "HF";
                                        linkDescription = Description.newBuilder().setType(ConnType).build();
                                        return Link.newBuilder().setDescription(linkDescription).build();
                                    }
                                    log.warn("Shouldn't be here! case 5 ");
                                }
                            }
                        }
                    }
                    else {
                        //latency is missing
                        //case 1
                        if (throughput > SATCOM_BW_H) {
                            log.info("Guess between " + n1.getName() + " " + n2.getName() + "is LAN ");
                            ConnType = "LAN";
                            linkDescription = Description.newBuilder().setType(ConnType).build();
                            return Link.newBuilder().setDescription(linkDescription).build();
                        }
                    }
                }
            }
            else {
                //throughput data is missing
                //case 1
                log.debug("Get ping between node 1 and node 2");
                latency = (int) localWorldState.getLatency(n2.getId(),n2.getId(), n1.getId(),timeValidity);
                log.info("LATENCY between: " + n1.getId() + " " + n2.getId() + " is " + latency);
                if(latency > 0) {

                    if(latency <= LAN_RTT_L) {
                        log.info("Guess between " + n1.getName() + " " + n2.getName() + "is LAN ");
                        ConnType = "LAN";
                        linkDescription = Description.newBuilder().setType(ConnType).build();
                        return Link.newBuilder().setDescription(linkDescription).build();
                    }
                }
            }
        }
        log.warn ("Not enough information to get a match");
        ConnType = "UNKNOWN";
        throughput  = 0;
        packetLoss  = 0;
        latency     = 0;
        linkDescription = Description.newBuilder().setType(ConnType).build();
        return Link.newBuilder().setDescription(linkDescription).build();

    }
*/
    public static int countBW(double bw, int ID) {

        double lanB = 1000000;
        double satB = 1000000;
        double hfB  = 56000;

        if(ID == 1) {
            //LAN
            if(bw >= lanB)
                return 100;
            if(((0.9 * lanB) <= bw) && (bw < lanB))
                return 90;
            if(((0.75 * lanB) <= bw) && (bw < (lanB * 0.9)))
                return 90;
            if(((0.50 * lanB) <= bw) && (bw < (lanB * 0.75)))
                return 80;
            if(((0.25 * lanB) <= bw) && (bw < (lanB * 0.50)))
                return 70;
            if(((0.10 * lanB) <= bw) && (bw < (lanB * 0.25)))
                return 60;
            if(bw < (lanB * 0.10))
                return 40;
        }

        if(ID == 2) {
            //SATCOM
            if(bw >= satB)
                return -20;
            if(((0.9 * satB) <= bw) && (bw < satB))
                return 90;
            if(((0.75 * satB) <= bw) && (bw < (satB * 0.9)))
                return 100;
            if(((0.50 * satB) <= bw) && (bw < (satB * 0.75)))
                return 80;
            if(((0.25 * satB) <= bw) && (bw < (satB * 0.50)))
                return 70;
            if(((0.10 * satB) <= bw) && (bw < (satB * 0.25)))
                return 60;
            if(bw < (satB * 0.10))
                return 40;
        }

        if(ID == 3) {
            //HF
            if(bw >= hfB)
                return -20;
            if(((0.9 * hfB) <= bw) && (bw < hfB))
                return 90;
            if(((0.75 * hfB) <= bw) && (bw < (hfB * 0.9)))
                return 100;
            if(((0.50 * hfB) <= bw) && (bw < (hfB * 0.75)))
                return 100;
            if(((0.25 * hfB) <= bw) && (bw < (hfB * 0.50)))
                return 90;
            if(((0.10 * hfB) <= bw) && (bw < (hfB * 0.25)))
                return 80;
            if(bw < (hfB * 0.10))
                return 80;
        }
        log.warn ("countBW no match?");
        return 0;
    }

    public static int countLAT(int lat, int ID) {

        int lanL = 10;
        int satL = 200;
        int hfL  = 100;

        if(ID == 1) {
            //LAN
            if(((0.8 * lanL) <= lat) && (lat < (1.2 * lanL)))
                return 100;
            if(((1.2 * lanL) <= lat) && (lat <= (2 * lanL)))
                return 100;
            if(lat > (2 * lanL))
                return 50;
	    if(((0.5 * hfL) <= lat) && (lat <= (0.8 * lanL)))
		return 90;
	    if(((0.2 * hfL) <= lat) && (lat <= (0.5 * lanL)))
		return 90;
	    if(((0.1 * lanL) <= lat) && (lat <= (0.2 * lanL)))
		return 90;
            if(lat < (0.1 * lanL))
                return 90;
        }

        if(ID == 2) {
            //SATCOM
            if(((0.8 * satL) <= lat) && (lat < (1.2 * satL)))
                return 100;
            if(((1.2 * satL) <= lat) && (lat <= (2 * satL)))
                return 100;
            if(lat > (2 * hfL))
                return 100;
	    if(((0.5 * satL) <= lat) && (lat <= (0.8 * satL)))
		return 40;
	    if(((0.2 * satL) <= lat) && (lat <= (0.5 * satL)))
		return 30;
	    if(((0.1 * satL) <= lat) && (lat <= (0.2 * satL)))
		return 20;
            if(lat < (0.1 * satL))
                return 10;
        }

        if(ID == 3) {
            //HF
            if(((0.8 * hfL) <= lat) && (lat < (1.2 * hfL)))
                return 100;
            if(((1.2 * hfL) <= lat) && (lat <= (2 * hfL)))
                return 100;
            if(lat > (2 * hfL))
                return 60;
	    if(((0.5 * hfL) <= lat) && (lat <= (0.8 * hfL)))
		return 70;
	    if(((0.2 * hfL) <= lat) && (lat <= (0.5 * hfL)))
		return 40;
	    if(((0.1 * hfL) <= lat) && (lat <= (0.2 * hfL)))
		return 30;
            if(lat < (0.1 * hfL))
                return 20;
        }


        log.warn ("countLAT no match?");
        return 0;
    }

    static Duration timeValidity;
    private static final Logger log = LoggerFactory.getLogger(SimpleAlgorithm.class);
}