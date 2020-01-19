import socket
from server_config import READ_TIMEOUT

class ClientSocket(object):
    def __init__(self, connection:socket.socket, address):
        self.connection = connection
        self.address = address

    def readline(self):
        # TODO: handle long message
        bufferSize = 4096
        self.connection.settimeout(READ_TIMEOUT)
        return self.connection.recv(bufferSize).decode()

    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)

    def sendall(self, *args, **kwargs):
        return self.connection.sendall(*args, **kwargs)

    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)