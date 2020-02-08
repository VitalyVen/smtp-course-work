import os
from PyPDF2 import PdfFileMerger
# from report.utils.make_smtp_client_fsm_graph import draw_client_fsm

if __name__ == '__main__':
    os.system('make clean')
    os.system('make report-client.pdf')
    pdfs = ['SBN_report/Первые_2_страницы_SBN.pdf', 'report-client.pdf']
    merger = PdfFileMerger()
    for pdf in pdfs:
        merger.append(pdf)
    merger.write("result.pdf")
    merger.close()