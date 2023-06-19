from flask import Flask, jsonify, render_template, send_from_directory, stream_with_context, request, Response
from flask_cors import CORS
from flask_httpauth import HTTPBasicAuth
from werkzeug.security import generate_password_hash, check_password_hash
from threading import Thread
from DBManager import DBManager
import logging
import json
import time
import os

# Configure the root logger
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class WebServerManager:
    def __init__(self, db_manager: DBManager):
        # Initialize Flask app
        self.app = Flask(__name__)
        self.app.config['TEMPLATES_AUTO_RELOAD'] = True
        # Initialize the Authentification
        self.auth = HTTPBasicAuth()
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
        @self.auth.verify_password
        def verify_password(username, password):
            print(f"user:{username} pw:{password}")
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
            try:
                devices = self.db_manager.get_all_devices()
                return jsonify(devices), 200
            except Exception as e:
                logger.error(f"An error occurred while fetching devices: {e}")
                return jsonify({"error": "An error occurred while fetching devices."}), 500

        @self.app.route("/devices/<device_uuid>/name", methods=["PUT"])
        @self.auth.login_required
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
        @self.auth.login_required
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
