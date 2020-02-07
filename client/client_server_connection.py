from SMTP_CLIENT_FSM import *
from client_socket import ClientSocket
from common.mail import Mail

class ClientServerConnection():
    # соединение с сервером с точки зрения клиента:
    # представлено клиентским сокетом, открытым на сервере для данного клиента и
    # машиной состояний клиента при общении с сервером (:)

    def __init__(self, socket_of_client_type: ClientSocket, mail: Mail, logger_from_main_class):  # logdir: str = 'logs'):
        self.socket = ClientSocket(socket_of_client_type, socket_of_client_type.getsockname())
        self.machine = SmtpClientFsm(socket_of_client_type.getsockname(), logger_from_main_class)  #  logdir=logdir)
        self.mail = mail  # Mail(to=[])
        self.receipientsLeft = len(mail.to)
