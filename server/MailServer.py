import socket
import threading
import select
from default import SERVER_PORT
from ClientsCollection import *
from Client import *
from state import *
from MailServer import *
from SMTP_FSM import *

class MailServer(object):
    def __init__(self, host='localhost', port=SERVER_PORT, threads=5):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()
        self.threads_cnt = threads

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

    def serve_forever(self):
        for i in range(self.threads_cnt):
            thread_sock = threading.Thread(target=thread_socket, args=(self, i,))
            thread_sock.daemon = True
            thread_sock.start()
        while True:
            pass
        
    def handle_client_read(self, cl:Client):
        try:
            line = cl.socket.readline()
        except socket.timeout:
            self.sock.close()
            return
        
        print(line)
        
        # check possible states from cl.machine.state
        # match exact state with re patterns
        # call handler for this state

        current_state = cl.machine.state

        if current_state == HELO_STATE:
            HELO_matched = re.search(HELO_pattern, line)
            if HELO_matched:
                command     = HELO_matched.group(1)
                domain      = HELO_matched.group(2) or "unknown"
                cl.mail     = f"{command}:<{domain}>\r\n"
                cl.domain   = domain
                cl.machine.HELO(cl.socket, cl.socket.address, domain)
                return
        elif current_state == MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                cl.mail_from    = MAIL_FROM_matched.group(1)
                cl.mail        += f"FROM:<{cl.mail_from}>\r\n"
                cl.machine.MAIL_FROM(cl.socket, cl.mail_from)
                return
        elif current_state == RCPT_TO_STATE:
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            if RCPT_TO_matched:
                mail_to         = RCPT_TO_matched.group(1)
                cl.mail        += f"TO:<{mail_to}>\r\n\r\n"
                cl.mail_to      = mail_to
                cl.machine.RCPT_TO(cl.socket, mail_to)
                return
        elif current_state == DATA_STATE:     # TODO: as fsm?
            DATA_start_matched = re.search(DATA_start_pattern, line)
            DATA_end_matched = re.search(DATA_end_pattern, line)
            if DATA_start_matched:          # TODO: case when data additional match data_start
                data = DATA_start_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_start(cl.socket)
            elif DATA_end_matched:          # TODO: and already started
                data = DATA_end_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_end(cl.socket)
                cl.mail_to_file()
            else:                           # TODO: only if started
                cl.mail += line
                cl.machine.DATA_additional(cl.socket)
            return
        elif current_state == QUIT_STATE:
            QUIT_matched = re.search(QUIT_pattern, line)
            if QUIT_matched:
                cl.machine.QUIT(cl.socket)
        
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
            cl.machine.HELO_write(cl.socket, cl.socket.address, cl.domain)
        elif current_state == MAIL_FROM_WRITE_STATE:
            cl.machine.MAIL_FROM_write(cl.socket, cl.mail_from)
        elif current_state == RCPT_TO_WRITE_STATE:
            cl.machine.RCPT_TO_write(cl.socket, cl.mail_to)
        elif current_state == DATA_WRITE_STATE:
            cl.machine.DATA_start_write(cl.socket)
        elif current_state == DATA_END_WRITE_STATE:
            cl.machine.DATA_end_write(cl.socket)
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

def thread_socket(serv, name):
    while True:
        client_sockets = serv.clients.sockets()
        rfds, wfds, errfds = select.select([serv.sock] + client_sockets, client_sockets, [], 100)
        if len(rfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    connection, client_address = fds.accept()
                    client = Client(socket=ClientSocket(connection, client_address))
                    client.machine.GREETING_write_handler(client.socket)
                    serv.clients[connection] = client
                else:
                    # print('Thread {}'.format(name))
                    serv.handle_client_read(serv.clients[fds])
        if len(wfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    # connection, client_address = fds.accept()
                    # client = Client(socket=ClientSocket(connection, client_address))
                    # client.machine.GREETING(client.socket)
                    # serv.clients[connection] = client
                    pass
                else:
                    # print('Thread {}'.format(name))
                    serv.handle_client_write(serv.clients[fds])