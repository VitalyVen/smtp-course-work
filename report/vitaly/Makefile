# Отчёт
REPORT_SERVER = report-server.pdf
REPORT_CLIENT = report-client.pdf
TEXDIR = tex
# latex -> pdf
PDFLATEX = pdflatex -interaction=nonstopmode
# Отчёт. PDFLATEX вызывается дважды для нормального 
# создания ссылок, это НЕ опечатка.
$(REPORT_SERVER): 	
	cd $(TEXDIR) && $(PDFLATEX) report-server.tex && $(PDFLATEX) report-server.tex && cp $(REPORT_SERVER) ..

$(REPORT_CLIENT): 	
	cd $(TEXDIR) && $(PDFLATEX) report-server.tex && $(PDFLATEX) report-server.tex && cp $(REPORT_CLIENT) ..


all : build report-server report-client

help:
	@echo "Use 'make build' for get client and server for your OS."
	@echo "Use 'make report' for get report (only for linux)."
	@echo "User 'make all' for get all programs and report."

build : build-client build-server
	$(DOTNET) publish -o release/$(detected_OS) -c release -r $(detected_OS)
	
build-client :

build-server: 

report-server : $(REPORT_SERVER)

report-client : $(REPORT_CLIENT)

report-clean :
	rm -f -- $(REPORT_SERVER)
	rm -f -- $(REPORT_CLIENT)

dotnet-clean:
	$(DOTNET) clean

clean : report-clean
	


	