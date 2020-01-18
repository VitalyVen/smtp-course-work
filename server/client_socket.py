import socket
from server_config import READ_TIMEOUT

class ClientSocket(object):
    def __init__(self, connection, address):
        self.connection = connection
        self.address = address

    def readline(self):
        # TODO: handle long message
        bufferSize = 4096

        self.connection.settimeout(READ_TIMEOUT)
        p=self.connection.recv(bufferSize).decode()
        # print(p)
        # print('___________________________________')
        print(len(p))
        return p

    def send(self, *args, **kwargs):
        print(*args)
        return self.connection.send(*args, **kwargs)

    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)