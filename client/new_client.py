import logging
import select
import socket
import threading
import client.utils as cu
from .SMTP_CLIENT_FSM import SmtpClientFsm
from client_socket import ClientSocket
from common.custom_logger_proc import QueueProcessLogger
from common.mail import Mail


class ClientServerConnection():
    # соединение с сервером с точки зрения клиента:
    # представлено клиентским сокетом, открытым на сервере для данного клиента и
    # машиной состояний клиента при общении с сервером (:)
    '''
    There are two states for data: 1 - we wait start data command "DATA", 2 - data end command "." or additional data.
    So we use value of self.data_start_already_matched=False
    '''

    def __init__(self, socket_of_client_type: ClientSocket, logdir: str ='logs'):
        self.socket = socket_of_client_type
        self.machine = SmtpClientFsm(socket_of_client_type.address, logdir=logdir)
        self.mail = Mail(to=[])
        self.data_start_already_matched = False


class MailClient(object):
    def __init__(self, logdir ='logs', threads=5):
        self.threads_cnt = threads
        self.threads = []
        self.logdir = logdir
        self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __enter__(self):
        return self

    def sendMailInMultipleThreads(self, blocking=True):
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
            pass
        # each thread finishes when main process will be finished (exit context manager)
        # so we should do nothing with blocking=True or other actions in main thread (:)
        #     try:
        #         sleep(0.1)
        #     except KeyboardInterrupt as e:
        #         self.__exit__(type(e), e, e.__traceback__)

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
        self.clientSockets

    def handle_from_server_read(self, clientServerConnection: ClientServerConnection):
        '''
         check possible states from clientServerConnection.machine.state
         match exact state with re (regular expression) patterns
         call appropriate handlers for each state
        '''
        try:
            line = clientServerConnection.socket.readline()
        except socket.timeout:
            self.logger.log(level=logging.WARNING, msg=f'Timeout on client read')
            self.sock.close()
            return

        current_state = clientServerConnection.machine.state

        if current_state == HELO_STATE:
            HELO_matched = re.search(HELO_pattern, line)
            if HELO_matched:
                command = HELO_matched.group(1)
                domain = HELO_matched.group(2) or "unknown"
                clientServerConnection.mail.helo_command = command
                clientServerConnection.mail.domain = domain
                clientServerConnection.machine.HELO(clientServerConnection.socket, clientServerConnection.socket.address, domain)
                return
        elif current_state == MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                clifilesInProcessentServerConnection.mail.from_ = MAIL_FROM_matched.group(1)
                clientServerConnection.machine.MAIL_FROM(clientServerConnection.socket, clientServerConnection.mail.from_)
                return
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

    def handle_to_server_write(self, clientServerConnection: ClientServerConnection):
        current_state = clientServerConnection.machine.state
        if current_state == GREETING_WRITE_STATE:
            clientServerConnection.machine.GREETING_write(clientServerConnection.socket)
        elif current_state == HELO_WRITE_STATE:
            clientServerConnection.machine.HELO_write(clientServerConnection.socket, clientServerConnection.socket.address, clientServerConnection.mail.domain)
        elif current_state == MAIL_FROM_WRITE_STATE:
            clientServerConnection.machine.MAIL_FROM_write(clientServerConnection.socket, clientServerConnection.mail.from_)
        elif current_state == RCPT_TO_WRITE_STATE:
            clientServerConnection.machine.RCPT_TO_write(clientServerConnection.socket, clientServerConnection.mail.to)
        elif current_state == DATA_WRITE_STATE:
            clientServerConnection.machine.DATA_start_write(clientServerConnection.socket)
        elif current_state == DATA_END_WRITE_STATE:
            clientServerConnection.machine.DATA_end_write(clientServerConnection.socket, clientServerConnection.mail.file_path)
        elif current_state == QUIT_WRITE_STATE:
            clientServerConnection.machine.QUIT_write(clientServerConnection.socket)
            clientServerConnection.socket.close()
            self.clients.pop(clientServerConnection.socket.connection)
        else:
            pass
            # print(current_state)
            # clientServerConnection.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')

    def run(self):
        clientHelper = cu.ClientHelper()
        # получение новых писем из папки maildir:
        filesInProcess = clientHelper.maildir_handler()

        self.clientSockets = set()
        # открываем количество сокетов, равное количеству сообщений для обработки в папке maildir(:)
        # TODO:11.01.20: [можно вынести чтение клиентских писем из файловой системы в отдельный поток,
        # тогда понадобится также очередь со списком ещё необработанных filesInProcess, из которой
        # потоки типа WorkingThread будут забирать данные для открытия сокетов,
        # а открывать каждый из них будет не более, допустим, 10, но не менее числа вычитанных mx-записей //
        # числа забранных из очереди сообщений данным потоком]
        for file in filesInProcess:
            # clientHelper.socket_init(file_.mx_host, file_.mx_port)
            m = Mail(to=[])
            mail = m.from_file(file)
            mx = clientHelper.get_mx(mail.domain)[0]
            if mx == '-1':
                print(mail.domain + ' error')
                continue
            new_socket = clientHelper.socket_init(host=mx)
            self.clientSockets.add(new_socket)
            # self.connections[socket] = Client(socket,'.', m)

        try:
            while True:
                rfds, wfds, errfds = select.select(self.clientSockets, self.clientSockets, [], 5)
                for fds in rfds:
                    self.handle_from_server_read(ClientServerConnection(self.clientSockets[fds]))
                for fds in wfds:
                    self.handle_to_server_write(ClientServerConnection(self.clientSockets[fds]))
        except ValueError:
            pass

    def terminate(self) -> None:
        self.active = False


if __name__ == '__main__':
    with MailClient(threads=1) as mainMailClient:
        mainMailClient.sendMailInMultipleThreads()

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
