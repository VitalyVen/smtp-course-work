from SMTP_FSM import *
from client_socket import ClientSocket
from mail import Mail


class Client():
    def __init__(self, socket:ClientSocket):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address)
        self.mail = Mail()