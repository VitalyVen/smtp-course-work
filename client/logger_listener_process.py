import logging
from logging import handlers
from time import sleep


# [N.B.: see https://medium.com/@jonathonbao/python3-logging-with-multiprocessing-f51f460b8778
# and https://docs.python.org/3/howto/logging-cookbook.html
# for further references...](:)
def listener_configurer():
    root = logging.getLogger()

    file_handler = handlers.RotatingFileHandler('logs/log.log', 'a', maxBytes=1000000, backupCount=2)
    # formatter = logging.Formatter('%(asctime)s %(processName)-10s %(name)s %(levelname)-8s %(message)s')
    formatter = logging.Formatter('%(asctime)s - LOGLEVEL:%(levelname)s  MESSAGE:%(message)s')
    file_handler.setFormatter(formatter)

    console_handler = logging.StreamHandler()
    logToConsoleFormatter = logging.Formatter("%(asctime)s [%(threadName)-12.12s] [%(levelname)-5.5s]  %(message)s")
    console_handler.setFormatter(logToConsoleFormatter)

    root.addHandler(file_handler)
    root.addHandler(console_handler)
    root.setLevel(logging.DEBUG)


def listener_process(queue):
    listener_configurer()
    while True:
        while not queue.empty():
            record = queue.get()
            listener_logger = logging.getLogger(record.name)
            listener_logger.handle(record)
        sleep(1)
