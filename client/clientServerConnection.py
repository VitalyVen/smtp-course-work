from SMTP_CLIENT_FSM import *
from client_socket import ClientSocket
from common.mail import Mail

class ClientServerConnection():
    # соединение с сервером с точки зрения клиента:
    # представлено клиентским сокетом, открытым на сервере для данного клиента и
    # машиной состояний клиента при общении с сервером (:)

    def __init__(self, socket_of_client_type: ClientSocket, mail: Mail, logdir: str = 'logs'):
        self.socket = socket_of_client_type
        self.machine = SmtpClientFsm(socket_of_client_type.address(), logdir=logdir)
        self.mail = mail  # Mail(to=[])
        '''
        There are two states for data: 
        1 - we wait start data command "DATA", 
        2 - data end command "." or additional data.
        So we use value of self.data_start_already_matched=False
        '''
        self.data_start_already_matched = False
        self.receipientsLeft = len(mail.to)