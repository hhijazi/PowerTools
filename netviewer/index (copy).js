var d3 = require('d3');

var CSS_FN = 'style.css';

var N_PRECIS = 3;

d3.select('link#stylesheet')
	.attr('href', CSS_FN);

//new addition
var map = L.map('network').setView([-35.28113, 149.11911], 20); //coordinates for ANU : -35.28113,149.11911..([37.8, -96.9], 4);
L.tileLayer('http://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
	attribution: 'Map data &copy; <a href="http://openstreetmap.org">OpenStreetMap</a> contributors, <a href="http://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>',
	maxZoom: 20,
}).addTo(map);

//end of new addition

var net = new Network(d3.select(map.getPanes().overlayPane).append('svg'),d3.select('#properties'));
//how do I include network?
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

var dhand = new DHandler();

window.onkeydown = function(e) {
		// Cross browser support
		var key = e.charCode ? e.charCode : e.keyCode ? e.keyCode : 0;
		//console.log(key);
		if (key == 50) { // 2
			dhand.decr(1);
			net.set_data(dhand.get(), dhand.ind);
		} else if (key == 51) { // 3
			dhand.incr(1);
			net.set_data(dhand.get(), dhand.ind);
		} else if (key == 49) { // 1
			dhand.decr(20);
			net.set_data(dhand.get(), dhand.ind);
		} else if (key == 52) { // 4
			dhand.incr(20);
			net.set_data(dhand.get(), dhand.ind);
		}
	};

document.getElementById("net-file")
	.addEventListener("change", function(evt) {
		var files = evt.target.files;
		var reader = new FileReader();

		reader.onload = function(evt) {
			//networkLoaded(JSON.parse(evt.target.result));
			net.draw_network(JSON.parse(evt.target.result));
			net.centre_view();
			};
		reader.readAsText(files[0]);
		}, false);

document.getElementById("dat-file")
	.addEventListener("change", function(evt) {
		var files = evt.target.files;
		var reader = new FileReader();

		reader.onload = function(evt) {
			dhand.data = JSON.parse(evt.target.result);
			dhand.ind = 0;
			net.set_data(dhand.get(), dhand.ind);
			};
		reader.readAsText(files[0]);
		}, false);

document.getElementById("save-network").onclick = net.save_network;

