import logging
from time import sleep
from random import random, randint


logger = logging.getLogger(__name__)  # use module name


def worker_logging_test_process(queue):
    for i in range(3):
        sleep(random())
        logger.info(f'Logging a random number {randint(0, 10)}')
