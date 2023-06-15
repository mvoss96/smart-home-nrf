from flask import Flask, jsonify, render_template, send_from_directory, request, Response
from flask_cors import CORS
from threading import Thread, Lock
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
        CORS(
            self.app,
            resources={
                r"*": {
                    "origins": "*",
                    "methods": ["GET", "HEAD", "POST", "OPTIONS", "PUT", "PATCH", "DELETE"],
                    "allow_headers": "*",
                }
            },
        )

        # CORS(self.app, origins=["http://localhost:5500"])
        self.db_manager = db_manager
        self.server = None

        # Define routes
        # @self.app.before_request
        # def before_request():
        #     headers = {
        #         "Access-Control-Allow-Origin": "*",
        #         "Access-Control-Allow-Methods": "GET, POST, PUT, DELETE, OPTIONS",
        #         "Access-Control-Allow-Headers": "Content-Type",
        #     }
        #     if request.method == "OPTIONS" or request.method == "options":
        #         return jsonify(headers), 200

        # @self.app.route('/', methods=['GET'])
        # def home():
        #     """
        #     Endpoint to serve the home page.
        #     """
        #     return send_from_directory(os.getcwd(),'Testpage.html')

        @self.app.route("/devices", methods=["GET"])
        def get_devices():
            """
            Endpoint to get all devices.
            """
            try:
                devices = self.db_manager.get_all_devices()
                return jsonify(devices), 200
            except Exception as e:
                logger.error(f"An error occurred while fetching devices: {e}")
                return jsonify({"error": "An error occurred while fetching devices."}), 500

        @self.app.route("/devices/<device_uuid>/name", methods=["PUT"])
        def rename_device(device_uuid: str):
            """
            Endpoint to get rename a device.
            """
            data = request.json
            if data != None:
                new_name = data.get("name")
                try:
                    uuid = [int(x) for x in device_uuid.split("-")]
                except ValueError:
                    return Response(status=400)
                self.db_manager.update_device_name(uuid, new_name)
                return Response()
            else:
                return Response(status=400)

        @self.app.route("/devices/<device_uuid>", methods=["DELETE"])
        def remove_device(device_uuid):
            """
            Endpoint to remove a device.
            """
            try:
                uuid = [int(x) for x in device_uuid.split("-")]
            except ValueError:
                return Response(status=400)
            self.db_manager.remove_device_from_db(uuid)
            return Response()

    def run(self, host="0.0.0.0", port=5000, debug=False):
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
            # self.server.terminate()
            self.server = None
