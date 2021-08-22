import sys
import nglogger

print ( nglogger.version() )


filename=str(sys.argv[1])
#"/var/xprojector/logs/xdb-main.nglog"

print("getting logfile: ", filename)
logfile = nglogger.open(filename)
print("getting data")

while 1:
	data = logfile.read()
	if data is not  None:
		print("MESSAGE:", data.payload, "CHECKSUM OK:", data.checksumok)
	else:
		break
