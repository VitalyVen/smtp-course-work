import socket
from server_config import READ_TIMEOUT

class ClientSocket(object):
    def __init__(self, connection, address):
        self.connection = connection
        self.address = address

    def readline(self):
        # TODO: handle long message
        bufferSize = 4096
        
        # add timeout to the connection if no commands are recieved
        self.connection.settimeout(READ_TIMEOUT)
        # buffer = self.connection.recv(bufferSize).decode()
        # # print(buffer)
        # # remove timeout if commands are recieved
        # self.connection.settimeout(None)
        # buffering = True
        # while buffering:
        #     # prevent empty lines from being processed
        #     if buffer == "\r\n":
        #         self.connection.send("500 5.5.2 Error: bad syntax \n".encode())
        #     if "\r\n" in buffer:
        #         (line, buffer) = buffer.split("\n", 1)
        #         return line
        #     elif "\r\n.\r\n" in buffer:
        #         return buffer
        #     else:
        #         more = self.connection.recv(bufferSize).decode()
        #         if not more:
        #             buffering = False
        #         else:
        #             buffer += more

        return self.connection.recv(bufferSize).decode()

    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)

    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)