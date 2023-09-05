import paho.mqtt.client as mqtt
from src.DBManager import DBManager
from src.CommunicationManager import CommunicationManager
from src.Logger import setup_logger
import time
import re

logger = setup_logger()
root_topic = "smart-home-nrf"
topic_pattern = r"smart-home-nrf/devices/(\[[\d, \s]+\])/set/(\w+)"


class MQTTManager:
    def __init__(self, db_manager: DBManager, comm_manager: CommunicationManager, broker_address="localhost", port=1883, username=None, password=None):
        self.db_manager = db_manager
        self.comm_manager = comm_manager
        self.client = mqtt.Client()
        if username and password:
            self.client.username_pw_set(username, password)

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(broker_address, port, 60)

    def on_connect(self, client, userdata, flags, rc):
        logger.info(f"Connected to MQTT Broker with result code {rc}")

    def on_message(self, client, userdata, msg):
        logger.info(f"MQTT message: {msg.topic} {msg.payload}")
        match = re.match(topic_pattern, msg.topic)
        if match:
            uuid = [int(i) for i in re.findall(r'\d+',  match.group(1))]
            parameter = match.group(2)
            device = self.db_manager.search_device_in_db(uuid)
            if device == None:
                return
            new_val = msg.payload
            if isinstance(new_val,bytes):
                new_val = int(new_val)

            self.comm_manager.set_device_param(uuid, parameter, str(new_val))




    def run(self):
        self.client.loop_start()
        self.client.subscribe(f"{root_topic}/devices/+/set/#")
        while True:
            while db_change := self.db_manager.get_changes():
                try:
                    uuid, change = db_change
                except ValueError as e:
                    logger.error(f"Unexpected content in queque: {db_change}")
                    return
                for key, value in change.items():
                    if key == "uuid":
                        continue
                    if isinstance(value, dict):
                        for sub_key, sub_value in value.items():
                            # Rules for specific status keys:
                            if sub_key == "humidity" and isinstance(sub_value, float):
                                sub_value = round(sub_value)

                            topic = f"{root_topic}/devices/{uuid}/{key}/{sub_key}"
                            self.client.publish(topic, str(sub_value) if isinstance(sub_value, list) else sub_value)
                    else:
                        self.client.publish(f"{root_topic}/devices/{uuid}/{key}", value, retain=True)

            time.sleep(1)

    def stop(self):
        self.client.loop_stop()
