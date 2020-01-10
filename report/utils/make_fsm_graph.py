import sys
import os
from server.SMTP_FSM import SMTP_FSM
# from client.C import SMTP_FSM_CLIENT
machine = SMTP_FSM('graph', '.')
machine.get_graph().draw(os.path.join(os.path.dirname(__file__), os.pardir, 'data/my_state_diagram_server.png'), prog='dot')

# machine = SMTP_FSM_CLIENT('graph')
# machine.get_graph().draw(os.path.join(os.path.dirname(__file__), os.pardir, 'data/my_state_diagram_client.png'), prog='dot')
