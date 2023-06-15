from flask import Flask, jsonify, render_template, send_from_directory, request
from flask_cors import CORS
from threading import Thread
from DBManager import DBManager
import logging
import os

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WebServerManager:
    def __init__(self, db_manager: DBManager):
        # Initialize Flask app
        self.app = Flask(__name__)
        CORS(self.app)  # This will enable CORS for all routes
        #CORS(self.app, origins=["http://localhost:5500"])
        self.db_manager = db_manager
        self.server = None

        # Define routes
        # @self.app.route('/', methods=['GET'])
        # def home():
        #     """
        #     Endpoint to serve the home page.
        #     """
        #     return send_from_directory(os.getcwd(),'Testpage.html')

        @self.app.route('/devices', methods=['GET'])
        def get_devices():
            """
            Endpoint to get all devices.
            """
            try:
                devices = self.db_manager.get_all_devices()
                return jsonify(devices)
            except Exception as e:
                logger.error(f"An error occurred while fetching devices: {e}")
                return jsonify({"error": "An error occurred while fetching devices."}), 500

        @self.app.route('/devices/<int:device_id>', methods=['DELETE'])
        def remove_device(device_id):
            """
            Endpoint to remove a device.
            """
            try:
                device = self.db_manager.search_device_in_db_by_id(device_id)
                if device:
                    self.db_manager.remove_device_from_db(device_id)
                    return jsonify({"message": "Device removed successfully."}), 200
                else:
                    return jsonify({"message": "Device not found."}), 404
            except Exception as e:
                logger.error(f"An error occurred while removing the device: {e}")
                return jsonify({"error": "An error occurred while removing the device."}), 500

    def run(self, host='0.0.0.0', port=5000, debug=False):
        """
        Start the Flask server in a new thread.
        """
        def start_app():
            self.app.run(host=host, port=port, debug=debug)
        
        self.server = Thread(target=start_app)
        self.server.start()

    def stop(self):
        """
        Stop the Flask server.
        """
        if self.server is not None:
            #self.server.terminate()
            self.server = None
