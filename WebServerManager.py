from flask import Flask, jsonify, render_template, request, Response
from flask_cors import CORS
from flask_httpauth import HTTPBasicAuth
from threading import Thread
from DBManager import DBManager
from CommunicationManager import CommunicationManager
import json
import time
from Logger import setup_logger

logger = setup_logger()

class WebServerManager:
    def __init__(self, db_manager: DBManager, comm_manager: CommunicationManager):
        # Initialize Flask app
        self.app = Flask(__name__)
        self.app.config["TEMPLATES_AUTO_RELOAD"] = True
        # Initialize the Authentification
        self.auth = HTTPBasicAuth()
        # Set up CORS if wanted
        # CORS(
        #     self.app,
        #     resources={
        #         r"*": {
        #             "origins": "*",
        #             "methods": ["GET", "HEAD", "POST", "OPTIONS", "PUT", "PATCH", "DELETE"],
        #             "allow_headers": "*",
        #         }
        #     },
        # )
        self.db_manager = db_manager
        self.comm_manager = comm_manager
        self.server = None

        # Define routes
        @self.auth.verify_password
        def verify_password(username, password):
            #print(f"user:{username} pw:{password}")
            return self.db_manager.check_http_password(password)

        @self.app.route("/", methods=["GET"])
        def home():
            """
            Endpoint to serve the home page.
            """
            return render_template("index.html")

        @self.app.route("/stream")
        @self.auth.login_required
        def stream():
            def generate():
                while True:
                    try:
                        devices = self.db_manager.get_all_devices()
                        logger.info(f"Devices fetched: {devices}")
                        yield f"data: {json.dumps(devices)}\n\n"
                    except Exception as e:
                        logger.error(f"An error occurred while fetching devices: {e}")
                        yield f"data: {{'error': 'An error occurred while fetching devices.'}}\n\n"
                    time.sleep(5)

            logger.info("Received a request for /stream endpoint")
            return Response(generate(), mimetype="text/event-stream")

        @self.app.route("/devices", methods=["GET"])
        @self.auth.login_required
        def get_devices():
            """
            Endpoint to get all devices.
            """
            devices = self.fetch_devices()
            if devices is not None:
                return jsonify(devices), 200
            else:
                return jsonify({"error": "An error occurred while fetching devices."}), 500

        @self.app.route("/devices/<device_uuid>/name", methods=["PUT"])
        @self.auth.login_required
        def rename_device(device_uuid: str):
            """
            Endpoint to get rename a device.
            """
            data = request.json
            uuid = self.parse_uuid(device_uuid)
            if uuid is None or data is None:
                return Response(status=400, response="Empty request or unable to parse UUID")
            self.db_manager.update_device_name(uuid, data.get("name"))
            return Response()

        @self.app.route("/devices/<device_uuid>", methods=["DELETE"])
        @self.auth.login_required
        def remove_device(device_uuid):
            """
            Endpoint to remove a device.
            """
            uuid = self.parse_uuid(device_uuid)
            if uuid is None:
                return Response(status=400, response="Unable to parse UUID")
            self.db_manager.remove_device_from_db(uuid)
            return Response()

        @self.app.route("/devices/<device_uuid>", methods=["GET"])
        @self.auth.login_required
        def get_device(device_uuid):
            """
            Endpoint to get the status of a specific device
            """
            uuid = self.parse_uuid(device_uuid)
            if uuid is None:
                return Response(status=400, response="Unable to parse UUID")
            device = self.db_manager.search_device_in_db(uuid)
            if device == None:
                return Response(status=400, response="Device not found")
            return jsonify(device), 200

        @self.app.route("/devices/<device_uuid>/<parameter>", methods=["GET"])
        @self.auth.login_required
        def gt_device_param(device_uuid, parameter):
            """
            Endpoint to get a single parameter of a specific device
            """
            uuid = self.parse_uuid(device_uuid)
            if uuid is None:
                return Response(status=400, response="Unable to parse UUID")
            device = self.db_manager.search_device_in_db(uuid)
            if device == None:
                return Response(status=400, response="Device not found")
            status = device.get("status")
            if status == None:
                return Response(status=400, response="Device does not have a status")
            if parameter == "status":
                return jsonify(status), 200
            return jsonify(status.get(parameter)), 200

        @self.app.route("/devices/<device_uuid>/<parameter>", methods=["PUT"])
        @self.auth.login_required
        def set_device_param(device_uuid, parameter):
            """
            Endpoint to set a single parameter of a specific device
            """
            data = request.json
            uuid = self.parse_uuid(device_uuid)
            if uuid is None or data is None:
                return Response(status=400, response="Empty request or unable to parse UUID")
            new_val = data.get("value")
            print(f"paramter: {parameter} new_val: {new_val}");
            device = self.db_manager.search_device_in_db(uuid)
            if device == None:
                return Response(status=400, response="Device not found")
            status = device.get("status")
            if status == None:
                return Response(status=400, response="Device does not have a status")
            if not self.comm_manager.set_device_param(uuid, parameter, str(new_val)):
                return Response(status=400, response="Unable to parse request")
            return Response()

    def parse_uuid(self, device_uuid):
        try:
            uuid = [int(x) for x in device_uuid.split("-")]
            return uuid
        except ValueError:
            return None

    def fetch_devices(self):
        try:
            devices = self.db_manager.get_all_devices()
            return devices
        except Exception as e:
            logger.error(f"An error occurred while fetching devices: {e}")
            return None

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
