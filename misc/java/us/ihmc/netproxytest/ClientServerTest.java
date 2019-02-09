package us.ihmc.netproxytest;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * ClientServerTest.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class ClientServerTest {
    static class Client {
        CommHelper _commHelper;
        private boolean _isLetterMode;

        Client(String host, int port, boolean isLetterMode) {
            _isLetterMode = isLetterMode;
            Socket s = null;
            try {
                s = new Socket(host, port);
            } catch (IOException e) {
                System.out.println("Unable to initialize Socket");
                e.printStackTrace();
            }

            _commHelper = new CommHelper(s);
        }

        public void send() {
            int i = 1;

            while (true) {
                String pattern = buildPattern(i, _isLetterMode);
                try {
                    _commHelper.sendLine(pattern);
                } catch (CommException e) {
                    System.out.println("CommException on sendLine");
                    e.printStackTrace();
                    return;
                }

                System.out.println("Sent 1000 times " + (_isLetterMode ? "A B C D E ...(whole alphabet)" : i));
                i++;
            }
        }
    }

    static class Server {
        CommHelper _commHelper;
        ServerSocket _serverSocket;
        private boolean _isLetterMode;

        Server(int port, boolean isLetterMode) {
            _isLetterMode = isLetterMode;
            try {
                _serverSocket = new ServerSocket(port);
                Socket socket = _serverSocket.accept();
                socket.setTcpNoDelay(true);
                _commHelper = new CommHelper(socket);
            } catch (IOException e) {
                System.out.println("Unable to initialize ServerSocket");
                e.printStackTrace();
            }
        }

        public void listen() {
            int i = 1;
            while (true) {
                try {
                    String pattern = buildPattern(i, _isLetterMode);
                    _commHelper.receiveMatch(pattern);
                    System.out.println("Received 1000 times " +  (_isLetterMode ? "A B C D E ...(whole alphabet)" : i));
                } catch (CommException e) {
                    System.out.println("CommException on receiveMatch");
                    e.printStackTrace();
                    return;
                } catch (ProtocolException e) {
                    System.out.println("Protocol exception, expected " + i);
                    e.printStackTrace();
                    return;
                }
                i++;
            }
        }


    }

    private static String buildPattern(int i, boolean isLetterMode) {

        StringBuilder sb = new StringBuilder();

        for (int j = 0; j < 1000; j++) {

            if (isLetterMode) {
                if (j < ALPHABET.length) {
                    sb.append(ALPHABET[j]);
                }
                else {
                    sb.append(ALPHABET[j % ALPHABET.length]);
                }
                sb.append(" ");

            } else {
                sb.append(i);
                sb.append(" ");
            }

        }

        return sb.toString();
    }


    public static void main(String[] args) {

        printUsage();

        String host;
        int port;
        boolean isLetterMode;

        if (args[1].equals("-l")) {
            isLetterMode = true;
        } else {
            isLetterMode = false;
        }

        if (args[0].equals("-s")) {
            port = Integer.valueOf(args[2]);
            Server s = new Server(port, isLetterMode);
            s.listen();
        } else if (args[0].equals("-c")) {
            host = args[2];
            port = Integer.valueOf(args[3]);
            Client c = new Client(host, port, isLetterMode);
            c.send();
        }
    }

    private final static String[] ALPHABET = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
            "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

    private static void printUsage() {
        System.out.println("Usage: ClientServerTest [-c/-s] [-l/-n] [host] [port] (only port for server)");
    }
}
