import logging
import select
import socket
import threading
import re

from time import sleep
from utils import ClientHelper

from client_state import * #GREETING_STATE as GREETING_STATE
# from client_state import EHLO_WRITE_STATE as EHLO_WRITE_STATE
# from client_state import MAIL_FROM_WRITE_STATE as MAIL_FROM_WRITE_STATE
# from client_state import RCPT_TO_WRITE_STATE as RCPT_TO_WRITE_STATE
# from client_state import DATA_WRITE_STATE as DATA_WRITE_STATE
# from client_state import DATA_END_WRITE_STATE as DATA_END_WRITE_STATE
# from client_state import QUIT_WRITE_STATE as QUIT_WRITE_STATE
# from client_state import EHLO_STATE as EHLO_STATE
# from client_state import MAIL_FROM_STATE as MAIL_FROM_STATE
#
# from client_state import GREETING_pattern as GREETING_pattern
# from client_state import EHLO_pattern as EHLO_pattern
# from client_state import EHLO_end_pattern as EHLO_end_pattern
#
# from SMTP_CLIENT_FSM import SmtpClientFsm
from client_socket import ClientSocket
from common.custom_logger_proc import QueueProcessLogger
from common.mail import Mail

mailDirGlobalQueue = Queue()

class ClientServerConnection():
    # соединение с сервером с точки зрения клиента:
    # представлено клиентским сокетом, открытым на сервере для данного клиента и
    # машиной состояний клиента при общении с сервером (:)

    def __init__(self, socket_of_client_type: ClientSocket, mail: Mail, logdir: str = 'logs'):
        self.socket = socket_of_client_type
        self.machine = SmtpClientFsm(socket_of_client_type.address, logdir=logdir)
        self.mail = mail  # Mail(to=[])
        '''
        There are two states for data: 
        1 - we wait start data command "DATA", 
        2 - data end command "." or additional data.
        So we use value of self.data_start_already_matched=False
        '''
        self.data_start_already_matched = False


class MailClient(object):
    def __init__(self, logdir ='logs', threads=5):
        self.threads_cnt = threads
        self.threads = []
        self.logdir = logdir
        self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __enter__(self):
        return self

    def getMailFromMailDir(self):


    def sendMailInAThread(self, blocking=True):
        # это главный метод класса, в нём реализована многопоточность
        # (параллельный вызов WorkingThread() в количестве threads_cnt потоков)
        for i in range(self.threads_cnt):
            th = WorkingThread(self)
            # kill thread gracefully by adding kill() method of a thread to exit() method of MailClient object:
            th.daemon = True
            th.name = 'Working Thread {}'.format(i)
            th.start()
        self.logger.log(level=logging.DEBUG, msg=f'Started {self.threads_cnt} threads')
        while blocking:
            # pass
        # each thread finishes when main process will be finished (exit context manager)
        # so we should do nothing with blocking=True or other actions in main thread (:)
            try:
                # N.B.: можно заменить на select по файловым дескрипторам:
                sleep(1)
            except KeyboardInterrupt as e:
                self.__exit__(type(e), e, e.__traceback__)

    def __exit__(self, exc_type, exc_val, exc_tb):
        for th in self.threads:
            th.terminate()
        for th in self.threads:
            th.join(timeout=2)
        self.logger.terminate()
        self.logger.join(timeout=2)


# def thread_socket(client: MailClient, clientSockets):
#     try:
#         while True:
#             rfds, wfds, errfds = select.select(clientSockets, clientSockets, [], 5)
#             for fds in rfds:
#                 client.handle_client_read(clientSockets[fds])
#             for fds in wfds:
#                 client.handle_client_write(clientSockets[fds])
#     except ValueError:
#         pass


