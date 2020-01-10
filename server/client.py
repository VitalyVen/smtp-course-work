from SMTP_FSM import *
from client_socket import ClientSocket
from common.mail import Mail


class Client():
    '''
    There are two states for data: 1 - we wait start data command "DATA", 2 - data end command "." or additional data.
    So we use value of self.data_start_already_matched=False
    '''
    def __init__(self, socket:ClientSocket, logdir: str):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address, logdir=logdir)
        self.mail = Mail(to=[])
        self.data_start_already_matched=False