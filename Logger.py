import logging
import colorlog

def setup_logger():
    logger = logging.getLogger('')  # Use a fixed name for the logger

    # if the logger has handlers, just return it
    if logger.hasHandlers():
        return logger

    logger.setLevel(logging.INFO)

    # Create a file handler
    file_handler = logging.FileHandler('nrf_smart.log')
    file_handler.setLevel(logging.INFO)

    # Create a console handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)

    # Define log colors
    log_colors = {
        'DEBUG': 'cyan',
        'INFO': 'white',
        'WARNING': 'yellow',
        'ERROR': 'red',
        'CRITICAL': 'red,bg_white',
    }

    # Create a logging format for the console
    console_formatter = colorlog.ColoredFormatter(
        "%(log_color)s%(asctime)s %(levelname)s %(filename)s:%(lineno)d - %(message)s",
        datefmt='%Y-%m-%d %H:%M:%S',  # Removing milliseconds
        log_colors=log_colors
    )

    # Create a logging format for the file
    file_formatter = logging.Formatter(
        "%(asctime)s %(levelname)-6s %(filename)s:%(lineno)d - %(message)s",
        datefmt='%Y-%m-%d %H:%M:%S',  # Removing milliseconds
    )

    # Set the formatter for the handlers
    file_handler.setFormatter(file_formatter)
    console_handler.setFormatter(console_formatter)

    # Add the handlers to the logger
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)

    return logger
