from nrf24USB import NRF24Device
from nrf24Smart import (
    DeviceMessage,
    HostMessage,
    MSG_TYPES,
    SetMessage,
    CHANGE_TYPES,
    supported_devices,
)
import time
import logging

logger = logging.getLogger("") 
logger.setLevel(logging.INFO)


device = NRF24Device("COM9", channel=101, address=99)
target_ID = 2


if device.error:
    raise ConnectionError("Error with the NRF24USB device")
device.wait_for_init()

start_time = time.time()
# for j in range(256):
#     for i in range(256):
dummy_set = SetMessage(index=0,changeType=CHANGE_TYPES.SET,value=0)
#print(dummy_set.get_raw())
dummy_msg = HostMessage([182, 68, 225, 237], MSG_TYPES.SET, [0, 1, 8, 1,255,255,255,255,255,255,255])
res = device.send_msg(target_ID, dummy_msg.get_raw(), True)
print(res)
device.stop_read_loop()