# TODO, get this working with plotjuggler.

import cereal.messaging as messaging
from cereal.services import service_list

topics = list(service_list.keys())
topics.remove('roadCameraState')
sm = messaging.SubMaster(topics)
pub = messaging.pub_sock("temp")

while True:
    sm.update()
    for topic in sm.updated:
        if sm.updated[topic]:
            print(sm.data[topic])
            pub.send(sm.data[topic].as_builder().to_bytes())
