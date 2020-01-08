import select

from mail_server import MailServer
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR, THREADS_CNT

from server.client import Client
from server.client_socket import ClientSocket

if __name__ == '__main__':
    with MailServer(port=SERVER_PORT, logdir=LOG_FILES_DIR, threads=THREADS_CNT) as serv:
        while True:
            client_sockets = serv.clients.sockets()
            rfds, wfds, errfds = select.select([serv.sock] + client_sockets, client_sockets, [], 5)
            for fds in rfds:
                if fds is serv.sock:
                    connection, client_address = fds.accept()
                    client = Client(socket=ClientSocket(connection, client_address), logdir=serv.logdir)
                    serv.clients[connection] = client
                else:
                    serv.handle_client_read(serv.clients[fds])
            for fds in wfds:
                serv.handle_client_write(serv.clients[fds])