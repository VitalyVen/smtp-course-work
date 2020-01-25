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

        # # https://stackoverflow.com/questions/29023885/python-socket-readline-without-socket-makefile
        # self.connection.settimeout(READ_TIMEOUT)
        # CHUNK_SIZE = 16
        # buffer = bytearray()
        # while True:
        #     chunk = self.connection.recv(CHUNK_SIZE) #stream.read(CHUNK_SIZE)
        #     buffer.extend(chunk)
        #     if b'\n' in chunk or not chunk:
        #         break
        # firstline = buffer[:buffer.find(b'\n')]
        # return firstline.decode()



    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)

    def sendall(self, *args, **kwargs):
        return self.connection.sendall(*args, **kwargs)

    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)