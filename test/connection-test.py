from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage, HostMessage, MSG_TYPES, SetMessage, CHANGE_TYPES, supported_devices
import time

device = NRF24Device("/dev/ttyUSB0", channel=101, address=0)
start_time = time.time()
dummy_msg = HostMessage([0,1,2,3], MSG_TYPES.INIT, [])
num_msg = 100
num_failed = 0
num_retried = 0
for i in range(num_msg):
    res = device.send_msg(1, dummy_msg.get_raw(), True)
    if res == None:
        num_retried += 1
        res = device.send_msg(1, dummy_msg.get_raw(), True)
        if res == None:
            num_failed += 1
    print(res)
    #time.sleep(0.1)

print(f"failed: {100*num_failed/num_msg}%, retried: {num_retried}")