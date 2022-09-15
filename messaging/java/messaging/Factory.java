package messaging;

import org.zeromq.ZMQ;

public class Factory {
    public static final ZMQ.Context context = ZMQ.context(4);
    public static final PortMap portmap = new PortMap().load();
    
    public static ZMQ.Context getContext(){
        return context;
    }
    public static PortMap getPortmap(){
        return portmap;
    }
    
    public static ZMQ.Context getNewContext(){
        return ZMQ.context(4);
    }
}
    