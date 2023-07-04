import threading
import sys

from DBManager import DBManager
from DeviceManager import DeviceManager
from CommunicationManager import CommunicationManager
from WebServerManager import WebServerManager
from Logger import setup_logger

logger = setup_logger()

class SmartHome:
    def __init__(self):
        # Initialize the Managers
        logger.info("NRF-Smart-Home started")
        self.db_manager = DBManager()
        self.device_manager = DeviceManager(self.db_manager)
        self.communication_manager = CommunicationManager(self.device_manager)
        self.webserver_manager = WebServerManager(self.db_manager, self.communication_manager)
        self.db_manager.set_http_password("test")

    def start_thread_and_catch_exceptions(self, target):
        def wrapper():
            try:
                target()
            except Exception as e:
                logger.exception(e)
                sys.exit()
        thread = threading.Thread(target=wrapper)
        thread.start()

    def start(self):
        self.start_thread_and_catch_exceptions(self.webserver_manager.run)
        self.start_thread_and_catch_exceptions(self.communication_manager.listen)
        self.start_thread_and_catch_exceptions(self.communication_manager.update_all_devices)

if __name__ == "__main__":
    home = SmartHome()
    home.start()
