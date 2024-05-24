import serial.tools.list_ports
import time
timestr = time.strftime("%Y_%m_%d")
print("########################################")
print("#      RFID Balance Keeper Logging     #")
print("########################################\n")
print("Benutzte Ports:")
portList = []

ports = serial.tools.list_ports.comports()
serialInst = serial.Serial()

for onePort in ports:
    portList.append(str(onePort))
print(onePort)

val = input("\nCOM Port auswählen: ")

for x in range(0, len(portList)):
    if portList[x].startswith("COM" + str(val)):
        portVar = "COM" + str(val)
        print(portVar)

serialInst.baudrate = 9600
serialInst.port = portVar
serialInst.open()

logfile = "Einnahmen_"+timestr+".txt"

with open(logfile, 'w') as file:
    try:
        while True:
            if serialInst.in_waiting:
                packet = serialInst.readline()
                data = packet.decode("utf-8").rstrip('\n')
                print(data)
                file.write(data)
                file.flush()  # Sicherstellen, dass die Daten sofort geschrieben werden
    except KeyboardInterrupt:
        print("Logging beendet.")
    except Exception as e:
        print(f"Fehler: {e}")
    finally:
        serialInst.close()
