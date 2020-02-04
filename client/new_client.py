import logging
import select
import socket
import sys
import threading
import re
import queue
import multiprocessing
from time import sleep

from SMTP_CLIENT_FSM import *
from client_server_connection import ClientServerConnection
from utils import ClientHelper

from client_state import * #GREETING_STATE as GREETING_STATE
# from client_state import EHLO_WRITE_STATE as EHLO_WRITE_STATE
# from client_state import MAIL_FROM_WRITE_STATE as MAIL_FROM_WRITE_STATE
# from client_state import RCPT_TO_WRITE_STATE as RCPT_TO_WRITE_STATE
# from client_state import DATA_STRING_WRITE_STATE as DATA_STRING_WRITE_STATE
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

mailDirGlobalQueue = multiprocessing.Queue()


isNotFirstClientRead = False

class MailClient(object):
    def __init__(self, logdir ='logs', threads=5):
        self.threads_cnt = threads
        self.threads = []
        # self.logdir = logdir
        #self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __enter__(self):
        return self

    def getMailFromMailDir(self):
        pass

    def sendMailInAThread(self, blocking=True):
        # это главный метод класса, в нём реализована многопоточность
        # (параллельный вызов WorkingThread() в количестве threads_cnt потоков)
        for i in range(self.threads_cnt):
            th = WorkingThread(self)
            # kill thread gracefully by adding kill() method of a thread to exit() method of MailClient object:
            th.daemon = True
            th.name = 'Working Thread {}'.format(i)
            th.start()
        #self.logger.log(level=logging.DEBUG, msg=f'Started {self.threads_cnt} threads')
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
        # for th in self.threads:
        #     th.terminate()
        # for th in self.threads:
        #     th.join(timeout=2)
        #self.logger.log(level=logging.DEBUG, msg=f'gracefull close of the main process')
        print('gracefull close of the main process')
        #self.logger.terminate()
        #self.logger.join(timeout=2)
        # sys.exit()


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


