import threading
import time

from src.DBManager import DBManager
from src.DeviceManager import DeviceManager
from src.CommunicationManager import CommunicationManager
from src.WebServerManager import WebServerManager
from src.MQTTManager import MQTTManager
from src.Logger import setup_logger


logger = setup_logger()

class SmartHome:
    def __init__(self):
        # Initialize the Managers
        logger.critical("NRF-Smart-Home started")
        self.shutdown_flag = threading.Event()
        self.db_manager = DBManager()
        self.device_manager = DeviceManager(self.db_manager)
        self.device_manager.start()
        self.communication_manager = CommunicationManager(self.device_manager, self.shutdown_flag)
        self.webserver_manager = WebServerManager(self.db_manager, self.communication_manager)
        self.mqtt_manager = MQTTManager(self.db_manager, self.communication_manager)
        self.db_manager.set_http_password("test")

        

    def start_thread_and_catch_exceptions(self, target):
        def wrapper():
            try:
                target()
            except Exception as e:
                logger.exception(e)
                logger.critical("shutting down...")
                self.shutdown_flag.set()
                self.webserver_manager.stop()

        thread = threading.Thread(target=wrapper, name=target.__name__)
        thread.start()


    def start(self):
        self.start_thread_and_catch_exceptions(self.webserver_manager.run)
        self.start_thread_and_catch_exceptions(self.mqtt_manager.run)
        time.sleep(1) # Wait for server
        self.start_thread_and_catch_exceptions(self.communication_manager.listen)
        self.start_thread_and_catch_exceptions(self.communication_manager.update_all_devices)

if __name__ == "__main__":
    home = SmartHome()
    home.start()
