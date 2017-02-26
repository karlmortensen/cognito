var Foscam = require('foscam-client');

var camera = new Foscam({
  username: 'kmort',
  password: 'password,7',
  host: '192.168.42.42',
  port: 88, // default
  protocol: 'http', // default
  rejectUnauthorizedCerts: true // default
});

// console.log('Slewing camera to point ' + process.argv[2]);
var setpoint = process.argv[2];
var getPointList = camera.getPTZPresetPointList(); // Get saved presets on the camera
var gotoPresetPoint = getPointList.then(function(pointList)
{
	// console.log('setpoint = '+setpoint);		
	var pointToUse = pointList.point4;
	if(setpoint.valueOf() === "A")
	{
		pointToUse = pointList.point7
	}
	else if(setpoint.valueOf() === "B")
	{
		pointToUse = pointList.point6
	}
    else if(setpoint.valueOf() === "C")
	{
		pointToUse = pointList.point5
	}
	else if(setpoint.valueOf() === "D")
	{
		pointToUse = pointList.point4
	}
    // console.log('Using point: ' + pointToUse);
    // console.log('PTZ Point List:' + JSON.stringify(pointList));
    // Note that ptzGotoPresetPoint's promise is fulfilled as soon as the command is received by the camera. There is no way to block until the camera has finished moving to the location.
    return camera.ptzGotoPresetPoint(pointToUse)
});

// Once the ptzGotoPresetPoint command is finished - snap a picture
//var snapPicture = gotoPresetPoint.then(function()
//{
//    return camera.snapPicture2()
//});
//
// Once the picture is snapped - do something with it
//snapPicture.then(function(binaryJpg)
//{
//    // do something with the binary jpg
//});