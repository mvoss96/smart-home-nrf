import paho.mqtt.client as mqtt
from src.DBManager import DBManager
from src.CommunicationManager import CommunicationManager
from src.DeviceManager import DeviceManager
from src.Logger import setup_logger
from threading import Event
import time
import re
import json

logger = setup_logger()
root_topic = "smart-home-nrf"
topic_pattern = r"smart-home-nrf/devices/(0x[a-fA-F0-9]+)/set/(\w+)"
hs_discovery_topic = "homeassistant"
sensor_types = "temperature, humidity, battery"


def get_unit_by_parameter(parameter: str) -> str:
    if parameter == "temperature":
        return "°C"
    elif parameter == "humidity":
        return "%"
    elif parameter == "battery":
        return "%"
    else:
        return ""


def to_hexstr(uuid) -> str:
    return "0x" + "".join([f"{x:02x}" for x in uuid])


def create_discovery_topic(uuid, parameter_type: str, parameter: str) -> str:
    topic = ""
    if parameter_type == "trigger":
        topic = f"{hs_discovery_topic}/device_automation/{to_hexstr(uuid)}/action_{parameter}/config"
    elif parameter_type in sensor_types:
        topic = f"{hs_discovery_topic}/sensor/{to_hexstr(uuid)}/{parameter_type}/config"
    elif parameter_type == "light":
        topic = f"{hs_discovery_topic}/light/{to_hexstr(uuid)}/config"
    return topic


def create_discovery_payload(class_obj, device, parameter_type, parameter) -> str:
    payload = {}
    uuid = device["uuid"]
    if "status" not in device:
        logger.warning(f"device {uuid} has not status")
        return ""
    
    uuid_str = to_hexstr(uuid)
    mqtt_device = {
        "identifiers": [f"nrfSmart_0x{uuid_str}"],
        "name": f"{class_obj.__name__}-{uuid_str}",
        "model": f"{class_obj.__name__}",
        "manufacturer": "MarcusVoss",
    }

    if parameter_type == "trigger":
        payload = {
            "automation_type": parameter_type,
            "type": "action",
            "subtype": parameter,
            "payload": parameter,
            "topic": f"{root_topic}/devices/{uuid_str}/action",
            "device": mqtt_device,
        }
    elif parameter_type in sensor_types:
        payload = {
            "name": parameter_type,
            "state_topic": f"{root_topic}/devices/{uuid_str}/{parameter}",
            "unit_of_measurement": get_unit_by_parameter(parameter_type),
            "unique_id": f"{parameter_type}-{uuid_str}",
            "device_class": parameter_type,
            "device": mqtt_device,
        }
    elif parameter_type == "light":
        payload = {
            "name": device.get("name"),
            "brightness_scale": 255,
            "brightness_state_topic": f"{root_topic}/devices/{uuid_str}/status/brightness",
            "brightness_command_topic": f"{root_topic}/devices/{uuid_str}/set/brightness",
            "state_topic": f"{root_topic}/devices/{uuid_str}/status/power",
            "payload_on": 1,
            "payload_off": 0,
            "command_topic": f"{root_topic}/devices/{uuid_str}/set/power",
            "unique_id": f"{parameter_type}-{uuid_str}",
            "device_class": parameter_type,
            "device": mqtt_device,
        }
        #Get Number of channels from db
        if device["status"].get("num_channels") == 2:
            payload["color_temp_state_topic"] = f"{root_topic}/devices/{uuid_str}/status/cct_mired"
            payload["color_temp_command_topic"] = f"{root_topic}/devices/{uuid_str}/set/cct_mired"


    return json.dumps(payload) if payload != {} else ""


class MQTTManager:
    def __init__(
        self,
        db_manager: DBManager,
        comm_manager: CommunicationManager,
        shutdown_flag : Event,
        broker_address="localhost",
        port=1883,
        username=None,
        password=None,
    ):
        self.db_manager = db_manager
        self.comm_manager = comm_manager
        self.shutdown_flag = shutdown_flag
        self.client = mqtt.Client()
        if username and password:
            self.client.username_pw_set(username, password)

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(broker_address, port, 60)

        self.has_send_discovery = []

    def on_connect(self, client, userdata, flags, rc):
        logger.info(f"Connected to MQTT Broker with result code {rc}")

        for device in self.db_manager.get_all_devices():
            # Check if the class exists in the supported_devices list
            class_obj = DeviceManager.get_supported_device(device["type"])
            if class_obj is None:
                continue

            for parameter_type, parameter_name in class_obj.mqtt_discovery_paramters:
                discovery_topic = create_discovery_topic(device["uuid"], parameter_type, parameter_name)
                discovery_payload = create_discovery_payload(class_obj, device, parameter_type, parameter_name)
                if discovery_topic and discovery_payload:
                    print(discovery_topic)
                    self.client.publish(discovery_topic, discovery_payload, retain=True)

    def on_message(self, client, userdata, msg):
        logger.info(f"MQTT message: {msg.topic} {msg.payload}")
        match = re.match(topic_pattern, msg.topic)
        if match:
            uuid_str = match.group(1)[2:]  # Remove the '0x' prefix
            uuid = [int(uuid_str[i : i + 2], 16) for i in range(0, len(uuid_str), 2)]  # Convert to list of integers
            parameter = match.group(2)
            device = self.db_manager.search_device_in_db(uuid)
            if device == None:
                return
            new_val = msg.payload

            if isinstance(new_val, bytes):
                if new_val.lower() == b"on":
                    new_val = 1
                elif new_val.lower() == b"off":
                    new_val = 0
                else:
                    try:
                        new_val = int(new_val)
                    except ValueError:
                        pass

            self.comm_manager.set_device_param(uuid, parameter, str(new_val))

    def publish(self, topic: str, value):
        # logger.info(f"publish: {topic} {value}")
        self.client.publish(topic, value, retain=True)

    def run(self):
        self.client.loop_start()
        self.client.subscribe(f"{root_topic}/devices/+/set/#")
        while not self.shutdown_flag.is_set():
            # Handle all status changes
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

                            topic = f"{root_topic}/devices/{to_hexstr(uuid)}/{key}/{sub_key}"
                            self.publish(topic, str(sub_value) if isinstance(sub_value, list) else sub_value)
                    else:
                        self.publish(f"{root_topic}/devices/{to_hexstr(uuid)}/{key}", value)

            # Handle all events
            while entry := self.comm_manager.get_event():
                try:
                    uuid, event = entry
                except ValueError as e:
                    logger.error(f"Unexpected content in queque: {entry}")
                    return
                # logger.info(f"event from {uuid}: {event}")
                topic = f"{root_topic}/devices/{to_hexstr(uuid)}/action"
                self.client.publish(topic, event)

            time.sleep(0.1)

        logger.info("MQTTManager stopped")
        self.stop()

    def stop(self):
        self.client.loop_stop()
