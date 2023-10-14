all:
	gcc simple-shell.c -o simpleshell
	gcc simple-scheduler.c -o simplescheduler
clean:
	@rm -f simpleshell
	@rm -f simplescheduler