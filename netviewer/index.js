var d3 = require('d3');

var CSS_FN = 'style.css';

var N_PRECIS = 3;



d3.select('link#stylesheet')
	.attr('href', CSS_FN);

var net = null;

document.getElementById("net-file")
	.addEventListener("change", function(evt) {
		var files = evt.target.files;
		var reader = new FileReader();

		reader.onload = function(evt) {
			var json = JSON.parse(evt.target.result);
			var map_start = ('map' in json) ? json.map : null;
			if (net !== null)
				net.clean_up();
			net = new Network(d3.select('#network'), d3.select('#properties'),
					map_start);
			net.draw_network(json);
			if (net.map !== null) {
				net.map.position_svg();
				net.map.position_table();
			} else {
				net.centre_view();
			}
			};
		reader.readAsText(files[0]);
		}, false);

document.getElementById("dat-file")
	.addEventListener("change", function(evt) {
		var files = evt.target.files;
		var reader = new FileReader();

		reader.onload = function(evt) {
			net.dhand.data = JSON.parse(evt.target.result);
			net.dhand.ind = 0;
			net.set_data();
			};
		reader.readAsText(files[0]);
		}, false);

document.getElementById("save-network").onclick = function() {
	net.save_network(); // have to wrap because starts off null
	};




/**function Optimise(){

	execFile("./PowerTools",['/home/angela/PowerTools/bin','/bin/sh'],(err,stdout,stderr)=> {
		if (err){
			console.error(err);
			return
		}
		console.log(stdout);
	});


}; */
/**function Optimise(){
	var collector = '';
	var execFile = require('child_process');
	var readable = require('stream').readable;
	var path = require('path');
	var collectorPath = path.join(__dirname, '../PowerTools/bin');
	execFile.spawn(collectorPath,[],function(err,stdout,stderr)=> {
		if (err){
			console.error(err);
			return
		}

		//Read the stdout and then push
		stdout.setEncoding('utf8');
		stdout.on('data',(chunk)=>{
			collector += chunk;
		});
		console.log(collector);
	});
};*/
////////using example found/////////////


// execFile: executes a file with the specified arguments
/*	child_process.execFile('ls', ['-lah', '/tmp'], function(error, stdout, stderr){
		console.log(stdout);
	});*/
	/*const execFile = require('child_process').execFile;
	const child = execFile('/home/angela/DEV/PowerTools/bin/PowerTools', ['--version'], (error, stdout, stderr) => {
		if (error) {
			throw error;
		}
		console.log(stdout);
	});*/

	/*	var child_process = require('child_process');
        var path = require('path');
        //var socket = require('socket.io');

    //var socket = io.connect('http://localhost:9966')
        var collector = null;
        if (collector !== null) {
            console.log('Collector is already running.');
            return;}

        /!*var collectorPath = path.join('/home/angela/DEV/PowerTools/bin');
        var collector = child_process.spawn('./PowerTools',collectorPath, {
            stdio:'ignore' }
        );*!/


        collector.on('exit', function (signal, code) {
            console.log('Collector exited with signal: %s and code: %d', signal, code);
            collector = null;

        });*/

function callback(response){
	console.log(response);
}


function httpGetAsync(theUrl,callback)
{

	var xmlHttp = new XMLHttpRequest();
	xmlHttp.onreadystatechange = function() {
		console.log(xmlHttp.status);
		if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
			callback(xmlHttp.responseText);
	};
	xmlHttp.open("GET", theUrl, true);
	xmlHttp.send(null);
}


document.getElementById("executable").onclick = function() {
	httpGetAsync('http://localhost:9004/solve/netviewer', callback);
/*	var child_process = require('child_process');


// exec: spawns a shell.
	child_process.exec('/home/angela/DEV/PowerTools/bin/PowerTools', function(error, stdout, stderr){
		console.log(stdout);
	});*/


};



window.onkeydown = function(e) {
	// Cross browser support
	var key = e.charCode ? e.charCode : e.keyCode ? e.keyCode : 0;
	//console.log(key);
	if (key == 50) { // 2
		net.dhand.decr(1);
		net.set_data();
	} else if (key == 51) { // 3
		net.dhand.incr(1);
		net.set_data();
	} else if (key == 49) { // 1
		net.dhand.decr(20);
		net.set_data();
	} else if (key == 52) { // 4
		net.dhand.incr(20);
		net.set_data();
	}
	};
