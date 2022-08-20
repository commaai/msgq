package messaging;
//java
import java.io.*;
import java.util.Map;
//yaml
import org.yaml.snakeyaml.Yaml;
import org.yaml.snakeyaml.constructor.Constructor;

public class PortMap{
    public Map<String, Service> services;
    public static final int STARTING_PORT = 8001;
    public static final int RESERVED_PORT = 8022;  // sshd

    public static int newPort(int idx){
        int port = idx + STARTING_PORT;
        return port >= RESERVED_PORT ? port + 1 : port;
    }
    public PortMap load(){
        Yaml yaml = new Yaml(new Constructor(PortMap.class));
        PortMap portmap;
        InputStream inputStream = PortMap.class.getResourceAsStream("/services.yaml");
        portmap = (PortMap) yaml.load(inputStream);
        try {
            inputStream.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        int idx = 0;
        for (String service : portmap.services.keySet()){
            portmap.services.get(service).port = newPort(idx);
            idx += 1;
        }
        return portmap;
    }
}
