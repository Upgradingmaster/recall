recall : recall.c db.c 
	gcc -Wall -Wextra -O0 -ggdb -lsqlite3 -o recall recall.c db.c 

.PHONY: install
install: recall
	install ./recall ${HOME}/.local/bin/recall
	