////////////////////////////////TRIAL///////////////////////////////////////
//backend
/**var child_process = require('child_process');
var path = require('path');
var express = require("express");
var app = express();
var port = 9966; //beefy server port

app.get("/", function(req,res){
	res.send("It works!");
});

app.use(express.static(__dirname + '/public'))

var io = require('socket.io').listen(app.listen(port));

//front-end
window.onload = function() {

	var messages = [];
	var socket = io.connect('http://127.0.0.1:9966'); //http://localhost:3700
	//var field = document.getElementById("field");
	var Optimise = document.getElementById("optimise");
	//var content = document.getElementById("content");

/**	socket.on('message', function (data) {
		if(data.message) {
			messages.push(data.message);
			var html = '';
			for(var i=0; i<messages.length; i++) {
				html += messages[i] + '<br />';
			}
			content.innerHTML = html;
		} else {
			console.log("There is a problem:", data);
		}
	}); */

/**	var collector = null;

	Optimise.onclick = function(req, res) {
		if (collector !== null) {
			res.end('Collector is already running.');
			return;
		}

		var collectorPath = path.join(__dirname, '../PowerTools/bin/PowerTools');
		collector = child_process.spawn(collectorPath, [], {
			stdio: 'ignore'
		});

		collector.on('exit', function () {
			socket.emit('optimise',{ message: 'exit'}); //in place of collector { message: text}
			collector = null;
		});

		socket.emit(res.end('Done.'));
	};


}

io.sockets.on('connection', function (socket) {
	socket.emit('message', { message: 'welcome to the chat' });
	socket.on('send', function (data) {
		io.sockets.emit('message', data);
	});
}); */
////////////////////////////////END TRIAL/////////////////////////////////////////
function DHandler() {
	this.data = null;
	this.ind = 0;
	this.incr = function(val) {
			this.ind = Math.min(this.ind + val, this.data.length - 1);
		};
	this.decr = function(val) {
			this.ind = Math.max(this.ind - val, 0);
		};
	this.get = function() { return this.data[this.ind]; };
}

function Map(cont, map_start) {
	// map zoom is value to set map to after initialisation
	// net zoom is value to initialise map at to give particular network scale
	this.map_zoom = ('map_zoom' in map_start) ? map_start.map_zoom : 16;
	this.net_zoom = ('net_zoom' in map_start) ? map_start.net_zoom : this.map_zoom;
	this.map = L.map(cont.node()).setView([map_start.lat, map_start.lon],
			this.net_zoom);
	this.map.setZoom(this.map_zoom); // now change to actual zoom for user
	L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png', {
		attribution: '&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
		}).addTo(this.map);

	this.overlay = d3.select(this.map.getPanes().overlayPane);
	this.svg = null;
	this.table = null;
	this.table_origin = null;

	this.attach_svg = function(svg) {
		this.svg = svg;
		};

	this.attach_table = function(table) {
		this.table = table;
		this.table_origin = this.map.layerPointToLatLng([0.0, 0.0]);
		};

	this.table_to_geo = function(pos) {
		var scale = this.map.getZoomScale(this.map.getZoom(), this.net_zoom);
		var point = this.map.latLngToContainerPoint(this.table_origin);
		return this.map.containerPointToLatLng([pos[0]*scale + point.x,
				pos[1]*scale + point.y]);
		};

	this.geo_to_table = function(geo) {
		var scale = this.map.getZoomScale(this.map.getZoom(), this.net_zoom);
		var point = this.map.latLngToContainerPoint(this.table_origin);
		var pos = this.map.latLngToContainerPoint(geo);
		pos.x -= point.x;
		pos.y -= point.y;
		pos.x /= scale;
		pos.y /= scale;
		return pos;
		};

	this.position_table = function() {
		var scale = this.map.getZoomScale(this.map.getZoom(), this.net_zoom);
		var point = this.map.latLngToContainerPoint(this.table_origin);
		this.table
			.attr('transform', 'translate(' + point.x + ',' + point.y
						+ ')scale(' + scale + ')');
		};

	this.position_svg = function() {
		var point = this.map.containerPointToLayerPoint([0,0]);
		// Keeping svg locked to window
		this.svg
			.style('left', point.x + 'px')
			.style('top', point.y + 'px');
		};

	this.map.on('dragend', function() {
		this.position_svg();
		this.position_table();
		}, this);

	this.map.on('zoomend', function() {
		this.position_svg();
		this.position_table();
		}, this);

	this.map.on('resize', function() {
		this.position_svg();
		this.position_table();
		}, this);

	this.clean_up = function() {
		this.map.remove();
		};
}

