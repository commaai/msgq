package messaging;

import java.util.HashMap;
import java.util.Map;

public class Protocols {
    public static String SHARED_MEMORY = "SHARED_MEMORY";
    public static String INTER_PROCESS  = "INTER_PROCESS";
    public static String TCP  = "TCP";
    public static final String defaultProtocol = INTER_PROCESS;

    public static String SHM_DIR = getSHMDir();

    public static Map<String, String> interfaces  = new HashMap<String, String>() {{
        put(SHARED_MEMORY, "inproc://");
        put(INTER_PROCESS, "ipc://" + SHM_DIR);
        put(TCP, "tcp://127.0.0.1:");
    }};
    /**
     if MESSAGING_PROTOCOL env variable is set, it would be used.
     **/
    public static String getSocketPath(String name){
        String MESSAGING_PROTOCOL_OVERRIDE = System.getenv("MESSAGING_PROTOCOL");
        String host = MESSAGING_PROTOCOL_OVERRIDE == null ? Protocols.interfaces.get(defaultProtocol) : Protocols.interfaces.get(MESSAGING_PROTOCOL_OVERRIDE);
        return host + name;
    }

    public static String getSHMDir(){
        if (System.getProperty("os.name").startsWith("Windows"))
            return System.getProperty("java.io.tmpdir");
        else
            return "@"; // use abstract sockets on unix.
    }
}