#from https://github.com/SkullTech/drymail/blob/master/drymail.py#L1
import base64
import mimetypes
import select
import socket
import ssl
import threading
from datetime import time
from email import encoders, message_from_bytes
from email.header import Header
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.utils import formataddr
from smtplib import SMTP, SMTP_SSL, SMTPServerDisconnected

import mistune
import null as null
from bs4 import BeautifulSoup

from client.client_main import MailServer
from common.custom_logger_proc import QueueProcessLogger
from common.mail import Mail

class MailClient(object):
    def __init__(self, host='localhost', port=2556, processes=5, logdir='logs', threads=5):
        # self.host = host
        # self.port = port
        # self.clients = ClientsCollection()
        # self.processes_cnt = processes
        # self.processes = []
        self.threads_cnt = threads
        self.logdir = logdir
        self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __enter__(self):
        self.socket_init()
        return self

    def sendMail(self, blocking=True):
        # '''
        #
        # :param blocking: processes finish when main process will be finished (exit context manager)
        # so we should do nothing with blocking=True or other actions in main thread
        # :return: None
        # '''
        # for i in range(self.processes_cnt):
        #     p = WorkingProcess(self)
        #     # p.daemon = True #not work with processes, kill it join it in __exit__()
        #     self.processes.append(p)
        #     p.name = 'Working Process {}'.format(i)
        #     p.start()
        # self.logger.log(level=logging.DEBUG, msg=f'Started {self.processes_cnt} processes')
        #
        #
        # while blocking:
        #     try:
        #         sleep(0.1)
        #     except KeyboardInterrupt as e:
        #         self.__exit__(type(e), e, e.__traceback__)
        # TODO: это главный метод класса, в нём многопоточность и WorkingThread() параллельный вызов

        for i in range(self.threads_cnt):
            th = WorkingThread(self)
            th.daemon = True #kill thread greasfully by joining kill() method to exit()
            th.name = 'Working Thread {}'.format(i)
            th.start()
        self.logger.log(level=logging.DEBUG, msg=f'Started {self.threads_cnt} threads')
        while blocking:
            pass

    def __exit__(self, exc_type, exc_val, exc_tb):
        # self.sock.close()
        for p in self.processes:
            p.terminate()
        # for p in self.processes:
        #     p.join(timeout=2)
        self.logger.terminate()
        # self.logger.join(timeout=2)
        # self.clients.__exit__(exc_type, exc_val, exc_tb)

def thread_socket(serv: MailServer):
    try:
        while True:
            rfds, wfds, errfds = select.select(clientSockets, clientSockets, [], 5)
            for fds in rfds:
                client.handle_client_read(clientSockets[fds])
            for fds in wfds:
                client.handle_client_write(clientSockets[fds])
    except ValueError:
        pass



