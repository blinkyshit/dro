#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import random
import os
import threading
import cherrypy
import time
import serial

from ws4py.server.cherrypyserver import WebSocketPlugin, WebSocketTool
from ws4py.websocket import WebSocket
from ws4py.messaging import TextMessage

HOST = '127.0.0.1'
PORT = 9000
COM_PORT = "COM3"

PAGE = '''<html>
<head>
  <title>DRO</title>
  <style type="text/css">
    body{
	text-align: center;	
	background-color: whitesmoke;
	display:inline;
    }
    div.outaxis{ 	
	margin:auto;
	text-align:top;
	margin-bottom: 20px;
	font-size: 4em;
    }
    span.inaxis{ 	
	margin:auto;
	padding-left:30px;
	padding-right:30px;
	margin-right: 10px;
	border: 3px gray solid; 
	background:black;
	text-align: top;
	color: yellow;
	font-size: 3em;
    }
    div.button{
        border:2px black solid;
		  background: lightgrey;         
        float: right;
        margin:auto;height: 1.5em;width: 2em;
        text-align: center;
        vertical-align: center;
    }
    a.zero:link,a.zero:visited{font-family:Corbel;font-size:1em;font-weight: normal;color:black;text-decoration: none;}
    a.zero:hover,a.zero:active{font-family:Corbel;font-size:1em;font-weight: normal;color:lightgrey;text-decoration: none;}
    a.conv:link,a.conv:visited{font-family:Corbel;font-size:1em;font-weight: normal;color:black;text-decoration: none;}
    a.conv:hover,a.conv:active{font-family:Corbel;font-size:1em;font-weight: normal;color:lightgrey;text-decoration: none;}
    </style>
</head>
<body>
<div class="outaxis"><a class="zero" href="" >x axis: </a><span class="inaxis" id="data0"></span><a id="z0" class="conv" href="" >in</a></br></div>
<div class="outaxis"><a class="zero" href="" >y axis: </a><span class="inaxis" id="data1"></span><a id="z1" class="conv" href="" >in</a></br></div>
<div class="outaxis"><a class="zero" href="" >z axis: </a><span class="inaxis" id="data2"></span><a id="z2" class="conv" href="" >in</a></div>
</body>
<script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js"></script>
<script type="text/javascript">

var socket;

$(document).ready(function() 
{
    socket = new WebSocket('ws://%s:%d/ws');
    socket.onmessage = function (e) { onreceive(e) };
    socket.onopen = function (e) { onopen(e) };
});
var el = document.getElementById('z0');
el.onclick = zero;
var el = document.getElementById('z1');
el.onclick = zero;
var el = document.getElementById('z2');
el.onclick = zero;

function zero() {
  if ($("#z0").text() === 'in'){
    $("#z0").text('mm');
    $("#z1").text('mm');
    $("#z2").text('mm');
  }
  else if ($("#z0").text() === 'mm'){
    $("#z0").text('in');
    $("#z1").text('in');
    $("#z2").text('in');
  }	  
  return false;
}
function onopen(e)
{
}
function onreceive(e)
{
    var data = e.data.split(";");

    var val0_tick = parseInt(data[0]);
    val0_mm = val0_tick / 200; //convert ticks to mm
    val0 = val0_mm / 25.4; //convert to inch

    var val1_tick = parseInt(data[1]);
    val1_mm = val1_tick / 200; //convert ticks to mm
    val1 = val1_mm / 25.4; //convert to inch

    var val2_tick = parseInt(data[2]);
    val2_mm = val2_tick / 200; //convert ticks to mm
    val2 = val2_mm / 25.4; //convert to inch

    if ($("#z0").text() === 'mm'){
  	$("#data0").text((val0*25.4).toFixed(2));
  	$("#data1").text((val1*25.4).toFixed(2));
  	$("#data2").text((val2*25.4).toFixed(2));
    }
    else if ($("#z0").text() === 'in'){
        $("#data0").text(val0.toFixed(4));
        $("#data1").text(val1.toFixed(4));
        $("#data2").text(val2.toFixed(4));
    }       
}

function onclose(e)
{
    alert("Server has closed the connection. Bugger!");
}

</script>
</html>
''';

class DataSource(threading.Thread):
    def run(self):        
        client = self._Thread__kwargs['client']
        ser = serial.Serial(COM_PORT,9600)
        print "opened '%s'" % COM_PORT
        client.send("hello world!\n")

        buffer = ''
        #last_received = ''
        
        while True:
            buffer = buffer + ser.read(ser.inWaiting())
            if '\n' in buffer:
                lines = buffer.split('\n') # Guaranteed to have at least 2 entries
                last_received = lines[-2]
                #If the Arduino sends lots of empty lines, you'll lose the
                #last filled line, so you could make the above statement conditional
                #like so: if lines[-2]: last_received = lines[-2]
                buffer = lines[-1]
                client.send(last_received)
                    
            #val = ser.readline().decode('ascii').strip()
            #client.send(val + "<br>\n")

        client.send("We're done now!")
        ser.close()
    
class DataSourceWebSocketHandler(WebSocket):
    def opened(self):
        self.data_source = DataSource(kwargs={'client': self})
        self.data_source.start()

    def received_message(self, m):
        print "received", m

    def closed(self, code, reason="A client left the room without a proper explanation."):
        print "socket closed"
        self.data_source.exit()
        # call to the thread and tell it to exit (serial_thread.exit())

class Root(object):
    def __init__(self, host, port, ssl=False):
        self.host = host
        self.port = port
        self.scheme = 'wss' if ssl else 'ws'

    @cherrypy.expose
    def index(self):
        return PAGE % (HOST, PORT);

    @cherrypy.expose
    def ws(self):
        cherrypy.log("Handler created: %s" % repr(cherrypy.request.ws_handler))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Data Display Server')
    parser.add_argument('--com', default=COM_PORT)
    parser.add_argument('--host', default=HOST)
    parser.add_argument('-p', '--port', default=PORT, type=int)
    args = parser.parse_args()

    HOST = args.host
    PORT = args.port
    COM_PORT = args.com

    cherrypy.config.update({'server.socket_host': args.host,
                            'server.socket_port': args.port})

    WebSocketPlugin(cherrypy.engine).subscribe()
    cherrypy.tools.websocket = WebSocketTool()

    cherrypy.quickstart(Root(args.host, args.port), '', config={
        '/ws': {
            'tools.websocket.on': True,
            'tools.websocket.handler_cls': DataSourceWebSocketHandler
            }
        }
    )
