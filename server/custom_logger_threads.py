import sys
import logging
import traceback
import threading
import multiprocessing
from datetime import time, datetime
from logging import FileHandler as FH
from time import sleep

# ============================================================================
# Define Log Handler
# ============================================================================
class CustomLogHandler(logging.Handler):
    """multiprocessing log handler

    This handler makes it possible for several processes
    to log to the same file by using a queue.

    """
    def __init__(self, fname):
        logging.Handler.__init__(self)

        self._handler = FH(fname)
        self.queue = multiprocessing.Queue(-1)
        thrd = threading.Thread(target=self.receive)
        thrd.daemon = True
        thrd.start()

    def setFormatter(self, fmt):
        logging.Handler.setFormatter(self, fmt)
        self._handler.setFormatter(fmt)

    def receive(self):
        while True:
            try:
                record = self.queue.get()
                self._handler.emit(record)
            except (KeyboardInterrupt, SystemExit):
                raise
            except EOFError:
                break
            except:
                traceback.print_exc(file=sys.stderr)

    def send(self,s):
        self.queue.put_nowait(s)

    def _format_record(self, record):
        times=datetime.fromtimestamp(record.created)
        level=record.levelname
        record.msg='{} LOGLEVEL:{} MASSAGE:{}'.format(str(times),level,record.msg)
        return record

    def emit(self, record):
        try:
            self.send(self._format_record(record))
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            self.handleError(record)

    def close(self):
        self._handler.close()
        logging.Handler.close(self)



if __name__ == '__main__':

    logger = logging.getLogger()
    logger.addHandler(CustomLogHandler('logging.log'))
    logger.setLevel(logging.DEBUG)


    logger.log(logging.INFO,'123')
    while True:
        logger.log(logging.INFO, '123')
        sleep(2)
