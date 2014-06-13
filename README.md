Module to do automatic simulations and analysis of Minecraft Industrial Craft 2 nuclear reactors.  The mechanics simulated are based on the decompiled code of `IC2 2_2.0.397-EXPERIMENTAL` .  Many of the simulations performed by this simulator are more accurate than existing simulators, due to this being based on the decompiled code itself (albeit optimized).

```javascript
var reactorsim = require('ic2-reactor-sim');
var reactor = [
	'VV', 'XX', 'XX',
	'U1', 'VV', 'XX',
	'U1', 'VV', 'XX',
	'VV', 'XX', 'XX',
	'XX', 'XX', 'XX',
	'XX', 'XX', 'XX'
];
reactorsim.runSimulation(reactor, function(error, simulationResults) {
	if(error) console.log(error);
	else console.log(simulationResults);
});
```

Reactors are specified with a 1-dimensional array, in row-major order (see example above) of components.  The size of the reactor is automatically determined by the length of the array.

The component codes are:
- XX: Empty
- VV: Heat Vent
- VR: Reactor Heat Vent
- VA: Advanced Heat Vent
- VC: Component Heat Vent
- VO: Overclocked Heat Vent
- EE: Heat Exchanger
- EA: Advanced Heat Exchanger
- ER: Reactor Heat Exchanger
- EC: Component Heat Exchanger
- C1: 10k Coolant Cell
- C3: 30k Coolant Cell
- C6: 60k Coolant Cell
- CR: RSH Condensator
- CL: LZH Condensator
- U1: Uranium Cell
- U2: Dual Uranium Cell
- U4: Quad Uranium Cell
- NN: Neutron Reflector
- NT: Thick Neutron Reflector
- PP: Reactor Plating
- PC: Containment Reactor Plating
- PH: Heat Capacity Reactor Plating

When running many simulations in sequence, I recommend setting the environment variable `UV_THREADPOOL_SIZE` to at least the number of cores in the system, to take better advantage of parallel processing.


