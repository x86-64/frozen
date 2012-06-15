PELICAN=pelican
PELICANOPTS=

BASEDIR=$(PWD)
INPUTDIR=$(BASEDIR)/src
OUTPUTDIR=$(BASEDIR)/blog
CONFFILE=$(BASEDIR)/pelican.conf.py

FTP_HOST=localhost
FTP_USER=anonymous
FTP_TARGET_DIR=/

SSH_HOST=locahost
SSH_USER=root
SSH_TARGET_DIR=/var/www

DROPBOX_DIR=~/Dropbox/Public/

help:
	@echo 'Makefile for a pelican Web site                                        '
	@echo '                                                                       '
	@echo 'Usage:                                                                 '
	@echo '   make html                        (re)generate the web site          '
	@echo '   make clean                       remove the generated files         '
	@echo '   ftp_upload                       upload the web site using FTP      '
	@echo '   ssh_upload                       upload the web site using SSH      '
	@echo '   dropbox_upload                   upload the web site using Dropbox  '
	@echo '   rsync_upload                     upload the web site using rsync/ssh'
	@echo '                                                                       '


all: clean $(OUTPUTDIR)/index.html
	cd wiki/ && markdoc build
	@echo 'Done'

$(OUTPUTDIR)/%.html:
	$(PELICAN) $(INPUTDIR) -o $(OUTPUTDIR) -s $(CONFFILE) $(PELICANOPTS)

clean:
	rm -fr $(OUTPUTDIR)
	mkdir $(OUTPUTDIR)

check:
	@echo 'ok'

dropbox_upload: $(OUTPUTDIR)/index.html
	cp -r $(OUTPUTDIR)/* $(DROPBOX_DIR)

ssh_upload: $(OUTPUTDIR)/index.html
	scp -r $(OUTPUTDIR)/* $(SSH_USER)@$(SSH_HOST):$(SSH_TARGET_DIR)

rsync_upload: $(OUTPUTDIR)/index.html
	rsync -e ssh -P -rvz --delete $(OUTPUTDIR)/* $(SSH_USER)@$(SSH_HOST):$(SSH_TARGET_DIR)

ftp_upload: $(OUTPUTDIR)/index.html
	lftp ftp://$(FTP_USER)@$(FTP_HOST) -e "mirror -R $(OUTPUTDIR) $(FTP_TARGET_DIR) ; quit"

github: $(OUTPUTDIR)/index.html
	ghp-import $(OUTPUTDIR)
	git push origin gh-pages

.PHONY: all help clean check ftp_upload ssh_upload rsync_upload dropbox_upload github
