# Network Viewer
Network Viewer is a tool for visualising static and dynamic network data (e.g., for electricity networks).  This data can optionally be overlayed onto a map if geographical coordinates are supplied.

The easiest way to try it is to download one of the small example networks in the `examples/` source code directly, and then open it with the hosted version: [Network Viewer](https://consort.gitlab.io/network-viewer/).

## Local Installation
If a local version is desired, then clone the source and open the index.html file from your modern browser of choice (tested on firefox and chromium).

## Development Installation
If developing then working copies of npm and [node](http://nodejs.org) are required.  Clone the source and install network-viewer via npm:
```Shell
$ cd network-viewer
$ npm install
```

Before viewing the results or making a commit, any changes to `index.js` need to be bundled into `bundle.js` using [Browserify](http://browserify.org/):
```Shell
$ npm install -g browserify
$ browserify index.js -o bundle.js
```

Alternatively, the mini webserver [beefy](http://didact.us/beefy/) can be used to automate changes on page reload (the manual approach is still required before committing):
```Shell
# npm -g install beefy browserify
$ beefy index.js:bundle.js
```

## Usage
Users open a json file describing the network and optionally a json file containing variable data. Examples of these formats are provided under `examples/`.

Panning, zooming and node repositioning features are provided along with an option to save the network layout.  If supplied, variable data can be stepped through by pressing the 2 (backward) and 3 (forward) keys, with 1 and 4 used for larger steps.

## External Links
http://d3js.org/

http://nodejs.org/

http://didact.us/beefy/

http://browserify.org/
