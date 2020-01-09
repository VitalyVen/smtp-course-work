import select

from mail_server import MailServer
from server_config import SERVER_PORT, READ_TIMEOUT,WRITE_TIMEOUT, LOG_FILES_DIR, THREADS_CNT

from server.client import Client
from server.client_socket import ClientSocket

def mailServerStart():
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

def mailServerStart2():
    with MailServer(port=SERVER_PORT, logdir=LOG_FILES_DIR, threads=THREADS_CNT) as serv:
        _EVENT_READ = select.POLLIN
        _EVENT_WRITE = select.POLLOUT
        pollerSocketsForRead = select.poll()
        pollerSocketsForWrite = select.poll()
        pollerSocketsForRead.register(serv.sock,select.POLLIN)
        pollerSocketsForWrite.register(serv.sock, select.POLLOUT)

        while (True):
            fdVsEventRead = pollerSocketsForRead.poll(READ_TIMEOUT)
            fdVsEventWrite=pollerSocketsForWrite.poll(WRITE_TIMEOUT)

            for descriptor, Event in fdVsEventRead:
                if Event==_EVENT_READ:
                    if descriptor==serv.sock.fileno():
                        connection, client_address = serv.sock.accept()
                        client = Client(socket=ClientSocket(connection, client_address), logdir=serv.logdir)
                        serv.clients[connection] = client
                        pollerSocketsForRead.register(connection, select.POLLIN)
                        pollerSocketsForWrite.register(connection, select.POLLOUT)

                    else:
                        socketFromDescriptor= serv.clients.socket(descriptor)
                        serv.handle_client_read(serv.clients[socketFromDescriptor])
                        pass
            for descriptor, Event in fdVsEventWrite:
                if Event == _EVENT_WRITE:
                    socketFromDescriptor = serv.clients.socket(descriptor)
                    serv.handle_client_write(serv.clients[socketFromDescriptor])


                # Do accept() on server socket or read from a client socket



if __name__ == '__main__':
    mailServerStart2()
