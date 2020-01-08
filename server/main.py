from mail_server import MailServer
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR, THREADS_CNT

if __name__ == '__main__':
    with MailServer(port=SERVER_PORT, logdir=LOG_FILES_DIR, threads=THREADS_CNT) as server:
        server.serve()