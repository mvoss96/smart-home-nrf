import logging
from logging.handlers import QueueHandler
import colorlog
import json
import queue


class RotatingQueue(queue.Queue):
    def put(self, item, block=True, timeout=None):
        # If the queue is full, remove the oldest item
        if self.full():
            self.get_nowait()
        super(RotatingQueue, self).put(item, block, timeout)


log_queue = RotatingQueue(5000)


# Create a custom JSON Formatter
class JsonFormatter(logging.Formatter):
    def format(self, record):
        message = f"{record.getMessage()} (File: {record.filename}, Line: {record.lineno})"
        log_entry = {
            "timestamp": record.created,
            "severity": record.levelname,
            "message": message,
            # Add other fields as needed
        }
        return json.dumps(log_entry)
    

# Create a custom filter function for ignoring flask logs
class IgnoreFlaskLogs(logging.Filter):
    def filter(self, record):
        return not ('flask' in record.name or 'werkzeug' in record.name)


def setup_logger():
    logger = logging.getLogger("")  # Use a fixed name for the logger

    # if the logger has handlers, just return it
    if logger.hasHandlers():
        return logger

    logger.setLevel(logging.INFO)

    # Create a file handler
    file_handler = logging.FileHandler("nrf_smart.log", mode="w")
    file_handler.setLevel(logging.INFO)
    file_handler.addFilter(IgnoreFlaskLogs())

    # Create a console handler
    console_handler = logging.StreamHandler(None)
    console_handler.setLevel(logging.INFO)

    # Create queue handler
    queue_handler = QueueHandler(log_queue)
    queue_handler.setLevel(logging.WARNING)
    queue_handler.addFilter(IgnoreFlaskLogs())

    # Define log colors
    log_colors = {
        "DEBUG": "cyan",
        "INFO": "white",
        "WARNING": "yellow",
        "ERROR": "red",
        "CRITICAL": "red,bg_white",
    }

    # Create a logging format for the console
    console_formatter = colorlog.ColoredFormatter(
        "%(log_color)s%(asctime)s %(levelname)s [%(threadName)s]%(filename)s:%(lineno)d - %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",  # Removing milliseconds
        log_colors=log_colors,
    )

    # Create a logging format for the file
    file_formatter = logging.Formatter(
        "%(asctime)s %(levelname)-6s [%(threadName)s]%(filename)s:%(lineno)d - %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",  # Removing milliseconds
    )

    # Create a JSON logging format for the web UI
    json_formatter = JsonFormatter()

    # Set the formatter for the handlers
    file_handler.setFormatter(file_formatter)
    console_handler.setFormatter(console_formatter)
    queue_handler.setFormatter(json_formatter)

    # Add the handlers to the logger
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)
    logger.addHandler(queue_handler)

    return logger