function Network(cont, prop, map_start=null) {
	var that = this;

	this.cont = cont; // viewport container for map/svg with accessible size
	this.wid = cont.node().clientWidth;
	this.hei = cont.node().clientHeight;

	window.onresize = function(evt) {
			that.wid = that.cont.node().clientWidth;
			that.hei = that.cont.node().clientHeight;
			that.svg.attr('width', that.wid)
				.attr('height', that.hei);
		};

	this.prop = prop;

	this.map = null;
	if (map_start !== null)
		this.map = new Map(cont, map_start);
	this.svg_cont = cont;
	if (this.map !== null)
		this.svg_cont = this.map.overlay;
	this.svg = this.svg_cont.append('svg');
	this.svg.attr('width', this.wid)
		.attr('height', this.hei);
	this.table = this.svg.append('g')
		.attr('class', 'table leaflet-zoom-hide');

	if (this.map !== null) {
		this.map.attach_svg(this.svg);
		this.map.attach_table(this.table);
	}

	this.dhand = new DHandler();

	this.defs = this.svg.append('defs');
	this.defs.call(add_markers);

	var node_r = 14;

	var node_drag = d3.behavior.drag()
		.on('dragstart', function(d) {
			d3.event.sourceEvent.stopPropagation();
			})
		.on('drag', function(d) {
			d.x += d3.event.dx;
			d.y += d3.event.dy;
			var edge_set = that.node_cons[d.id].edge;
			var chum_set = that.node_cons[d.id].chum;
			d3.select(this)
				.attr('cx', function(d) { return d.x; })
				.attr('cy', function(d) { return d.y; });
			that.edge.filter(function(d) { return edge_set.has(d.id); })
				.each(update_edge_pos);
			that.chum.filter(function(d) { return chum_set.has(d.id); })
				.attr('cx', function(d) { return that.ndmap.node[d.n0].x +
					that.chum_offset[d.id].xoff; })
				.attr('cy', function(d) { return that.ndmap.node[d.n0].y +
					that.chum_offset[d.id].yoff; })
			})
		.on('dragend', function(d) {
			});

	this.nd = null; // network data (lists form)
	this.ndmap = null; // network data (dicts form)
	this.node_cons = null; // connections for each node
	this.chum_offset = null; // connections for each node

	this.edge = null;
	this.node = null;
	this.chum = null;

	this.save_network = function() {
		// Reattached to event so can't use this, use that instead
		for (var i=0; i<that.nd.node.length; i++)
			that.set_node_geo(that.nd.node[i]);
		var json = JSON.stringify(that.nd);
		var url = 'data:text/json;charset=utf8,' + encodeURIComponent(json);
		//window.open(url, '_blank');
		//window.focus();
		// The above two lines work, but they open in new window.
		// As a workaround create a link, give download attribute and trigger
		// a mouse event on it.
		var link = document.createElement('a');
		link.download = 'network.json';
		link.href = url;
		var click_event = new MouseEvent('click', {
			'view': window,
			'bubbles': true,
			'cancelable': false
			});
		link.dispatchEvent(click_event);


		};


	var update_edge_pos = function(d) {
		var x1 = that.ndmap.node[d.n0].x;
		var y1 = that.ndmap.node[d.n0].y;
		var x2 = that.ndmap.node[d.n1].x;
		var y2 = that.ndmap.node[d.n1].y;
		var dx = 0.0;
		var dy = 0.0;
		if (x2 == x1) {
			dy = node_r;
		} else {
			var m = Math.abs((y2 - y1)/(x2 - x1));
			dx = Math.sqrt(node_r*node_r/(1 + m*m));
			dy = m*dx;
		}
		dx = x1 <= x2 ? dx : -dx;
		dy = y1 <= y2 ? dy : -dy;
		d3.select(this)
			.attr('x1', x1 + dx)
			.attr('y1', y1 + dy)
			.attr('x2', x2 - dx)
			.attr('y2', y2 - dy);
		};

	var update_positions = function() {
		that.node
			.attr('cx', function(d) { return d.x; })
			.attr('cy', function(d) { return d.y; });
		that.chum
			.attr('cx', function(d) { return that.ndmap.node[d.n0].x +
				that.chum_offset[d.id].xoff; })
			.attr('cy', function(d) { return that.ndmap.node[d.n0].y +
				that.chum_offset[d.id].yoff; })
		that.edge
			.each(update_edge_pos);
		};

	var prop_string = function(v) {
		if (Array.isArray(v)) {
			var str = '[';
			for (var i=0; i<v.length; i++) {
				str += prop_string(v[i]);
				if (i < v.length-1) {
					str += ','
				}
			}
			return str + ']';
		} else if (!isNaN(v) && v % 1 !== 0) {
			return v.toPrecision(4);
		} else {
			return v.toString();
		}
		};

	var last_loaded = null;
	var load_prop = function(d) {
		if (d === null)
			return;
		last_loaded = d;
		tprop = that.prop.select('#tprop');
		cprop = that.prop.select('#cprop');
		vprop = that.prop.select('#vprop');
		tprop.html(''); // clear all children
		cprop.html(''); // clear all children
		vprop.html(''); // clear all children
		Object.keys(d)
			.filter(function(k) { return k != 'cprop' && k != 'vprop'; })
			.sort()
			.forEach(function(k) {
				tprop.append('p')
					.attr('class', 'prop-line')
					.text(k + ': ' + prop_string(d[k]));
				});
		if ('cprop' in d) {
			Object.keys(d.cprop)
				.sort()
				.forEach(function(k) {
					cprop.append('p')
						.attr('class', 'prop-line')
						.text(k + ': ' + prop_string(d.cprop[k]));
					});
		}
		if ('vprop' in d) {
			Object.keys(d.vprop)
				.sort()
				.forEach(function(k) {
					vprop.append('p')
						.attr('class', 'prop-line')
						.text(k + ': ' + prop_string(d.vprop[k]));
					});
		}
		};

	var generate_offset = function(mag, i, tot) {
			var ang = Math.PI*(((tot-1)/2.0 - i)/4.0 + 90.0/180.0) ;
			return {"xoff": mag*Math.cos(ang), "yoff": mag*Math.sin(ang)};
		};

	this.set_node_pos = function(n) {
		if (this.map !== null) {
			var point = this.map.geo_to_table([n.lat, n.lon]);
			n.x = point.x;
			n.y = point.y;
		}
		};

	this.set_node_geo = function(n) {
		if (this.map !== null) {
			var geo = this.map.table_to_geo([n.x, n.y]);
			n.lat = geo.lat;
			n.lon = geo.lng;
		}
		};

	this.draw_network = function(nd) {
		this.nd = nd;
		this.ndmap = {};
		this.node_cons = {};
		this.chum_offset = {};
		// Build nodes map
		this.ndmap.node = {};
		for (var i=0; i<nd.node.length; i++) {
			var n = nd.node[i];
			this.set_node_pos(n);
			this.ndmap.node[n.id] = n;
			this.node_cons[n.id] = {'edge': new Set(), 'chum': new Set()};
		}
		// Link edges to their nodes' data
		this.ndmap.edge = {};
		for (var i=0; i<nd.edge.length; i++) {
			var e = nd.edge[i];
			this.ndmap.edge[e.id] = e;
			this.node_cons[e.n0].edge.add(e.id);
			this.node_cons[e.n1].edge.add(e.id);
		}
		// Link chums to their node's data and give offset
		this.ndmap.chum = {};
		for (var i=0; i<nd.chum.length; i++) {
			var c = nd.chum[i];
			this.ndmap.chum[c.id] = c;
			this.chum_offset[c.id] = this.node_cons[c.n0].chum.size; // temp
			this.node_cons[c.n0].chum.add(c.id);
		}
		// Calculate offsets now we know how many per node
		for (var i=0; i<nd.chum.length; i++) {
			var c = nd.chum[i];
			this.chum_offset[c.id] = generate_offset(1.5*node_r,
					this.chum_offset[c.id],
					this.node_cons[c.n0].chum.size);
		}
		// If deco doesn't exist in data, add it
		if (!('deco' in nd))
			nd['deco'] = [];
		this.edge = this.table.selectAll('.edge').data(nd.edge);
		this.edge.enter() // create elements when more data than existing
			.append('line')
			.on('click', load_prop)
			.append('title'); // tooltip
		this.edge
			.attr('class', function(d) { return 'edge ' + d.type; })
			.attr('marker-start', function(d) {
				if ('m0' in d)
					return 'url(#sta_' + d.m0 + ')';
				else
					return 'none';
				})
			.attr('marker-end', function(d) {
				if ('m1' in d)
					return 'url(#end_' + d.m1 + ')';
				else
					return 'none';
				});
		this.edge.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.edge.exit().remove();

		this.node = this.table.selectAll('.node').data(nd.node);
		this.node.enter()
			.append('circle')
			.on('click', load_prop)
			.call(node_drag)
			.append('title'); // tooltip
		this.node
			.attr('class', function(d) { return 'node ' + d.type; })
			.attr('r', node_r);
		this.node.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.node.exit().remove();

		this.chum = this.table.selectAll('.chum').data(nd.chum);
		this.chum.enter()
			.append('circle')
			.on('click', load_prop)
			.append('title'); // tooltip
		this.chum
			.attr('class', function(d) { return 'chum ' + d.type; })
			.attr('r', node_r/2);
		this.chum.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.chum.exit().remove();

		update_positions();
		this.update_deco('cprop');

		};

	this.update_deco = function(prop) {
		for (var i=0; i<this.nd.deco.length; i++) {
			var d = this.nd.deco[i];
			if (d.prop != prop)
				continue;
			switch (d.feat) {
			case 'colour':
				dec_colour(d, this[d.kind]);
				break;
			case 'size':
				dec_size(d, this[d.kind]);
				break;
			default:
				break;
			}
		}
		};

	this.set_data = function() {
			var dat = this.dhand.get();
			var ind = this.dhand.ind
			this.prop.select('#dataindex').text('Index: ' + ind);

			for (var i=0; i<dat.length; i++) {
				var kind = dat[i].kind;
				this.ndmap[kind][dat[i].id].vprop = dat[i].vprop;
			}
			load_prop(last_loaded);
			this.update_deco('vprop');
		};

	// should not be used if map is being used
	this.zoom = d3.behavior.zoom()
		.size([this.wid, this.hei])
		.scaleExtent([0.01, 10])
		.on('zoom', function() {
				that.table.attr('transform', 'translate(' + d3.event.translate +
							')scale(' + d3.event.scale + ')');
			});
	if (this.map === null)
		this.svg.call(this.zoom);

	// should not be used if map is being used
	this.centre_view = function() {
		var n = this.nd.node[0];
		var xmin = n.x;
		var ymin = n.y;
		var xmax = n.x;
		var ymax = n.y;
		for (var i=1; i<this.nd.node.length; i++) {
			n = this.nd.node[i];
			xmin = xmin <= n.x ? xmin : n.x;
			ymin = ymin <= n.y ? ymin : n.y;
			xmax = xmax >= n.x ? xmax : n.x;
			ymax = ymax >= n.y ? ymax : n.y;
		}
		var xmid = (xmin + xmax)/2;
		var ymid = (ymin + ymax)/2;
		var xscale = this.wid/(xmax - xmin);
		var yscale = this.hei/(ymax - ymin);
		var scale = xscale < yscale ? xscale : yscale;
		var xtran = this.wid/2 - scale*xmid;
		var ytran = this.hei/2 - scale*ymid;
		this.zoom.translate([xtran,ytran]).scale(scale);
		this.zoom.event(this.svg); // call event so gets drawn in new position
		};

	// remove everything in container and reset properties
	this.clean_up = function() {
		if (this.map !== null)
			this.map.clean_up();
		this.cont
			.attr('class', ''); // clear any leaflet classes
		var cont = this.cont.node();
		while (cont.firstChild)
			cont.removeChild(cont.firstChild);

		that.prop.select('#tprop').html('');
		that.prop.select('#cprop').html('');
		that.prop.select('#vprop').html('');
		};
}

