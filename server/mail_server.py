import multiprocessing
import socket
import sys
import threading
import logging
import select

from common.logger_threads import CustomLogHandler
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR, WRITE_TIMEOUT
from client_collection import ClientsCollection
from client import Client
from client_socket import ClientSocket
from state import *
from common.custom_logger_proc import QueueProcessLogger
from dns import *

class MailServer(object):
    def __init__(self, host='localhost', port=2556, threads=5, logdir='logs',blocking=True):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()
        self.proc = None
        self.blocking=blocking
        self.logdir = logdir
        self.logger = logging.getLogger()
        self.logger.addHandler(CustomLogHandler('../logs/logging.log'))
        self.logger.setLevel(logging.DEBUG)

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

    def mail_start(self):
        thread = threading.Thread(target=mail_poll_logic, args=(self,))
        thread.daemon = True
        thread.start()

        while self.blocking:
            pass



    def stop(self):
        self.blocking=False
        self.sock.close()
        quit()
        sys.exit()

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

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()
        # for p in self.processes:
        #     p.terminate()
        # for p in self.processes:
        #     p.join(timeout=2)
        self.logger.terminate()
        # self.logger.join(timeout=2)
        # self.clients.__exit__(exc_type, exc_val, exc_tb)



def mail_poll_logic(serv:MailServer):

    try:
        _EVENT_READ = select.POLLIN
        _EVENT_WRITE = select.POLLOUT
        pollerSocketsForRead = select.poll()
        pollerSocketsForWrite = select.poll()
        pollerSocketsForRead.register(serv.sock, select.POLLIN)
        pollerSocketsForWrite.register(serv.sock, select.POLLOUT)

        while (True):
            fdVsEventRead = pollerSocketsForRead.poll(READ_TIMEOUT)
            fdVsEventWrite = pollerSocketsForWrite.poll(WRITE_TIMEOUT)

            for descriptor, Event in fdVsEventRead:
                if Event == _EVENT_READ:
                    if descriptor == serv.sock.fileno():
                        connection, client_address = serv.sock.accept()
                        client = Client(socket=ClientSocket(connection, client_address), logdir=serv.logdir)
                        serv.clients[connection] = client
                        pollerSocketsForRead.register(connection, select.POLLIN)
                        pollerSocketsForWrite.register(connection, select.POLLOUT)

                    else:
                        socketFromDescriptor = serv.clients.socket(descriptor)
                        serv.handle_client_read(serv.clients[socketFromDescriptor])
                        pass
            for descriptor, Event in fdVsEventWrite:
                if Event == _EVENT_WRITE:
                    socketFromDescriptor = serv.clients.socket(descriptor)
                    serv.handle_client_write(serv.clients[socketFromDescriptor])
    except ValueError:
        pass

# def start():
#     with MailServer(port=SERVER_PORT, logdir=LOG_FILES_DIR) as server:
#         server.mail_poll_logic()
#
# def stop_server(proc):
#     proc.kill()