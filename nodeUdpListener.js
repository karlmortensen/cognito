var PORT = 7777;
var HOST = 'localhost';

var Foscam = require('foscam-client');

var camera = new Foscam({
  username: 'kmort',
  password: 'password,7',
  host: '192.168.42.42',
  port: 88, // default
  protocol: 'http', // default
  rejectUnauthorizedCerts: true // default
});


var dgram = require('dgram');
var server = dgram.createSocket('udp4');
var getPointList = camera.getPTZPresetPointList(); // Get saved presets on the camera
server.on('listening', function () {

	var address = server.address();
    console.log('UDP Server listening on ' + address.address + ":" + address.port);
});

server.on('message', function (message, remote) {
	var pointToUse = getPointList.point4;
	if(message.valueOf() === "A")
	{
		pointToUse = getPointList.point7
	}
	else if(message.valueOf() === "B")
	{
		pointToUse = getPointList.point6
	}
    else if(message.valueOf() === "C")
	{
		pointToUse = getPointList.point5
	}
	else if(message.valueOf() === "D")
	{
		pointToUse = getPointList.point4
	}
    camera.ptzGotoPresetPoint(pointToUse)
    console.log(remote.address + ':' + remote.port +' - ' + message);

});

server.bind(PORT, HOST);