function Network(svg, prop) {
	var that = this;

	var cont = svg.node().parentNode;
	this.wid = cont.clientWidth;
	this.hei = cont.clientHeight;
	this.svg = svg;
	this.svg.attr('width', this.wid)
		.attr('height', this.hei);
	this.table = svg.append('g')
		.attr('class', 'table leaflet-zoom-hide') ; //added leaflet-zoom-hide
	window.onresize = function(evt) {
			that.wid = cont.clientWidth;
			that.hei = cont.clientHeight;
			that.svg.attr('width', that.wid)
				.attr('height', that.hei);
		};

	var node_r = 15;

	this.defs = this.svg.append('defs');
	this.defs.call(add_markers);

	var zoom = d3.behavior.zoom()
		.size([this.wid, this.hei])
		.scaleExtent([0.01, 10])
		.on('zoom', function() {
				that.table.attr('transform', 'translate(' + d3.event.translate +
							')scale(' + d3.event.scale + ')');
			});
	this.svg.call(zoom);

	var node_drag = d3.behavior.drag()
		.on('dragstart', function(d) {
			d3.event.sourceEvent.stopPropagation();
			})
		.on('drag', function(d) {
			d.x += d3.event.dx;
			d.y += d3.event.dy;
			var edge_set = that.node_cons[d.id].edges;
			var deco_set = that.node_cons[d.id].decos;
			d3.select(this)
				.attr('cx', function(d) { return d.x; })
				.attr('cy', function(d) { return d.y; });
			that.edges.filter(function(d) { return edge_set.has(d.id); })
				.each(update_edge_pos);
			that.decos.filter(function(d) { return deco_set.has(d.id); })
				.attr('cx', function(d) { return that.ndmap.nodes[d.n0].x + d.xoff; })
				.attr('cy', function(d) { return that.ndmap.nodes[d.n0].y + d.yoff; });
			})
		.on('dragend', function(d) {
			});

	this.nd = null; // network data (lists form)
	this.ndmap = null; // network data (dicts form)
	this.node_cons = null; // connections for each node

	this.edges = null;
	this.nodes = null;
	this.decos = null;

	this.save_network = function() {
		// Reattached to event so can't use this, use that instead
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
		var x1 = that.ndmap.nodes[d.n0].x;
		var y1 = that.ndmap.nodes[d.n0].y;
		var x2 = that.ndmap.nodes[d.n1].x;
		var y2 = that.ndmap.nodes[d.n1].y;
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
		that.nodes
			.attr('cx', function(d) { return d.x; })
			.attr('cy', function(d) { return d.y; });
		that.decos
			.attr('cx', function(d) { return that.ndmap.nodes[d.n0].x + d.xoff; })
			.attr('cy', function(d) { return that.ndmap.nodes[d.n0].y + d.yoff; });
		that.edges
			.each(update_edge_pos);
		};

	var last_loaded = null;
	var load_prop = function(d) {
		if (d == null)
			return;
		last_loaded = d;
		tprop = prop.select('#tprop');
		cprop = prop.select('#cprop');
		vprop = prop.select('#vprop');
		tprop.html(''); // clear all children
		cprop.html(''); // clear all children
		vprop.html(''); // clear all children
		for (var k in d) {
			if (k != 'cprop') {
				tprop.append('p')
					.attr('class', 'prop-line')
					.text(k + ': ' + d[k]);
			}
		}
		for (var k in d.cprop) {
			cprop.append('p')
				.attr('class', 'prop-line')
				.text(k + ': ' + d.cprop[k]);
		}
		for (var k in d.vprop) {
			vprop.append('p')
				.attr('class', 'prop-line')
				.text(k + ': ' + d.vprop[k]);
		}
		};

	this.draw_network = function(nd) {
		this.nd = nd;
		this.ndmap = {};
		this.node_cons = {};
		// Build nodes map
		this.ndmap.nodes = {};
		for (var i=0; i<nd.nodes.length; i++) {
			this.ndmap.nodes[nd.nodes[i].id] = nd.nodes[i];
			this.node_cons[nd.nodes[i].id] = {'edges': new Set(), 'decos': new Set()};
		}
		// Link edges to their nodes' data
		this.ndmap.edges = {};
		for (var i=0; i<nd.edges.length; i++) {
			this.ndmap.edges[nd.edges[i].id] = nd.edges[i];
			this.node_cons[nd.edges[i].n0].edges.add(nd.edges[i].id);
			this.node_cons[nd.edges[i].n1].edges.add(nd.edges[i].id);
		}
		// Link decos to their node's data and give offset
		this.ndmap.decos = {};
		for (var i=0; i<nd.decos.length; i++) {
			this.ndmap.decos[nd.decos[i].id] = nd.decos[i];
			this.node_cons[nd.decos[i].n0].decos.add(nd.decos[i].id);
			switch (nd.decos[i].type) {
			case 'load':
				nd.decos[i].xoff = 0.0;
				nd.decos[i].yoff = 1.5*node_r;
				break;
			case 'generator':
			case 'feed':
				nd.decos[i].xoff = 0.0;
				nd.decos[i].yoff = -1.5*node_r;
				break;
			default:
				nd.decos[i].xoff = 0.0;
				nd.decos[i].yoff = 0.0;
			}
		}
		this.edges = this.table.selectAll('.edge').data(nd.edges);
		this.edges.enter() // create elements when more data than existing
			.append('line')
			.on('click', load_prop)
			.append('title'); // tooltip
		this.edges
			.attr('class', function(d) { return 'edge ' + d.type; })
			.attr('marker-start', function(d) {
				if ('n0_sw' in d)
					return d.n0_sw == 0 ? 'url(#sta_open)' : 'url(#sta_closed)';
				else
					return 'none';
				})
			.attr('marker-end', function(d) {
				if ('n1_sw' in d)
					return d.n1_sw == 0 ? 'url(#end_open)' : 'url(#end_closed)';
				else
					return 'none';
				});
		this.edges.filter(function(d) { return d.type == 'line';})
			.style('stroke', function(d) { return phase_color(d.phase) });
		this.edges.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.edges.exit().remove();

		this.nodes = this.table.selectAll('.node').data(nd.nodes);
		this.nodes.enter()
			.append('circle')
			.on('click', load_prop)
			.call(node_drag)
			.append('title'); // tooltip
		this.nodes
			.attr('class', function(d) { return 'node ' + d.type; })
			.attr('r', node_r);
		this.nodes.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.nodes.exit().remove();

		this.decos = this.table.selectAll('.deco').data(nd.decos);
		this.decos.enter()
			.append('circle')
			.on('click', load_prop)
			.append('title'); // tooltip
		this.decos
			.attr('class', function(d) { return 'deco ' + d.type; })
			.attr('r', node_r/2);
		this.decos.filter(function(d) { return d.type == 'load';})
			.style('fill', function(d) { return phase_color(d.phase) });
		this.decos.select('title')
			.attr('name', function(d) { return d.name; })
			.text(function(d) { return d.name; });
		this.decos.exit().remove();

		update_positions();

		};

	this.centre_view = function() {
		var n = this.nd.nodes[0];
		var xmin = n.x;
		var ymin = n.y;
		var xmax = n.x;
		var ymax = n.y;
		for (var i=1; i<this.nd.nodes.length; i++) {
			n = this.nd.nodes[i];
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
		zoom.translate([xtran,ytran]).scale(scale);
		zoom.event(this.svg); // call event so gets drawn in new position
		};

	this.set_data = function(dat, ind) {
			prop.select('#dataindex').text('Index: ' + ind);

			for (var i=0; i<dat.length; i++) {
				var type = dat[i].elem;
				this.ndmap[type][dat[i].id].vprop = dat[i].vprop;
			}
			load_prop(last_loaded);

			// Should make use of ind to indicate where in dataset we are
			//for (var i=0; i<this.dec.length; i++) {
			//	apply_dec(this.dec[i], dat);
			//}
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

function phase_color(phase) {
	switch (phase) {
	case 4:
		return '#FF0F57';
	case 5:
		return '#0F7BFF';
	case 6:
		return '#93FF0F';
	case 7:
		return 'gray';
	default:
		return 'black';
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
			//	case 'colour':
			//		dc['func'] = dec_colour;
			//		dc['scale'].range([0, 270.0]);
			//		dc['obj'] = this.ele[par];
			//		break;
			//	case 'width':
			//		dc['func'] = dec_width;
			//		dc['scale'].range([2, 1.8*node_r]);
			//		dc['obj'] = this.ele[par];
			//		break;
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
// Decorator functions
//function dec_colour(scale, obj, v, dat) {
//	obj.data(dat)
//		.style('fill', function(d) { return d3.hsl(scale(d[v]), 0.8, 0.4); });
//	obj.select('title')
//		.text(function(d) {
//				return d3.select(this).attr('name') + '\n' + v + ' ' + d[v];
//			});
//}
//
//function dec_width(scale, obj, v, dat) {
//	obj.data(dat)
//		.style('stroke-width', function(d) {
//				return scale(Math.abs(d[v]));
//			});
//	obj.select('title')
//		.text(function(d) {
//				return d3.select(this).attr('name') + '\n' + v + ' ' + d[v];
//			});
//}
//
//function dec_radius(scale, obj, v, dat) {
//	var arc = d3.svg.arc()
//		.innerRadius(0)
//		.outerRadius(function(d) {
//				var r = Math.abs(scale(d[v]));
//				return r < 0.0 ? 0.0 : r;
//			})
//		.startAngle(function (d) {
//				return d[v] < 0.0 ? Math.PI/2 : -Math.PI/2; // Radians
//			})
//		.endAngle(function (d) {
//				return d[v] < 0.0 ? 3*Math.PI/2 : Math.PI/2; // Radians
//			});
//
//	obj.data(dat)
//		.attr('d', arc);
//
//	obj.select('title')
//		.text(function(d) {
//				return d3.select(this).attr('name') + '\n' + v + ' ' + d[v];
//			});
//}
//
//function dec_text(scale, obj, v, dat) {
//	obj.data(dat)
//		.text(function(d) {
//				return scale(d[v].toFixed(N_PRECIS));
//			});
//}
//
//function apply_dec(dec, dat) {
//	dec['func'](dec['scale'], dec['obj'], dec['param'], dat[dec['parent']]);
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