class WorkingThread(threading.Thread):

    def __init__(self, clientFromArg: MailClient, *args, **kwargs):
        super(WorkingThread, self).__init__(*args, **kwargs)
        self.active = True
        self.client = clientFromArg
        self.clientSockets


    def handle_client_read(self, cl: Client):
        '''
         check possible states from cl.machine.state
         match exact state with re patterns
         call handler for this state
        '''
        try:
            line = cl.socket.readline()
        except socket.timeout:
            self.logger.log(level=logging.WARNING, msg=f'Timeout on client read')
            self.sock.close()
            return

        current_state = cl.machine.state

        if current_state == HELO_STATE:
            HELO_matched = re.search(HELO_pattern, line)
            if HELO_matched:
                command = HELO_matched.group(1)
                domain = HELO_matched.group(2) or "unknown"
                cl.mail.helo_command = command
                cl.mail.domain = domain
                cl.machine.HELO(cl.socket, cl.socket.address, domain)
                return
        elif current_state == MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                cl.mail.from_ = MAIL_FROM_matched.group(1)
                cl.machine.MAIL_FROM(cl.socket, cl.mail.from_)
                return
        elif current_state == RCPT_TO_STATE:
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            if RCPT_TO_matched:
                mail_to = RCPT_TO_matched.group(1)
                cl.mail.to.append(mail_to)
                cl.machine.RCPT_TO(cl.socket, mail_to)
                return
        elif current_state == DATA_STATE:
            if cl.data_start_already_matched:
                DATA_end_matched = re.search(DATA_end_pattern, line)
                if DATA_end_matched:
                    data = DATA_end_matched.group(1)
                    if data:
                        cl.mail.body += data
                    cl.machine.DATA_end(cl.socket)
                    cl.mail.to_file()
                    cl.data_start_already_matched = False
                else:  # Additional data case
                    cl.mail.body += line
                    cl.machine.DATA_additional(cl.socket)
            else:
                # check another recepient firstly
                RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
                if RCPT_TO_matched:
                    mail_to = RCPT_TO_matched.group(1)
                    cl.mail.to.append(mail_to)
                    cl.machine.ANOTHER_RECEPIENT(cl.socket, mail_to)
                    return
                # data start secondly
                DATA_start_matched = re.search(DATA_start_pattern, line)
                if DATA_start_matched:
                    data = DATA_start_matched.group(1)
                    if data:
                        cl.mail.body += data
                    cl.machine.DATA_start(cl.socket)
                    cl.data_start_already_matched = True
                else:
                    pass  # TODO: incorrect command to message to client

            return
        QUIT_matched = re.search(QUIT_pattern, line)
        if QUIT_matched:
            cl.machine.QUIT(cl.socket)
            return
        # Transition possible from any states
        RSET_matched = re.search(RSET_pattern, line)
        if RSET_matched:
            cl.machine.RSET(cl.socket)
        else:
            pass
        # cl.socket.send(f'500 Unrecognised command {line}\n'.encode())
        # print('500 Unrecognised command')


    def handle_client_write(self, cl: Client):
        current_state = cl.machine.state
        if current_state == GREETING_WRITE_STATE:
            cl.machine.GREETING_write(cl.socket)
        elif current_state == HELO_WRITE_STATE:
            cl.machine.HELO_write(cl.socket, cl.socket.address, cl.mail.domain)
        elif current_state == MAIL_FROM_WRITE_STATE:
            cl.machine.MAIL_FROM_write(cl.socket, cl.mail.from_)
        elif current_state == RCPT_TO_WRITE_STATE:
            cl.machine.RCPT_TO_write(cl.socket, cl.mail.to)
        elif current_state == DATA_WRITE_STATE:
            cl.machine.DATA_start_write(cl.socket)
        elif current_state == DATA_END_WRITE_STATE:
            cl.machine.DATA_end_write(cl.socket, cl.mail.file_path)
        elif current_state == QUIT_WRITE_STATE:
            cl.machine.QUIT_write(cl.socket)
            cl.socket.close()
            self.clients.pop(cl.socket.connection)
        else:
            pass
            # print(current_state)
            # cl.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')

    def run(self):
        try:
            while True:
                rfds, wfds, errfds = select.select(clientSockets, clientSockets, [], 5)
                for fds in rfds:
                    self.handle_client_read(clientSockets[fds])
                for fds in wfds:
                    self.handle_client_write(clientSockets[fds])
        except ValueError:
            pass

    def terminate(self) -> None:
        self.active = False


if __name__ == '__main__':
    # with MailServer() as client:
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

    import client.utils as cu
    clientHelper = cu.ClientHelper()
    filesInProcess = clientHelper.maildir_handler()

    clientSockets = set()
    for file in filesInProcess:
        #TODO:10.01.20: добавить в записи списка filesInProcess поля mx_host и mx_port, полученные с помощью get_mx() :
        # clientHelper.socket_init(file_.mx_host, file_.mx_port)
        m = Mail(to=[])
        mail = m.from_file(file)
        mx = clientHelper.get_mx(mail.domain)[0]
        if mx == '-1':
            print(mail.domain + ' error')
            continue
        socket = clientHelper.socket_init(mx)
        clientSockets.add(socket)
        # self.connections[socket] = Client(socket,'.', m)