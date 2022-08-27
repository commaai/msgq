package messaging;
//zmq
import org.zeromq.ZMQ;
//java
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
//logging
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ZMQPubHandler{

    public static final ZMQ.Context context = Factory.getContext();
    public static final PortMap portMap = Factory.getPortmap();
    public static Logger logger = LoggerFactory.getLogger(ZMQPubHandler.class);
    public Map<String, ZMQ.Socket> sockets = new HashMap<>();

    public ZMQPubHandler(){
    }

    public boolean createPublishers(List<String> topicList){
        boolean status = true;
        for (String topic : topicList){
            status = status & createPublisher(topic);
        }
        return status;
    }

    public boolean createPublisher(String topic){
        ZMQ.Socket pub;
        int port = portMap.services.get(topic).port;
        pub = context.socket(ZMQ.PUB);
        pub.bind(Utils.getSocketPath(Integer.toString(port)));
        this.sockets.put(topic, pub);
        logger.debug("Publisher created: {}", topic);
        return true;
    }

    public void releasePublishers(List<String> topicList){
        for (String topic : topicList){
            this.sockets.get(topic).close();
        }
    }

    public void releaseAll(){
        for(String topic : this.sockets.keySet()) {
            this.sockets.get(topic).close();
        }
    }

    public void publish(Map<String, byte[]> data){
        for(String topic : data.keySet()) {
            this.sockets.get(topic).send(data.get(topic), 0);
        }
    }

    public void publishBuffer(String topic, ByteBuffer data){
        data.rewind();
        this.sockets.get(topic).sendByteBuffer(data, 0);
    }

    public void publish(String topic, byte[] data){
        this.sockets.get(topic).send(data, 0);
    }
}
