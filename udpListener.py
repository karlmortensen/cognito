import socket
from subprocess import call

UDP_IP="192.168.42.217"
UDP_PORT = 5555
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

while True:
	data, addr = sock.recvfrom(1024)
	print "got message:", data
	call(["node", "slewCameraTo.js", data])
