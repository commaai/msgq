package messaging;

import java.util.HashMap;
import java.util.Map;

public class Utils {
    public static String SHARED_MEMORY = "SHARED_MEMORY";
    public static String INTER_PROCESS  = "INTER_PROCESS";
    public static String TCP  = "TCP";
    public static final String defaultZMQProtocol = TCP;
    public static String defaultZMQAdderess = "127.0.0.1";

    public static Map<String, String> interfaces  = new HashMap<String, String>() {{
        put(SHARED_MEMORY, "inproc://");
        put(INTER_PROCESS, "ipc://@");
        put(TCP, "tcp://");
    }};

    public static String getZMQAddress(){
        String MESSAGING_ADDRESS_OVERRIDE = System.getenv("ZMQ_MESSAGING_ADDRESS");
        return MESSAGING_ADDRESS_OVERRIDE != null ? MESSAGING_ADDRESS_OVERRIDE : defaultZMQAdderess;
    }

    public static String getZMQProtocol(){
        String MESSAGING_PROTOCOL_OVERRIDE = System.getenv("ZMQ_MESSAGING_PROTOCOL");
        return MESSAGING_PROTOCOL_OVERRIDE != null ? interfaces.get(MESSAGING_PROTOCOL_OVERRIDE) : interfaces.get(defaultZMQProtocol);
    }

    public static String getSocketPath(String endpoint){
        return getZMQProtocol() + getZMQAddress() + ":" + endpoint;
    }

    public static String getSHMDir(){
        if (System.getProperty("os.name").startsWith("Windows"))
            return System.getProperty("java.io.tmpdir");
        else
            return "@"; // use abstract sockets on unix.
    }
}