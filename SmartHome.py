from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage, HostMessage, SetMessage, CHANGE_TYPES, MSG_TYPES, supported_devices
from DBManager import DBManager
from DeviceManager import DeviceManager
from CommunicationManager import CommunicationManager
from WebServerManager import WebServerManager
import threading
import time
import logging

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class SmartHome:
    def __init__(self):
        # Initialize the DBManager, DeviceManager and CommunicationManager
        self.db_manager = DBManager()
        self.device_manager = DeviceManager(self.db_manager)
        self.communication_manager = CommunicationManager(self.device_manager)

        # Create the WebServerManager instance and provide a reference to the db_manager
        self.webserver_manager = WebServerManager(self.db_manager)

    def send_test(self):
        """
        This function sends a test message to the first device in the table every two seconds.
        """
        current_time = time.time()
        if current_time - self.communication_manager.last_update_time < 2.0:
            return
        device = self.device_manager.db_manager.get_all_devices()[0]
        msg = HostMessage(
            uuid=self.db_manager.uuid,
            msg_type=MSG_TYPES.SET,
            data=SetMessage(index=1, changeType=CHANGE_TYPES.SET, value=[32]).get_raw(),
        )
        logger.info(self.device_manager.send_msg_to_device(1, msg.get_raw()))
        self.communication_manager.poll_device(device["uuid"])
        self.communication_manager.last_update_time = current_time

    def start(self):
        """
        Starts listening for incoming messages and periodically sends a test message.
        """
        webserver_thread = threading.Thread(target=self.webserver_manager.run)
        webserver_thread.start()
        while True:
            self.communication_manager.listen()
            self.communication_manager.poll_all_devices()
            #self.send_test()


if __name__ == "__main__":
    home = SmartHome()
    home.start()
