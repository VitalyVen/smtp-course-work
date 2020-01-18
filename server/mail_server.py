import socket
import multiprocessing
import logging
import select
import threading

from common.logger_threads import CustomLogHandler
from server_config import SERVER_PORT, READ_TIMEOUT, LOG_FILES_DIR, WRITE_TIMEOUT, DNS_RESERV
from client_collection import ClientsCollection
from client import Client
from client_socket import ClientSocket
from state import *
from common.custom_logger_proc import QueueProcessLogger
from time import sleep
from dns import *

class MailServer(object):
    def __init__(self, host='localhost', port=2556, logdir='logs/logging.log'):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()

        self.processes = []
        self.logdir = logdir
        logger = logging.getLogger()
        logger.addHandler(CustomLogHandler(self.logdir))
        logger.setLevel(logging.DEBUG)
        self.logger=logger

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

        thrd = threading.Thread(target=run,args=(self,))
        thrd.daemon = True
        thrd.start()
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
                ip = cl.socket.address[0]
                cl.mail.helo_command     = command
                cl.mail.domain   = domain
                if dns_check(ip,domain)==False:
                    cl.machine.QUIT(cl.socket)
                else:
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
            cl.poll_metka = False
            cl.machine.DATA_start_write(cl.socket)
        elif current_state == DATA_END_WRITE_STATE:

            cl.machine.DATA_end_write(cl.socket, cl.mail.file_path)
        elif current_state == QUIT_WRITE_STATE:
            cl.machine.QUIT_write(cl.socket)
        elif current_state == FINISH_STATE:
            cl.socket.close()
            # self.clients.pop(cl.socket.connection)
        else:
            pass
            # print(current_state)
            # cl.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()

        # for p in self.processes:
        #     p.join(timeout=2)
        # self.logger.terminate()
        # self.logger.join(timeout=2)
        self.clients.__exit__(exc_type, exc_val, exc_tb)

def run(serv:MailServer):
    try:
        _EVENT_READ = select.POLLIN
        _EVENT_WRITE = select.POLLOUT
        pollerSockets = select.poll()
        pollerSockets.register(serv.sock, select.POLLIN)


        while True:  # for make terminate() work
            fdVsEventRead = pollerSockets.poll(READ_TIMEOUT)
            for descriptor, Event in fdVsEventRead:
                if Event == _EVENT_READ:
                    if descriptor == serv.sock.fileno():
                        connection, client_address = serv.sock.accept()
                        client = Client(socket=ClientSocket(connection, client_address), logdir=serv.logdir)
                        serv.clients[connection] = client
                        poll_wait_for_socket(pollerSockets,connection,write=True)
                    else:
                        socketFromDescriptor = serv.clients.socket(descriptor)
                        data = serv.clients.data
                        if data[socketFromDescriptor].machine.state==DATA_END_WRITE_STATE:

                            serv.handle_client_read(serv.clients[socketFromDescriptor])
                            poll_wait_for_socket(pollerSockets, socketFromDescriptor, read=True)
                        else:
                            serv.handle_client_read(serv.clients[socketFromDescriptor])
                            poll_wait_for_socket(pollerSockets, socketFromDescriptor,write= True)

                if Event == _EVENT_WRITE:
                    socketFromDescriptor = serv.clients.socket(descriptor)
                    data = serv.clients.data
                    if data[socketFromDescriptor].machine.state == QUIT_WRITE_STATE:
                        serv.handle_client_write(serv.clients[socketFromDescriptor])
                        poll_wait_for_socket(pollerSockets,socketFromDescriptor, write=True)
                    elif data[socketFromDescriptor].machine.state ==FINISH_STATE:
                        serv.handle_client_write(serv.clients[socketFromDescriptor])
                        pollerSockets.unregister(descriptor)
                    else:
                        serv.handle_client_write(serv.clients[socketFromDescriptor])
                        poll_wait_for_socket(pollerSockets, socketFromDescriptor, read=True)


    except (KeyboardInterrupt, ValueError, socket.timeout) as e:
        print(e)



def poll_wait_for_socket(poll,sock, read=False, write=False):
    if not read and not write:
        raise RuntimeError("poll error")
    mask = 0
    if read:
        mask |= select.POLLIN
    if write:
        mask |= select.POLLOUT

    poll.register(sock, mask)

def dns_check(ip,domain):
    if DNS_RESERV==False:
        return True
    ptr = reversename.from_address(ip)
    reversed_dns = str(resolver.query(ptr, "PTR")[0])

    if reversed_dns!=domain:
        return False
    else:
        return True