function add_markers() {
	this.append('marker')
		.attr('id', 'end_closed')
		.attr('viewBox', '0 0 6 6')
		.attr('refX', 6)
		.attr('refY', 3)
		.attr('markerWidth', 1.5)
		.attr('markerheight', 1.5)
		.attr('orient', 'auto')
		.append('rect')
		.attr('x', 0)
		.attr('y', 0)
		.attr('width', 6)
		.attr('height', 6);
	this.append('marker')
		.attr('id', 'sta_closed')
		.attr('viewBox', '0 0 6 6')
		.attr('refX', 0)
		.attr('refY', 3)
		.attr('markerWidth', 1.5)
		.attr('markerheight', 1.5)
		.attr('orient', 'auto')
		.append('rect')
		.attr('x', 0)
		.attr('y', 0)
		.attr('width', 6)
		.attr('height', 6);
	this.append('marker')
		.attr('id', 'end_open')
		.attr('viewBox', '0 0 6 6')
		.attr('refX', 6)
		.attr('refY', 3)
		.attr('markerWidth', 1.5)
		.attr('markerheight', 1.5)
		.attr('orient', 'auto')
		.append('rect')
		.attr('x', 0)
		.attr('y', 0)
		.attr('width', 6)
		.attr('height', 6);
	this.append('marker')
		.attr('id', 'sta_open')
		.attr('viewBox', '0 0 6 6')
		.attr('refX', 0)
		.attr('refY', 3)
		.attr('markerWidth', 1.5)
		.attr('markerheight', 1.5)
		.attr('orient', 'auto')
		.append('rect')
		.attr('x', 0)
		.attr('y', 0)
		.attr('width', 6)
		.attr('height', 6);
}

