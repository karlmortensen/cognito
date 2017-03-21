var PORT = 7777;
var HOST = 'localhost';

var Foscam = require('foscam-client');

var camera = new Foscam({
  username: 'kmort',
  password: 'password,7',
  host: '192.168.42.110',
  port: 88, // default
  protocol: 'http', // default
  rejectUnauthorizedCerts: true // default
});

var dgram = require('dgram');
var server = dgram.createSocket('udp4');
camera.getPTZPresetPointList().then(function(getPointList)
{
	var A = new Buffer("A");
	var B = new Buffer("B");
	var C = new Buffer("C");
	var D = new Buffer("D");
	server.on('listening', function () 
	{
		var address = server.address();
		console.log('UDP Server listening on ' + address.address + ":" + address.port);
	});

	server.on('message', function (message, remote)
	{
		var pointToUse = getPointList.point4;
		if(message.readUInt8(0) == A.readUInt8(0))
		{
			pointToUse = getPointList.point7
		}
		else if(message.readUInt8(0) === B.readUInt8(0))
		{
			pointToUse = getPointList.point6
		}
		else if(message.readUInt8(0) === C.readUInt8(0))
		{
			pointToUse = getPointList.point5
		}
		else if(message.readUInt8(0) === D.readUInt8(0))
		{
			pointToUse = getPointList.point4
		}
		else
		{
			console.log('Oops! Running nothing');
		}
		camera.ptzGotoPresetPoint(pointToUse)
        //console.log(remote.address + ':' + remote.port +' - ' + message);
        console.log('Slewing to ' + message);
	});

server.bind(PORT, HOST);

});
