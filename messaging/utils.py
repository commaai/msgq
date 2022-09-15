import os

ZMQ_PROTOCOLS = {"SHARED_MEMORY": "inproc://",
                 "INTER_PROCESS": "ipc://@",
                 "TCP": "tcp://"}

def get_zmq_protocol():
    default_zmq_protocol = "tcp://";
    force_protocol = os.getenv("ZMQ_MESSAGING_PROTOCOL");
    if force_protocol is not None:
        default_zmq_protocol = ZMQ_PROTOCOLS[force_protocol]
    return default_zmq_protocol
  

def get_address():
    address = "127.0.0.1";
    default_address = os.getenv("ZMQ_MESSAGING_ADDRESS");
    if default_address is not None:
        address = default_address
    return address;

def get_zmq_socket_path(endpoint):
    return get_zmq_protocol() + get_address() + ":" + endpoint
    
