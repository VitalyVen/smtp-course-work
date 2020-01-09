import socket
import multiprocessing
import logging
import select
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR
from client_collection import ClientsCollection
from client import Client
from client_socket import ClientSocket
from state import *
from common.custom_logger_proc import QueueProcessLogger
from time import sleep

class MailServer(object):
    def __init__(self, host='localhost', port=2556, processes=5, logdir='logs'):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()
        self.processes_cnt = processes
        self.processes = []
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

        :param blocking: processes finish when main process will be finished (exit context manager)
        so we should do nothing with blocking=True or other actions in main thread
        :return: None
        '''
        for i in range(self.processes_cnt):
            p = WorkingProcess(self)
            # p.daemon = True #not work with processes, kill it join it in __exit__()
            self.processes.append(p)
            p.name = 'Working Process {}'.format(i)
            p.start()
        self.logger.log(level=logging.DEBUG, msg=f'Started {self.processes_cnt} processes')
        #TODO not while True, make it wait signal, without do nothing, it utilize CPU on 100%

        while blocking:
            try:
                sleep(0.1)
            except KeyboardInterrupt as e:
                self.__exit__(type(e), e, e.__traceback__)
        
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
            cl.socket.close()
            self.clients.pop(cl.socket.connection)
        else:
            pass
            # print(current_state)
            # cl.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()
        for p in self.processes:
            p.terminate()
        # for p in self.processes:
        #     p.join(timeout=2)
        self.logger.terminate()
        # self.logger.join(timeout=2)
        self.clients.__exit__(exc_type, exc_val, exc_tb)

class WorkingProcess(multiprocessing.Process):

    def __init__(self, serv:MailServer, *args, **kwargs):
        super(WorkingProcess, self).__init__(*args, **kwargs)
        self.active = True
        self.server = serv
    def terminate(self) -> None:
        self.active = False
    def run(self):
        try:
            while self.active:#for make terminate() work
                client_sockets = self.server.clients.sockets()
                rfds, wfds, errfds = select.select([self.server.sock] + client_sockets, client_sockets, [], 1)
                for fds in rfds:
                    if fds is self.server.sock:
                        connection, client_address = fds.accept()
                        client = Client(socket=ClientSocket(connection, client_address), logdir=self.server.logdir)
                        self.server.clients[connection] = client
                    else:
                        self.server.handle_client_read(self.server.clients[fds])
                for fds in wfds:
                    self.server.handle_client_write(self.server.clients[fds])
        except (KeyboardInterrupt, ValueError, socket.timeout) as e:
            self.terminate()