from nrf24USB import NRF24Device
from nrf24Smart import DeviceMessage
import time


device = NRF24Device("/dev/ttyUSB0", channel=101, address=1)
start_time = time.time()
for i in range(0):
    res = device.send_msg(0, [1, 0, 2])
    print(res)
    # if (res != None):
    #     break
# print(device.send_msg(42, [100, 99, 98, 97, 96, 95], False))
# print(device.send_msg(41, [100, 99, 98, 97, 96, 95]))
# Stop profiling

# Print the profiling results
end_time = time.time()
print(f"Execution time: {end_time - start_time} seconds")

while True:
    data = device.get_message()
    if data:
        msg = DeviceMessage(data)
        if not msg.is_valid:
            print("invalid message!")
            continue
        if msg.ID == DeviceMessage.INIT_ID:
            print(msg, "->", bytes(msg.DATA).decode(errors="ignore"))

        # res = device.send_msg(0, data)
        # print("<-", res)
    else:
        time.sleep(0.01)


device.stop_read_loop()
