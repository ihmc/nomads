package us.ihmc.aci.test;

import us.ihmc.aci.agserve.ACIAgServeAdminClient;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * Test class for ACIAgServeClient
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class ACIAgServeClientTest
{
    /**
     * Reads the acr file, deploys and activates the service and, finally calls the method <code>getPrimeNumbers</code>
     * and <code>getPrimeNumbersUpTo</code>
     */
    public ACIAgServeClientTest (String ip, String nodeUUID, int n)
    {
        if ((ip == null) || (nodeUUID == null)) {
            System.err.println ("Either the ip or the node UUID have not been set");
            return;
        }

        try {
            ACIAgServeAdminClient client = new ACIAgServeAdminClient (ip);
            String serviceName = "us.ihmc.primenumbers.PrimeNumbersService";
            String acrFile = "data/PrimeNumbers/service/PrimeNumbersService.acr";
            client.deployServiceRemotely (nodeUUID, new File (acrFile).toURI().toURL());  // The deployService does also the registration

            String serviceInstanceUUID = client.activateServiceRemotely (nodeUUID, serviceName);
            System.out.println ("[ACIAgServeClientTest] serviceInstanceUUID: " + serviceInstanceUUID);
            String instanceUUID = serviceInstanceUUID.replace ("acil://", "").replace ("|" + nodeUUID + "|", "");
            System.out.println ("[ACIAgServeClientTest] instanceUUID: " + instanceUUID);

            List<Integer> results = (List<Integer>) client.invokeService (instanceUUID, "getPrimeNumbers", new Object[0], false);
            System.out.print ("[getPrimeNumbers] PRIME NUMBERS BEFORE 100: ");
            for (Integer i : results) {
                System.out.print (i);
                System.out.print (" ");
            }
            System.out.println("\n");

            Object[] params = new Object[1];
            params[0] = n;
            results = (List<Integer>) client.invokeService (instanceUUID, "getPrimeNumbersUpTo", params, false);
            System.out.print ("[getPrimeNumbersUpTo(" + params[0] + ") PRIME NUMBERS BEFORE " + params[0] + ": ");
            for (Integer i : results) {
                System.out.print (i);
                System.out.print (" ");
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main (String[] args)
    {
        String ip = null;
        String node = null;
        int n = 100;

        int i=0;
        while (true) {
            if (args.length <= i) {
                break;
            }

            if (args[i].equals ("-ip")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                ip = args[i];
            }
            else if (args[i].equals ("-node")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                node = args[i];
            }
            else if (args[i].equals ("-n")) {
                i++;
                if (args.length <= i) {
                    break;
                }
                try {
                    n = Integer.parseInt (args[i]);
                }
                catch (NumberFormatException e) {
                    System.err.println ("The value assigned to n is not correct. Using default value.");
                }
            }
            else {
                System.err.println ("Wrong parameter " + args[i] + ". Ignoring it");
            }

            i++;
        }

        if ((ip == null) || (node == null)) {
            System.err.println ("Either the ip or the node UUID have not been set");
            return;
        }

        new ACIAgServeClientTest (ip, node, n);
    }
}
