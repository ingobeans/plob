.PHONY: device, simulator, upload

PLAYDATE_PORT = "/dev/ttyACM0"

SDK_PATH = ${PLAYDATE_SDK_PATH}

simulator:
	mkdir build 2>/dev/null || true
	cd build && cmake .. && make
	PlaydateSimulator plob.pdx

device:
	mkdir build_device 2>/dev/null || true
	cd build_device && cmake -DCMAKE_TOOLCHAIN_FILE=${SDK_PATH}/C_API/buildsupport/arm.cmake .. && make

upload:
	make device
	sudo echo Uploading game to ${PLAYDATE_PORT}
	DEVICES=$$(ls /dev/ | grep -E "sd.{1,}1") && \
	pdutil /dev/ttyACM0 datadisk && \
	while true; do \
		NEW_DEVICES_LIST=$$(ls /dev/ | grep -E "sd.{1,}1") && \
		NEW_DEVICES=$${NEW_DEVICES_LIST#$$DEVICES}; \
		if [ $${#NEW_DEVICES} -ne 0 ]; then \
			echo "found...";\
			sleep 2.0; \
			NEW=$$(echo "$$NEW_DEVICES" | xargs); \
			DISK_PATH=$$(udisksctl mount -b /dev/$$NEW | grep -oP "(?<=at ).*"); \
			echo "mounted...";\
			cp -r *_DEVICE.pdx $$DISK_PATH/Games ; \
			echo "uploaded...";\
			udisksctl unmount -b /dev/$$NEW; \
			sudo eject /dev/$$NEW; \
			echo "finished!";\
			break; \
		fi \
	done; \


