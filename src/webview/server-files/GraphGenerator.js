var childProcess = require('child_process');
var DotCode = require('./DotCode.js');

/**
 * Replace unsafe tags with HTML escape chars
 * @param  {string} str Input string
 * @return {string}     Escaped string Output
 */
function safeTagsReplace(str) {
    return str.replace(/[&<>]/g, function (tag) {
		var tagsToReplace = {'&': '&amp;', '<': '&lt;', '>': '&gt;'};
	    return tagsToReplace[tag] || tag;
	});
}

var graphGenerator = {
	/**
	 * Generate dot code for a tree
	 * @param  {object} tree        An object containing an nodes and edges arrays
	 * @param  {int} focusedNode    Node id of the node to focus on
	 * @return {string}             Dot code for this tree
	 */
	getDotCodeForTree: function (tree, focusedNode) {
		var nodes = tree.nodes,
			vertices = tree.edges;

		return new DotCode()
			.digraph('G', function(d) {
				d.node({
					shape: 'box',
					fontname: 'Courier New',
					fontsize: '10'
				}).edge({
					fontname: 'Courier New',
					fontsize: '9'
				});

				for (var i = 0; i < nodes.length; i++) {
					d.node('node' + nodes[i].id, {
						shape: 'record', 
						label: safeTagsReplace(nodes[i].label.trim()) + ' | ' + nodes[i].scoreReadable, 
						tooltip: safeTagsReplace(nodes[i].tooltip.trim()),
						style: 'filled',
						color: nodes[i].id == focusedNode ? '#2b2b2b' : 'white',
						penwidth: nodes[i].id == focusedNode ? 3.5 : 1,
						fontcolor: 'white',
						fillcolor: nodes[i].color
					});
				}

				for (var i = 0; i < vertices.length; i++) {
					d.edge('node' + vertices[i].from, 'node' + vertices[i].to, {
						label: '  ' + vertices[i].score,
						color: vertices[i].color,
						penwidth: vertices[i].thickness,
						fontcolor: vertices[i].color
					});
				}
			})
			.toCode();
	},

	/**
	 * Convert dot code to SVG drawing
	 * @param  {string}   dotCode  Dot code for the graph
	 * @param  {Function} callback Call back function which is called with (data,err)
	 */
	convertDotToSvg: function (dotCode, callback) {
		var dot = childProcess.spawn('dot', ['-Tsvg']);
		var svg = '';

		dot.stdout.on('data', function(data, err) {
			svg += data;
		});

		dot.on('error', function(err) {
			callback(null, err);
		});

		dot.on('close', function(code) {
			if(code == 0) {
				callback(svg, null);
			}
		});

		dot.stdin.write(dotCode);
		dot.stdin.end();
	},

	/**
	 * Check if the SVG graph generator is installed
	 * @return {Boolean} True if GraphViz is installed
	 */
	isInstalled: function () {
		try {
			childProcess.execSync('dot -V', {stdio: 'ignore'});
		} catch(err) {
			return false;
		}
		return true;
	}
};

module.exports = graphGenerator;