function dec_colour(deco, sel) {
	var cfunc = null;
	if ('map' in deco) {
		cfunc = function(v) { return v in deco.map ? deco.map[v] : 'blue'; };
	} else {
		var scale = d3.scale.linear()
			.domain('domain' in deco ? deco.domain : [0, 1])
			.range('range' in deco ? deco.range : [0, 270.0])
			.clamp(true);
		cfunc = function(v) { return d3.hsl(scale(v), 0.8, 0.4); };
	}
	sel.filter('.' + deco.type)
		.style(deco.kind == 'edge' ? 'stroke' : 'fill',
			function(d) { return cfunc(d[deco.prop][deco.pname]) });
}

function dec_size(deco, sel) {
	var cfunc = null;
	if ('map' in deco) {
		cfunc = function(v) { return v in deco.map ? deco.map[v] : 8; };
	} else {
		var scale = d3.scale.linear()
			.domain('domain' in deco ? deco.domain : [0, 1])
			.range('range' in deco ? deco.range : [4, 14])
			.clamp(true);
		cfunc = function(v) { return scale(v); };
	}
	if (deco.kind == 'edge') {
		sel.filter('.' + deco.type)
			.style('stroke-width',
				function(d) { return cfunc(d[deco.prop][deco.pname]) });
	} else {
		sel.filter('.' + deco.type)
			.attr('r',
				function(d) { return cfunc(d[deco.prop][deco.pname]) });
	}
}

			//for (var i=0; i<this.dec_inf.length; i++) {
			//	var dc = {};
			//	dc['scale'] = d3.scale.linear();
			//	dc['scale'].domain(this.dec_inf[i]['domain']);
			//	dc['param'] = this.dec_inf[i]['param'];
			//	var par =  this.dec_inf[i]['parent'];
			//	dc['parent'] = par;
			//	var name = par + '_' + this.dec_inf[i]['feat'] + '_' +
			//		this.dec_inf[i]['param'];

			//	switch (this.dec_inf[i]['feat']) {
			//	case 'arc':
			//		dc['func'] = dec_radius;
			//		dc['scale'].range([0, 0.8*node_r]);
			//		var arc = this.table.selectAll('.'+name)
			//			.data(nd[par])
			//			.enter()
			//			.append('path')
			//			.attr('class', name)
			//			.attr('transform', function(d) {
			//					return 'translate(' + that.xt(d.x) + ','
			//							+ that.yt(d.y) + ')';
			//				});
			//		arc.append('title'); // tooltip
			//		dc['obj'] = arc;
			//		break;
			//	case 'text':
			//		dc['func'] = dec_text;
			//		dc['scale'].range([0, 1]);
			//		var txt = this.table.selectAll('.'+name)
			//			.data(nd[par])
			//			.enter()
			//			.append('text')
			//			.attr('class', name)
			//			.attr('x', function(d) {
			//					return that.xt(d.x);
			//				})
			//			.attr('y', function(d) {
			//					return that.yt(d.y);
			//				})
			//			.attr('dx', this.dec_inf[i]['xoff'])
			//			.attr('dy', this.dec_inf[i]['yoff'])
			//			.attr('text-anchor', 'middle');
			//		dc['obj'] = txt;
			//		break;
			//	default:
			//		break;
			//	}
			//	this.dec.push(dc);
			//}
			//
