import select

from mail_server import MailServer
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR, THREADS_CNT

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

socket_map = {}
def readwrite(obj, flags):
    try:
        if flags & select.POLLIN:
            obj.handle_read_event()
        if flags & select.POLLOUT:
            obj.handle_write_event()
        if flags & select.POLLPRI:
            obj.handle_expt_event()
        if flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
            obj.handle_close()
    except OSError as e:
            obj.handle_close()
    except Exception:
        raise
    except:
        obj.handle_error()
def poll2(timeout=0.0, map=None):
    # Use the poll() support added to the select module in Python 2.0
    if map is None:
        map = socket_map
    if timeout is not None:
        # timeout is in milliseconds
        timeout = int(timeout*1000)
    pollster = select.poll()
    if map:
        for fd, obj in list(map.items()):
            flags = 0
            if obj.readable():
                flags |= select.POLLIN | select.POLLPRI
            # accepting sockets should not be writable
            if obj.writable() and not obj.accepting:
                flags |= select.POLLOUT
            if flags:
                pollster.register(fd, flags)

        r = pollster.poll(timeout)
        for fd, flags in r:
            obj = map.get(fd)
            if obj is None:
                continue
            readwrite(obj, flags)

def mailServerStart2():
    with MailServer(port=SERVER_PORT, logdir=LOG_FILES_DIR, threads=THREADS_CNT) as serv:

        pollerObject = select.poll()

        pollerObject.register(serv.sock, select.POLLIN)
        pollerObject2 = select.poll()

        pollerObject2.register(serv.sock, select.POLLOUT)
        while (True):

            fdVsEvent = pollerObject.poll(10)



            for descriptor, Event in fdVsEvent:
                print("Got an incoming connection request")
                if descriptor==8:
                    connection, client_address = serv.sock.accept()
                    fd=connection.fileno()
                    client = Client(socket=ClientSocket(connection, client_address,fd), logdir=serv.logdir)
                    serv.clients[connection] = client
                    pollerObject.register(connection, select.POLLIN)
                    print("Start processing")
                else:
                    p= serv.clients.socket(descriptor)
                    serv.handle_client_read(serv.clients[p])
                    pass



                # Do accept() on server socket or read from a client socket



if __name__ == '__main__':
    mailServerStart2()
