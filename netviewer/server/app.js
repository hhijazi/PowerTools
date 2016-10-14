/**
 * Created by angela on 22/08/16.
 */
    //Notes
    // Goal: send a get request to the server (port 9004)
    // the server listens and runs the executable
var child_process = require('child_process');
var path = require('path');
var express = require('express');
var app = express();


var collector = null;

//when GET request is made to http://localhost:3000/solve/netviewer, execute following
app.get('/solve/netviewer', function (req, res) {
    //var networkname = req.params.networkname;
    res.setHeader('Access-Control-Allow-Origin','*');
    res.setHeader('Content-Type','application/json');
    //res.json({"hello": 5});
    if (collector !== null) {
        res.end('Collector is already running.');
        return;
    }

    var collectorPath = path.join('/home/angela/DEV/PowerTools/bin/PowerTools');
    collector = child_process.exec(collectorPath,'/bin/sh');

    collector.stdout.on('data', function(data) {
        console.log('stdout: ' + data);
    });

    collector.stderr.on('data', function(data) {
        console.log('stdout: ' + data);
    });

    collector.on('exit', function (signal, code) {
        console.log('Collector exited with signal: %s and code: %d', signal, code);
        //console.log('data: %d',collector);
        collector = null;
    });

    res.end('Done.');
});

app.listen(9004, function(){
    console.log('listening on port http://localhost:9004/solve/netviewer')
});

//The app starts a server and listens on port 9004 for connections. The app responds
//by running the executable for request to '/solve/:networkname?'
