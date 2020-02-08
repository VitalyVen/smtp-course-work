import os
from SMTP_CLIENT_FSM import SmtpClientFsm

if __name__ == '__main__':

    machine = SmtpClientFsm('graph')
    machine.get_graph().draw(os.path.join(os.path.dirname(__file__), os.pardir, 'my_state_diagram_client.png'), prog='dot')
