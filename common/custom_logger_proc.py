import logging
import multiprocessing
from time import sleep

# ============================================================================
# Define Log Handler
# ============================================================================

class QueueProcessLogger(multiprocessing.Process):
    """multiprocessing log handler

    This handler makes it possible for several processes
    to log to the same file by using a queue.

    """
    def __init__(self, filename,*args, **kwargs):
        self.filename = filename
        super(QueueProcessLogger, self).__init__()
        self.queue = multiprocessing.Queue()
        self.active = True
        self.start()

    def terminate(self):
        self.active = False
    def log(self, level, msg):
        self.queue.put_nowait('{}:{}'.format(level, msg))
    def run(self):
        logger = logging.getLogger()
        handler = logging.FileHandler(filename=self.filename)
        formatter = logging.Formatter(
            '%(asctime)s - LOGLEVEL:%(levelname)s  MESSAGE:%(message)s')
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        while self.active:
            try:
                record = self.queue.get()
                if record is None:
                    break
                level = int(record.split(':')[0])
                message = ':'.join(record.split(':')[1:])
                logger.setLevel(logging.DEBUG)
                logger.log(level, message)
            except (KeyboardInterrupt, SystemExit) as e:
                self.terminate()
            except Exception:
                import sys, traceback
                traceback.print_exc(file=sys.stderr)
        # self.queue.close()

if __name__ == '__main__':
    logger=QueueProcessLogger(filename='../logs/process_logging.log')
    logger.log(logging.INFO,'123')

    while True:
        logger.log(logging.INFO, '123')
        sleep(2)
