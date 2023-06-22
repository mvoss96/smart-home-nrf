from DBManager import DBManager
from DeviceManager import DeviceManager
from CommunicationManager import CommunicationManager
from WebServerManager import WebServerManager
import threading
import logging

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class SmartHome:
    def __init__(self):
        # Initialize the Managers
        self.db_manager = DBManager()
        self.device_manager = DeviceManager(self.db_manager)
        self.communication_manager = CommunicationManager(self.device_manager)
        self.webserver_manager = WebServerManager(self.db_manager, self.communication_manager)
        self.db_manager.set_http_password("test")

    def start(self):
        """
        Starts listening for incoming messages and periodically sends a test message.
        """
        webserver_thread = threading.Thread(target=self.webserver_manager.run)
        webserver_thread.start()
        while True:
            self.communication_manager.listen()
            self.communication_manager.poll_all_devices()

if __name__ == "__main__":
    home = SmartHome()
    home.start()
