//var reactorsim = require('./build/Release/nodereactorsim.node');
var reactorsim = require('bindings')('nodereactorsim.node');

exports.runSimulation = reactorsim.runSimulation;

exports.getDimensions = function(numExtraChambers) {
	return {
		width: 3 + numExtraChambers,
		height: 6
	};
};

exports.allComponents = [
	'XX',
	'VV', 'VR', 'VA', 'VC', 'VO',
	'EE', 'EA', 'ER', 'EC',
	'C1', 'C3', 'C6',
	'CR', 'CL',
	'U1', 'U2', 'U4',
	'NN', 'NT',
	'PP', 'PC', 'PH'
];
