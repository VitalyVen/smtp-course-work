import os
from PyPDF2 import PdfFileMerger
from report.utils.make_fsm_graph import draw_server_fsm

if __name__ == '__main__':
    # path = os.path.join(os.path.dirname(__file__), os.pardir, 'data/my_state_diagram_server.png')
    # draw_server_fsm(path)

    os.system('make report-server.pdf')
    pdfs = ['Первые_2_страницы_v.pdf', 'report-server.pdf']
    merger = PdfFileMerger()
    for pdf in pdfs:
        merger.append(pdf)
    merger.write("result.pdf")
    merger.close()