class WorkingThread(threading.Thread):

    def __init__(self, mainClientFromArg: MailClient, *args, **kwargs):
        super(WorkingThread, self).__init__(*args, **kwargs)
        self.active = True
        self.mailClientMain = mainClientFromArg
        self.clientSockets = []

    def handle_talk_to_server_RW(self, clientServerConnection: ClientServerConnection, readFlag):
        '''
         check possible states from clientServerConnection.machine.state
         match exact state with re (regular expression) patterns
         call appropriate handlers for each state
        '''
        if readFlag:
            try:
                line = clientServerConnection.socket.readline()
            except socket.timeout:
                self.logger.log(level=logging.WARNING, msg=f'Timeout on clientServerConnection read')
                self.sock.close()
                return

        # N.B.: состояние FSM-машины здесь привязано к передаваемому в аргументах сокету(:)
        current_state = clientServerConnection.machine.state

        ##N.B.: client write_to_server states:
        # if current_state == EHLO_WRITE_STATE:
        #     clientServerConnection.machine.EHLO_write(clientServerConnection.socket, clientServerConnection.socket.address, clientServerConnection.mail.domain)
        # elif current_state == MAIL_FROM_WRITE_STATE:
        #     clientServerConnection.machine.MAIL_FROM_write(clientServerConnection.socket, clientServerConnection.mail.from_)
        # elif current_state == RCPT_TO_WRITE_STATE:
        #     clientServerConnection.machine.RCPT_TO_write(clientServerConnection.socket, clientServerConnection.mail.to)
        # elif current_state == DATA_WRITE_STATE:
        #     clientServerConnection.machine.DATA_start_write(clientServerConnection.socket)
        # elif current_state == DATA_END_WRITE_STATE:
        #     clientServerConnection.machine.QUIT_write(clientServerConnection.socket, clientServerConnection.mail.file_path)
        # elif current_state == QUIT_WRITE_STATE:
        #     clientServerConnection.machine.FINISH(clientServerConnection.socket)
        #     clientServerConnection.socket.close()
        #     self.clients.pop(clientServerConnection.socket.connection)
        # else:
        #     pass
        #     # print(current_state)
        #     ## clientServerConnection.socket.send(f'Unrecognised state to write something to a server'.encode())
        #     # print('Unrecognised state to write something to a server')


        if current_state == GREETING_STATE:
            GREETING_matched = re.search(GREETING_pattern, line)
            if GREETING_matched:
                clientServerConnection.machine.EHLO()
                return
        elif current_state == EHLO_WRITE_STATE:
            clientServerConnection.machine.EHLO_write(clientServerConnection.socket, clientServerConnection.mail.domain)
            return
        elif current_state == EHLO_STATE:
            EHLO_matched = re.search(EHLO_pattern, line)
            if EHLO_matched:
                clientServerConnection.machine.EHLO_again()
                return
            else:
                EHLO_end_matched = re.search(EHLO_end_pattern, line)
                if EHLO_end_matched:
                    clientServerConnection.machine.MAIL_FROM()
                    return
        elif current_state == MAIL_FROM_WRITE_STATE:
            clientServerConnection.machine.MAIL_FROM_write(clientServerConnection.socket, clientServerConnection.mail.from_)
            return
        elif current_state == MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                ## clifilesInProcessentServerConnection.mail.from_ = MAIL_FROM_matched.group(1)
                clientServerConnection.machine.RCPT_TO(clientServerConnection.socket, clientServerConnection.mail.from_)
                return
        elif current_state == RCPT_TO_WRITE_STATE:
            clientServerConnection.machine.RCPT_TO_write(clientServerConnection.socket, clientServerConnection.mail.to)
            return
        # TODO:15.01.20: get this method done, at last)):
        elif current_state == RCPT_TO_STATE:
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            if RCPT_TO_matched:
                mail_to = RCPT_TO_matched.group(1)
                clientServerConnection.mail.to.append(mail_to)
                clientServerConnection.machine.RCPT_TO(clientServerConnection.socket, mail_to)
                return
        elif current_state == DATA_STATE:
            if clientServerConnection.data_start_already_matched:
                DATA_end_matched = re.search(DATA_end_pattern, line)
                if DATA_end_matched:
                    data = DATA_end_matched.group(1)
                    if data:
                        clientServerConnection.mail.body += data
                    clientServerConnection.machine.DATA_end(clientServerConnection.socket)
                    clientServerConnection.mail.to_file()
                    clientServerConnection.data_start_already_matched = False
                else:  # Additional data case
                    clientServerConnection.mail.body += line
                    clientServerConnection.machine.DATA_additional(clientServerConnection.socket)
            else:
                # check another recepient firstly
                RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
                if RCPT_TO_matched:
                    mail_to = RCPT_TO_matched.group(1)
                    clientServerConnection.mail.to.append(mail_to)
                    clientServerConnection.machine.ANOTHER_RECEPIENT(clientServerConnection.socket, mail_to)
                    return
                # data start secondly
                DATA_start_matched = re.search(DATA_start_pattern, line)
                if DATA_start_matched:
                    data = DATA_start_matched.group(1)
                    if data:
                        clientServerConnection.mail.body += data
                    clientServerConnection.machine.DATA_start(clientServerConnection.socket)
                    clientServerConnection.data_start_already_matched = True
                else:
                    pass  # TODO: incorrect command to message to client

            return
        QUIT_matched = re.search(QUIT_pattern, line)
        if QUIT_matched:
            clientServerConnection.machine.QUIT(clientServerConnection.socket)
            return
        # Transition possible from any states
        RSET_matched = re.search(RSET_pattern, line)
        if RSET_matched:
            clientServerConnection.machine.RSET(clientServerConnection.socket)
        else:
            pass
        # clientServerConnection.socket.send(f'500 Unrecognised command {line}\n'.encode())
        # print('500 Unrecognised command')

    # def handle_to_server_write(self, clientServerConnection: ClientServerConnection):
    #     current_state = clientServerConnection.machine.state
    #     if current_state == EHLO_WRITE_STATE:
    #         clientServerConnection.machine.EHLO_write(clientServerConnection.socket, clientServerConnection.socket.address, clientServerConnection.mail.domain)
    #     elif current_state == MAIL_FROM_WRITE_STATE:
    #         clientServerConnection.machine.MAIL_FROM_write(clientServerConnection.socket, clientServerConnection.mail.from_)
    #     elif current_state == RCPT_TO_WRITE_STATE:
    #         clientServerConnection.machine.RCPT_TO_write(clientServerConnection.socket, clientServerConnection.mail.to)
    #     elif current_state == DATA_WRITE_STATE:
    #         clientServerConnection.machine.DATA_start_write(clientServerConnection.socket)
    #     elif current_state == DATA_END_WRITE_STATE:
    #         clientServerConnection.machine.QUIT_write(clientServerConnection.socket, clientServerConnection.mail.file_path)
    #     elif current_state == QUIT_WRITE_STATE:
    #         clientServerConnection.machine.FINISH(clientServerConnection.socket)
    #         clientServerConnection.socket.close()
    #         self.clients.pop(clientServerConnection.socket.connection)
    #     else:
    #         pass
    #         # print(current_state)
    #         ## clientServerConnection.socket.send(f'Unrecognised state to write something to a server'.encode())
    #         # print('Unrecognised state to write something to a server')

    def run(self):
        while True:

        # получение новых писем из папки maildir:
        filesInProcess = mailDirGlobalQueue.get()

        self.clientSockets = []
        for file in filesInProcess:
            # clientHelper.socket_init(file_.mx_host, file_.mx_port)
            m = Mail(to=[])
            mail = m.from_file(file)
            mx = clientHelper.get_mx(mail.domain)[0]
            if mx == '-1':
                print(mail.domain + ' error')
                continue
            new_socket = clientHelper.socket_init(host=mx)
            self.clientSockets.append([])
            cur_cs_list_index = len(self.clientSockets) - 1
            self.clientSockets[cur_cs_list_index].append(new_socket)
            self.clientSockets[cur_cs_list_index].append(mail)
            # self.connections[socket] = Client(socket,'.', m)

        try:
            while True:
                self.clientSockets.clear()
                rfds, wfds, errfds = select.select(self.clientSockets, self.clientSockets, [], 5)
                for fds in rfds:
                    self.handle_talk_to_server_RW(ClientServerConnection(self.clientSockets[fds][0], self.clientSockets[fds][1]),
                                                  True)
                for fds in wfds:
                    self.handle_talk_to_server_RW(ClientServerConnection(self.clientSockets[fds][0], self.clientSockets[fds][1]),
                                                  False)
        except ValueError:
            pass

    def terminate(self) -> None:
        self.active = False


