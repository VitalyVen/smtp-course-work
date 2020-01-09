import socket
import threading
import logging
import select
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR
from client_collection import ClientsCollection
from client import Client
from client_socket import ClientSocket
from state import *
from common.custom_logger_proc import QueueProcessLogger


class MailServer(object):
    def __init__(self, host='localhost', port=2556, threads=5, logdir='logs'):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()
        self.threads_cnt = threads
        self.logdir = logdir
        self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __enter__(self):
        self.socket_init()
        return self

    def socket_init(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.settimeout(READ_TIMEOUT)
        server_address = (self.host, self.port)
        self.sock.bind(server_address)
        self.sock.listen(0)
        self.logger.log(level=logging.DEBUG, msg=f'Server socket initiated on port: {self.port}')

    def serve(self, blocking=True):
        '''

        :param blocking: daemon=True means all threads finish when main thread will be finished
        so we should do nothing with blocking=True or other actions in main thread
        :return: None
        '''
        for i in range(self.threads_cnt):
            thread_sock = threading.Thread(target=thread_socket, args=(self,))
            thread_sock.daemon = True
            thread_sock.name = 'Working Thread {}'.format(i)
            thread_sock.start()
        self.logger.log(level=logging.DEBUG, msg=f'Started {self.threads_cnt} threads')
        while blocking:
            pass
        
    def handle_client_read(self, cl:Client):
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
                command     = HELO_matched.group(1)
                domain      = HELO_matched.group(2) or "unknown"
                cl.mail.helo_command     = command
                cl.mail.domain   = domain
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
                mail_to         = RCPT_TO_matched.group(1)
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
                    cl.data_start_already_matched=False
                else:# Additional data case
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
                    pass#TODO: incorrect command to message to client

            return
        elif current_state == QUIT_STATE:
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

    def handle_client_write(self, cl:Client):
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
            self.clients.pop(cl.socket.connection)
        else:
            pass
            # print(current_state)
            # cl.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()

def thread_socket(serv:MailServer):
    while True:
        client_sockets = serv.clients.sockets()
        rfds, wfds, errfds = select.select([serv.sock] + client_sockets, client_sockets, [], 5)
        for fds in rfds:
            if fds is serv.sock:
                connection, client_address = fds.accept()
                client = Client(socket=ClientSocket(connection, client_address), logdir=serv.logdir)
                serv.clients[connection] = client
            else:
                serv.handle_client_read(serv.clients[fds])
        for fds in wfds:
            serv.handle_client_write(serv.clients[fds])