//
//function dec_text(scale, obj, v, dat) {
//	obj.data(dat)
//		.text(function(d) {
//				return scale(d[v].toFixed(N_PRECIS));
//			});
//}

//function plot_data(dat) {
//	var plt1 = new Plot(WIDTH, 200, 0);
//	plt1.addData(dat['resid']['pres'].map(Math.log10), '#885544');
//	plt1.addData(dat['resid']['dres'].map(Math.log10), '#335588');
//	var plt2 = new Plot(WIDTH, 200, 1);
//	plt2.addData(dat['resid']['cost'], '#885544');
//	plt2.addData(dat['resid']['objv'], '#335588');
//}
//
//function Plot(w, h, id) {
//	var x_pad = 50;
//	var y_pad = 25;
//	this.id = id;
//	this.svg = d3.select('body')
//				.append('svg')
//				.attr('width', w)
//				.attr('height', h)
//				.attr('id', id);
//	this.xscale = d3.scale.linear();
//	this.xscale.range([x_pad, w - x_pad]);
//	this.yscale = d3.scale.linear();
//	this.yscale.range([h - y_pad, y_pad]);
//	this.xmax = 0;
//	this.ymin = 0;
//	this.ymax = 0;
//	this.data = [];
//	this.lines = [];
//	this.xaxis = d3.svg.axis()
//			.scale(this.xscale)
//			.orient('bottom')
//			.ticks(5);
//	this.yaxis = d3.svg.axis()
//			.scale(this.yscale)
//			.orient('left')
//			.ticks(5);
//	this.gxaxis = this.svg.append('g')
//			.attr('class', 'axis')
//			.attr('transform', 'translate(0,' + (h - y_pad) + ')')
//			.call(this.xaxis);
//	this.gyaxis = this.svg.append('g')
//			.attr('class', 'axis')
//			.attr('transform', 'translate(' + x_pad + ',0)')
//			.call(this.yaxis);
//	this.addData = function(dat, colour) {
//			if (this.data.length > 0) {
//				this.xmax = Math.max(this.xmax, dat.length);
//				this.ymin = Math.min(this.ymin, Math.min.apply(Math, dat));
//				this.ymax = Math.max(this.ymax, Math.max.apply(Math, dat));
//			} else {
//				this.xmax = dat.length;
//				this.ymin = Math.min.apply(Math, dat);
//				this.ymax = Math.max.apply(Math, dat);
//			}
//			//console.log(this.xmax + ' ' + this.ymin + ' ' + this.ymax);
//			this.xscale.domain([0, this.xmax]);
//			this.yscale.domain([this.ymin, this.ymax]);
//			var that = this;
//			var lineFunc = d3.svg.line()
//					.x(function(d, i) { return that.xscale(i); })
//					.y(function(d) { return that.yscale(d); })
//					.interpolate('linear');
//			var lineFuncZero = d3.svg.line()
//					.x(function(d, i) { return that.xscale(i); })
//					.y(function(d) { return that.yscale(that.ymin); })
//					.interpolate('linear');
//			var line = this.svg.append('svg:path')
//					.attr('stroke', colour)
//					.attr('stroke-width', 2)
//					.attr('fill', 'none')
//					.attr('d', lineFuncZero(dat));
//			this.data.push(dat);
//			this.lines.push(line);
//			for (var i=0; i<this.data.length; i++) {
//				this.lines[i].transition()
//					.delay(1000)
//					.duration(750)
//					.attr('d', lineFunc(this.data[i]));
//			}
//
//			this.gxaxis.call(this.xaxis);
//			this.gyaxis.call(this.yaxis);
//		};
//}
//