class WorkingThread(): #WorkingThread(threading.Thread):

    def __init__(self, mainClientFromArg: MailClient, logdir ='logs', *args, **kwargs):
        super(WorkingThread, self).__init__(*args, **kwargs)
        self.active = True
        self.mailClientMain = mainClientFromArg
        # self.clientSockets = []
        self.clientServerConnectionList = []
        self.logdir = logdir
        #self.logger = QueueProcessLogger(filename=f'{logdir}/log.log')

    def __exit__(self, exc_type, exc_val, exc_tb):
        for clientServerConnection in self.clientServerConnectionList:
            clientServerConnection.socket.shutdown()  #TODO: probably change to shutdown only (need to Google it!)
            clientServerConnection.socket.close()
            #self.logger.log(level=logging.DEBUG, msg=f'gracefull close of a thread')
            print('gracefull close of a thread')

    def handle_talk_to_server_RW(self, clientServerConnection: ClientServerConnection, readFlag):
        '''
         check possible states from clientServerConnection.machine.state
         match exact state with re (regular expression) patterns
         call appropriate handlers for each state
        '''
        # clientServerConnection.socket.connection.setblocking(0)

        print('inside handle_talk_to_server_RW, after setblocking is set to null for clientServerConnection, readFlag: ' + str(readFlag))

        line = ""
        if readFlag:
            try:
                line = clientServerConnection.socket.readline()
                #self.logger.log(level=logging.DEBUG, msg=f"clientServerConnection.socket.readLine(): {line}\n")
            except socket.timeout:
                #self.logger.log(level=logging.WARNING, msg=f'Timeout on clientServerConnection read')
                clientServerConnection.socket.close()
                return

        # N.B.: состояние FSM-машины здесь привязано к передаваемому в аргументах сокету(:)

        current_state = clientServerConnection.machine.state
        print('inside handle_talk_to_server_RW, AFTER current_state is set, current_state: ' + current_state)

        if current_state == GREETING_STATE:
            # # N.B.: waiting until socket is ready to read (if socket ready to write is triggered first):
            # if not readFlag:
            #     print('if not readFlag code part')
            #     clientServerConnection.socket.sendall(f''.encode())
            #     print('if not readFlag code part, after clientServerConnection.socket.sendall(f"".encode())')
            #     return
            GREETING_matched = re.match(GREETING_pattern, line)
            if GREETING_matched:
                print('if GREETING_matched, GREETING_matched.group(2): ' + GREETING_matched.group(2))
                clientServerConnection.machine.EHLO(GREETING_matched.group(2), line)
                print('current readLine() in GREETING_STATE: ' + line)
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == EHLO_WRITE_STATE:
            HELO_matched = re.search(HELO_pattern_CLIENT, clientServerConnection.mail.helo_command) or 'localhost'
            clientServerConnection.machine.EHLO_write(clientServerConnection.socket, HELO_matched.group(2))
            print('current readLine() in EHLO_WRITE_STATE: ' + line)
            print('current HELO_matched.group(2): ' + HELO_matched.group(2))
            return
        elif current_state == EHLO_STATE:
            print('enter EHLO_STATE')
            EHLO_end_matched = re.search(EHLO_end_pattern, line)
            if EHLO_end_matched:
                clientServerConnection.machine.MAIL_FROM()
                print('current readLine() in EHLO_STATE EHLO_end_matched: ' + line)
                return
            else:
                EHLO_matched = re.search(EHLO_pattern, line)
                if EHLO_matched:
                    clientServerConnection.machine.EHLO_again()
                    print('current readLine() in EHLO_STATE EHLO_matched: ' + line)
                    return
                # else:
                #     clientServerConnection.machine.ERROR__()
                #     return
        elif current_state == MAIL_FROM_WRITE_STATE:
            clientServerConnection.machine.MAIL_FROM_write(clientServerConnection.socket, clientServerConnection.mail.from_)
            return
        elif current_state == MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                clientServerConnection.machine.RCPT_TO()
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == RCPT_TO_WRITE_STATE:
            indexOfCurrentReceipient = len(clientServerConnection.mail.to) - clientServerConnection.receipientsLeft
            clientServerConnection.machine.RCPT_TO_write(clientServerConnection.socket,
                                                         clientServerConnection.mail.to[indexOfCurrentReceipient])
            clientServerConnection.receipientsLeft -= 1
            return
        elif current_state == RCPT_TO_STATE:
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            RCPT_TO_WRONG_matched = re.search(RCPT_TO_WRONG_pattern, line)
            if clientServerConnection.receipientsLeft == 0:
                clientServerConnection.machine.DATA_start()
                return
            elif RCPT_TO_matched:
                clientServerConnection.machine.RCPT_TO_additional(False)
                return
            elif RCPT_TO_WRONG_matched:
                clientServerConnection.machine.RCPT_TO_additional(True)
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == DATA_STRING_WRITE_STATE:
            clientServerConnection.machine.DATA_start_write(clientServerConnection.socket)
            return
        elif current_state == DATA_STATE:
            DATA_matched = re.search(DATA_pattern, line)
            if DATA_matched:
                clientServerConnection.machine.DATA()
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == DATA_WRITE_STATE:
            clientServerConnection.machine.DATA_write(clientServerConnection.socket, clientServerConnection.mail.body)
            return
        elif current_state == DATA_END_WRITE_STATE:
            clientServerConnection.machine.DATA_end_write(clientServerConnection.socket)
            return
        elif current_state == DATA_END_STATE:
            DATA_END_matched = re.search(DATA_END_pattern, line)
            SERVICE_UNAVAILABLE_matched = re.search(SERVICE_UNAVAILABLE_pattern, line)
            if DATA_END_matched:
                clientServerConnection.machine.QUIT('250 OK')
                return
            elif SERVICE_UNAVAILABLE_matched:
                clientServerConnection.machine.QUIT('451 4.7.1 Sorry, the service is currently unavailable. Please come back later.')
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == QUIT_WRITE_STATE:
            clientServerConnection.machine.QUIT_write(clientServerConnection.socket)
            return
        elif current_state == QUIT_STATE:
            QUIT_matched = re.search(QUIT_pattern, line)
            if QUIT_matched:
                clientServerConnection.machine.FINISH()  #clientServerConnection.machine.FINISH(clientServerConnection.socket)
                return
            # else:
            #     clientServerConnection.machine.ERROR__()
            #     return
        elif current_state == FINISH_STATE:
            # N.B.: for gracefull shutdown of the socket(:)
            clientServerConnection.socket.shutdown(socket.SHUT_RDWR)
            clientServerConnection.socket.close()
            # t_d_: pop out socket from clientSockets set
            return
        elif current_state == ERROR_STATE:
            # N.B.: for gracefull shutdown of the socket(:)
            # clientServerConnection.socket.shutdown(socket.SHUT_RDWR)
            clientServerConnection.socket.close()
            # t_d_: pop out socket from clientSockets set
            return
        # else:
            # pass

        # N.B.: [мы можем оказаться в этой части кода только, если блок -elif-+/-else НЕ нашёл совпадений,
        # т.к. в каждой его ветви стоит return из текущего метода](:)
        # print('current_state: ' + current_state)
        #self.logger.log(level=logging.DEBUG, msg=f'current_state is {current_state}')
        clientServerConnection.socket.sendall(f'500 Unrecognised command {line}\n'.encode())
        print('current readLine(): ' + line)
        print('500 Unrecognised command')
        #self.logger.log(level=logging.DEBUG, msg=f'Sent response: "500 Unrecognised command" to the server')
        print('last state: ' + current_state)
        clientServerConnection.machine.ERROR()
        # current_state = ERROR_STATE


    def checkMaildirAndCreateNewSocket(self):
        #  получение новых писем из папки maildir:
        # filesInProcess = mailDirGlobalQueue.get()

        # способ без очереди
        clientHelper = ClientHelper()
        # получение новых писем из папки maildir:
        filesInProcess_fromMain = clientHelper.maildir_handler()

        # self.clientSockets = []
        for file in filesInProcess_fromMain:  # filesInProcess:
            # clientHelper.socket_init(file_.mx_host, file_.mx_port)
            m = Mail(to=[])
            mail = m.from_file(file)
            mx = clientHelper.get_mx(mail.domain)[0]
            if mx == '-1':
                print(mail.domain + ' error')
                continue
            new_socket = clientHelper.socket_init(host=mx)

            # client = ClientServerConnection(socket_of_client_type=ClientSocket(new_socket, new_socket.getsockname()), mail=mail) #logdir (!!!)
            ## serv.clients[connection] = client

            ## self.clientSockets.append([])
            ## cur_cs_list_index = len(self.clientSockets) - 1
            ## self.clientSockets[cur_cs_list_index].append(new_socket)
            ## self.clientSockets[cur_cs_list_index].append(mail)

            # self.clientSockets.append(new_socket)

            ## self.connections[socket] = Client(socket,'.', m)
            ## return self.clientSockets
            new_client_server_connection = ClientServerConnection(socket_of_client_type=new_socket, mail=mail)
            self.clientServerConnectionList.append(new_client_server_connection)

    def poll_change_read_write(self, poll, sock, read=False, write=False):
        if not read and not write:
            raise RuntimeError("poll error")
        mask = 0
        # N.B.: переключаем события логическим ИЛИ:
        if read:
            mask |= select.POLLIN
        if write:
            mask |= select.POLLOUT
        poll.register(sock, mask)

    # isNotFirstClientRead = False

    def run(self):
        global isNotFirstClientRead
        try:
            while True:
                # self.clientSockets.clear()
                print('before self.checkMaildirAndCreateNewSocket()')
                self.checkMaildirAndCreateNewSocket()
                print('after self.checkMaildirAndCreateNewSocket()')

                list_of_sockets = []
                for clientServerConnection in self.clientServerConnectionList:
                  print('before list_of_sockets.append(...)')
                  list_of_sockets.append(clientServerConnection.socket.connection) #= [x for [x, y] in self.clientSockets]
                  print(clientServerConnection.socket.connection)
                  print(clientServerConnection.socket.connection.fileno())
                  print('after list_of_sockets.append(...)')

                if list_of_sockets:
                    print('before select.select(...)')
                    print('list_of_sockets len(): ' + str(len(list_of_sockets)))
                    rfds, wfds, errfds = select.select(list_of_sockets, list_of_sockets, [], 5)
                    print('after select.select(...)')
                    for fds in rfds:
                        isNotFirstClientRead = True
                        # N.B.: ...next(...) returns clientServerConnection.socket.connection for connection == fds(:)
                        print('inside rfds')
                        socket_conn_ = next(filter(lambda x: x.socket.connection == fds, self.clientServerConnectionList))
                        print('inside rfds after socket_conn_')
                        self.handle_talk_to_server_RW(socket_conn_,
                                                      # ClientServerConnection(self.clientSockets[fds][0], self.clientSockets[fds][1]),
                                                      True)
                        print('inside rfds after handle_talk_to_server_RW')
                    if isNotFirstClientRead:
                        for fds in wfds:
                            # N.B.: ...next(...) returns clientServerConnection.socket.connection for connection == fds(:)
                            print('inside wfds')
                            socket_conn_ = next(filter(lambda x: x.socket.connection == fds, self.clientServerConnectionList))
                            print('inside wfds after socket_conn_')
                            self.handle_talk_to_server_RW(socket_conn_,
                                                          # ClientServerConnection(self.clientSockets[fds][0], self.clientSockets[fds][1]),
                                                          False)
                            print('inside wfds after handle_talk_to_server_RW')
                    for fds in errfds:
                        print('Here is one of error fds (errfds), error fd: ' + fds)
        except (KeyboardInterrupt, ValueError, socket.timeout) as e_0:
            print(e_0)

        # try:
        #     _EVENT_READ = select.POLLIN
        #     _EVENT_WRITE = select.POLLOUT
        #
        #     pollerSockets = select.poll()
        #     while True:
        #         self.checkMaildirAndCreateNewSocket()
        #         list_of_sockets = []
        #         for clientServerConnection in self.clientServerConnectionList:
        #             list_of_sockets.append(clientServerConnection.socket.connection)
        #
        #         # изначально регистрируем / устанавливаем все сокеты на чтение:
        #         for sock in list_of_sockets:
        #             pollerSockets.register(sock, select.POLLIN)
        #
        #         tuple_of_fds_and_events = pollerSockets.poll()  # .poll(timeout=3.0)
        #         for fds, Event in tuple_of_fds_and_events:
        #             if Event == _EVENT_READ:
        #                 # N.B.: ...next(...) returns clientServerConnection.socket.connection for connection == fds(:)
        #
        #                 socket_conn_ = next(filter(lambda x: x.socket.connection.fileno() == fds, self.clientServerConnectionList))
        #
        #                 self.handle_talk_to_server_RW(socket_conn_, True)
        #
        #                 self.poll_change_read_write(pollerSockets,
        #                                             next(filter(lambda x: x.socket.connection.fileno() == fds, self.clientServerConnectionList)).socket.connection,
        #                                             read=False, write=True)
        #             elif Event == _EVENT_WRITE:
        #                 # N.B.: ...next(...) returns clientServerConnection.socket.connection for connection == fds(:)
        #                 self.handle_talk_to_server_RW(next(filter(lambda x: x.socket.connection.fileno() == fds, self.clientServerConnectionList)).socket.connection, False)
        #
        #                 self.poll_change_read_write(pollerSockets, pollerSockets,
        #                                             next(filter(lambda x: x.socket.connection.fileno() == fds, self.clientServerConnectionList)).socket.connection,
        #                                             read=True, write=False)
        # except (KeyboardInterrupt, ValueError, socket.timeout) as e_1:
        #     print(e_1)

    def terminate(self) -> None:
        self.active = False

if __name__ == '__main__':
    with MailClient(threads=1) as mainMailClient:
        # th = WorkingThread(mainClientFromArg=mainMailClient) #threading.Thread(target=run,args=(self,))
        # ## kill thread gracefully by adding kill() method of a thread to exit() method of MailClient object:
        # th.daemon = True
        # th.name = 'Working Thread {}'
        # th.start()
        # while True:
        #     ## clientHelper = ClientHelper()
        #     ## # получение новых писем из папки maildir:
        #     ## filesInProcess_fromMain = clientHelper.maildir_handler()
        #     ## mailDirGlobalQueue.put(filesInProcess_fromMain)
        #     ## # mainMailClient.sendMailInAThread()
        #     try:
        #         sleep(0.1)
        #     except KeyboardInterrupt as e:
        #         mainMailClient.__exit__(type(e), e, e.__traceback__)

        fakeThread = WorkingThread(mainClientFromArg=mainMailClient)
        while True:
            try:
                fakeThread.run()
            except KeyboardInterrupt as e:
                mainMailClient.__exit__(type(e), e, e.__traceback__)
