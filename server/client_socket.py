import socket
from server_config import READ_TIMEOUT

class ClientSocket(object):
    def __init__(self, connection, address):
        self.connection = connection
        self.address = address

    def readbytes(self):
        # TODO: handle long message
        bufferSize = 4096
        self.connection.settimeout(READ_TIMEOUT)
        return self.connection.recv(bufferSize).decode()

    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)

    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)