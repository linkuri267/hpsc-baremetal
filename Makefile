all: trch rtps trch-bl0

trch:
	$(MAKE) -C trch
rtps:
	$(MAKE) -C rtps
trch-bl0:
	$(MAKE) -C trch-bl0 

clean:
	$(MAKE) -C trch clean
	$(MAKE) -C rtps clean
	$(MAKE) -C trch-bl0 clean

.PHONY: rtps trch trch-bl0
