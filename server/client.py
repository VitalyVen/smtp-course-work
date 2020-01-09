from SMTP_FSM import *
from client_socket import ClientSocket
from common.mail import Mail


class Client():
    def __init__(self, socket:ClientSocket, logdir: str):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address, logdir=logdir)
        self.mail = Mail()