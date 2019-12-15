import sys
import os
from server.main import SMTP_FSM
machine = SMTP_FSM('graph')
machine.get_graph().draw(os.path.join(os.path.dirname(__file__), os.pardir, 'data/my_state_diagram.png'), prog='dot')

