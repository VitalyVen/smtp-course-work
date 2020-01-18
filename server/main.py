import socket

from mail_server import MailServer
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR

if __name__ == '__main__':
    with MailServer(port=SERVER_PORT) as server:
        server.serve()