if __name__ == '__main__':
    with MailClient(threads=1) as mainMailClient:
        th = WorkingThread(self)
        # kill thread gracefully by adding kill() method of a thread to exit() method of MailClient object:
        th.daemon = True
        th.name = 'Working Thread {}'.format(i)
        th.start()
        while True:
            clientHelper = ClientHelper()
            # получение новых писем из папки maildir:
            filesInProcess_fromMain = clientHelper.maildir_handler()
            mailDirGlobalQueue.put(filesInProcess_fromMain)
            # mainMailClient.sendMailInAThread()


    #  with MailServer() as client:
    #     soc = []
    #     context = ssl._create_stdlib_context()
    #     client_sockets = socket.create_connection(("smtp.yandex.ru", 465), 5,
    #                                               None)
    #     client_sockets = context.wrap_socket(client_sockets,
    #                                          server_hostname="smtp.yandex.ru")
    #     client_sockets2 = socket.create_connection(("smtp.yandex.ru", 465), 5,
    #                                                None)
    #     client_sockets2 = context.wrap_socket(client_sockets2,
    #                                           server_hostname="smtp.yandex.ru")
    #     soc.append(client_sockets)
    #     soc.append(client_sockets2)
    #     while True:
    #         rfds, wfds, errfds = select.select(soc, [], [], 100)
    #         if len(rfds) != 0:
    #             for fds in rfds:
    #                 # serv.handle_client(serv.clients[fds])
    #                 print(fds.recv())
    #                 print(fds.send(b'ehlo [127.0.1.1]\r\n'))

    # https://stackoverflow.com/questions/33397024/mail-client-in-python-using-sockets-onlyno-smtplib (:)

    # import client.utils as cu
    #
    # clientHelper = cu.ClientHelper()
    # filesInProcess = clientHelper.maildir_handler()
    #
    # clientSockets = set()
    # for file in filesInProcess:
    #     # clientHelper.socket_init(file_.mx_host, file_.mx_port)
    #     m = Mail(to=[])
    #     mail = m.from_file(file)
    #     mx = clientHelper.get_mx(mail.domain)[0]
    #     if mx == '-1':
    #         print(mail.domain + ' error')
    #         continue
    #     socket = clientHelper.socket_init(mx)
    #     clientSockets.add(socket)
    #     # self.connections[socket] = Client(socket,'.', m)
