import os
from PyPDF2 import PdfFileMerger

if __name__ == '__main__':
    os.system('make report-server.pdf')
    pdfs = ['Первые_2_страницы_v.pdf', 'report-server.pdf']
    merger = PdfFileMerger()
    for pdf in pdfs:
        merger.append(pdf)
    merger.write("result.pdf")
    merger.close()