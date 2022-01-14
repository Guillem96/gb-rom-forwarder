BUILDDIR=build
TARGET=gb-forwarder

RPI_USER=pi
RPI_HOST=192.168.1.78
RPI_DESTINATION=/home/pi/$(TARGET)


directories:
	mkdir -p $(BUILDDIR)

build: directories
	go build -o $(BUILDDIR)/$(TARGET) main.go forwarder.go

rpi-build:
	GOOS=linux GOARCH=arm GOARM=5 go build -o $(BUILDDIR)/rpi-$(TARGET) main.go forwarder.go
	chmod +x $(BUILDDIR)/rpi-$(TARGET)

rpi-upload: rpi-build
	sshpass -p raspberry scp $(BUILDDIR)/rpi-$(TARGET) $(RPI_USER)@$(RPI_HOST):$(RPI_DESTINATION)

rpi-run: rpi-upload
	sshpass -p raspberry ssh $(RPI_USER)@$(RPI_HOST) "$(RPI_DESTINATION)"

run: build
	$(BUILDDIR)/$(TARGET)