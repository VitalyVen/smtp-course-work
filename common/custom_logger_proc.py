import logging
import multiprocessing
from time import sleep

# ============================================================================
# Define Log Handler
# ============================================================================

class QueueProcessLogger():
    """multiprocessing log handler

    This handler makes it possible for several processes
    to log to the same file by using a queue.

    """
    def __init__(self, filename):
        self.filename = filename
        self.queue = multiprocessing.Queue()
        thrd = multiprocessing.Process(target=self.receive)
        thrd.start()

    def log(self, level, msg):
        self.queue.put_nowait('{}:{}'.format(level, msg))

    def receive(self):
        logger = logging.getLogger()
        handler = logging.FileHandler(filename=self.filename)
        formatter = logging.Formatter(
            '%(asctime)s - LOGLEVEL:%(levelname)s  MESSAGE:%(message)s')
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        while True:
            try:
                record = self.queue.get()
                if record is None:
                    break
                level = int(record.split(':')[0])
                message = ':'.join(record.split(':')[1:])
                logger.setLevel(logging.DEBUG)
                logger.log(level, message)
            except (KeyboardInterrupt, SystemExit) as e:
                raise e
            except Exception:
                import sys, traceback
                traceback.print_exc(file=sys.stderr)



if __name__ == '__main__':
    logger=QueueProcessLogger(filename='../logs/process_logging.log')
    logger.log(logging.INFO,'123')

    while True:
        logger.log(logging.INFO, '123')
        sleep(2)
