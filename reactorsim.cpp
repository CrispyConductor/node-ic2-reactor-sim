#include "reactorsim.hpp"
#include <iostream>
#include <typeinfo>
#include "gridio.hpp"

namespace reactorsim {

using std::shared_ptr;
using std::cout;
using std::endl;

/***** Heatable *****/

// Returns the REMAINING heat that was not able to be added/removed
int Heatable::alterHeat(int heat) {
	int newHeat = pendingHeat;
	newHeat += heat;
	if(newHeat > maxHeat) {
		setDestroyed(true);
		heat = maxHeat - newHeat + 1;
	} else {
		if(newHeat < 0) {
			heat = newHeat;
			newHeat = 0;
		} else {
			heat = 0;
		}
		pendingHeat = newHeat;
	}
	return heat;
}


/***** ReactorComponent *****/


shared_ptr<ReactorComponent> ReactorComponent::create(ComponentType type, Reactor* reactor, int x, int y) {
	ReactorComponent* ptr;
	switch(type) {
		case COMPONENT_NONE: ptr = 0; break;

		case HEAT_VENT: ptr = new HeatVent(HEAT_VENT, reactor, x, y, 6, 0, 1000); break;
		case REACTOR_HEAT_VENT: ptr = new HeatVent(REACTOR_HEAT_VENT, reactor, x, y, 5, 5, 1000); break;
		case ADVANCED_HEAT_VENT: ptr = new HeatVent(ADVANCED_HEAT_VENT, reactor, x, y, 12, 0, 1000); break;
		case OVERCLOCKED_HEAT_VENT: ptr = new HeatVent(OVERCLOCKED_HEAT_VENT, reactor, x, y, 20, 36, 1000); break;

		case COMPONENT_HEAT_VENT: ptr = new ComponentHeatVent(COMPONENT_HEAT_VENT, reactor, x, y, 4); break;

		case HEAT_EXCHANGER: ptr = new HeatExchanger(HEAT_EXCHANGER, reactor, x, y, 12, 4, 2500); break;
		case ADVANCED_HEAT_EXCHANGER: ptr = new HeatExchanger(ADVANCED_HEAT_EXCHANGER, reactor, x, y, 24, 8, 5000); break;
		case CORE_HEAT_EXCHANGER: ptr = new HeatExchanger(CORE_HEAT_EXCHANGER, reactor, x, y, 0, 72, 2500); break;
		case COMPONENT_HEAT_EXCHANGER: ptr = new HeatExchanger(COMPONENT_HEAT_EXCHANGER, reactor, x, y, 36, 0, 5000); break;

		case COOLANT_CELL_10: ptr = new CoolantCell(COOLANT_CELL_10, reactor, x, y, 10000); break;
		case COOLANT_CELL_30: ptr = new CoolantCell(COOLANT_CELL_30, reactor, x, y, 30000); break;
		case COOLANT_CELL_60: ptr = new CoolantCell(COOLANT_CELL_60, reactor, x, y, 60000); break;

		case CONDENSATOR_RSH: ptr = new Condensator(CONDENSATOR_RSH, reactor, x, y, 20000); break;
		case CONDENSATOR_LZH: ptr = new Condensator(CONDENSATOR_LZH, reactor, x, y, 100000); break;

		case URANIUM_CELL: ptr = new UraniumCell(URANIUM_CELL, reactor, x, y, 1); break;
		case DUAL_URANIUM_CELL: ptr = new UraniumCell(DUAL_URANIUM_CELL, reactor, x, y, 2); break;
		case QUAD_URANIUM_CELL: ptr = new UraniumCell(QUAD_URANIUM_CELL, reactor, x, y, 4); break;

		case NEUTRON_REFLECTOR: ptr = new NeutronReflector(NEUTRON_REFLECTOR, reactor, x, y, 10000); break;
		case THICK_NEUTRON_REFLECTOR: ptr = new NeutronReflector(THICK_NEUTRON_REFLECTOR, reactor, x, y, 40000); break;

		case REACTOR_PLATING: ptr = new ReactorPlating(REACTOR_PLATING, reactor, x, y, 1000); break;
		case CONTAINMENT_REACTOR_PLATING: ptr = new ReactorPlating(CONTAINMENT_REACTOR_PLATING, reactor, x, y, 500); break;
		case HEAT_CAPACITY_REACTOR_PLATING: ptr = new ReactorPlating(HEAT_CAPACITY_REACTOR_PLATING, reactor, x, y, 1700); break;

		default: ptr = 0; break;
	}
	return shared_ptr<ReactorComponent>(ptr);
}

void ReactorComponent::setDestroyed(bool d) {
	if(!reactor->ignoreComponentDestroyed) {
		bool wasDestroyed = pendingDestroyed;
		pendingDestroyed = d;
		if(d && !wasDestroyed) {
			reactor->componentDestroyed(x, y);
		}
	}
}



/***** Reactor *****/

Reactor::Reactor(int extraChambers) {
	height = 6;
	width = 3 + extraChambers;
	numExtraChambers = extraChambers;
	ignoreComponentDestroyed = false;
	components.reserve(width * height);
	for(int i = 0; i < width * height; i++) {
		components.push_back(shared_ptr<ReactorComponent>());
	}
	init();
}

Reactor::Reactor(const Reactor& other) {
	height = other.height;
	width = other.width;
	numExtraChambers = other.numExtraChambers;
	numUraniumCells = other.numUraniumCells;
	curSimState = other.curSimState;
	pendingSimState = other.pendingSimState;
	maxHeat = other.maxHeat;
	ignoreComponentDestroyed = other.ignoreComponentDestroyed;
	components.reserve(width * height);
	for(std::vector<shared_ptr<ReactorComponent>>::const_iterator itr = other.components.cbegin(); itr != other.components.cend(); ++itr) {
		shared_ptr<ReactorComponent> newComponent(itr->get() ? itr->get()->clone() : 0);
		if(newComponent.get()) {
			newComponent->reactor = this;
		}
		components.push_back(newComponent);
	}
}

std::vector<ComponentType> Reactor::getComponentTypes() {
	std::vector<ComponentType> ret;
	ret.reserve(width * height);
	for(std::vector<shared_ptr<ReactorComponent>>::iterator itr = components.begin(); itr != components.end(); ++itr) {
		if(!itr->get()) ret.push_back(COMPONENT_NONE);
		else ret.push_back((*itr)->type);
	}
	return ret;
}

void Reactor::setComponentTypes(std::vector<ComponentType> types) {
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			set(x, y, types[y*width+x]);
		}
	}
}

void Reactor::init() {
	//maxHeat = 10000 + numExtraChambers * 1000;
	maxHeat = 10000;
}

ReactorComponent* Reactor::get(int x, int y) const {
	if(x < 0 || y < 0 || x >= width || y >= height) return 0;
	ReactorComponent* ptr = components[y*width+x].get();
	if(!ptr) return 0;
	if(ptr->isDestroyed()) return 0;
	return ptr;
}

void Reactor::set(int x, int y, shared_ptr<ReactorComponent> component) {
	components[y*width+x] = component;
}

void Reactor::set(int x, int y, ComponentType type) {
	set(x, y, ReactorComponent::create(type, this, x, y));
}

void Reactor::commit() {
	curSimState = pendingSimState;
	for(shared_ptr<ReactorComponent>& comp : components) {
		if(comp.get()) {
			comp->commit();
			if(comp->isDestroyed()) {
				comp.reset();
			}
		}
	}
}

void Reactor::rollback() {
	pendingSimState = curSimState;
	for(std::vector<shared_ptr<ReactorComponent>>::iterator itr = components.begin(); itr != components.end(); ++itr) {
		if(itr->get()) {
			(*itr)->rollback();
		}
	}
}

int Reactor::getHeat() {
	return pendingSimState.reactorHeat;
}

void Reactor::setHeat(int heat) {
	pendingSimState.reactorHeat = heat;
	if(pendingSimState.reactorHeat >= maxHeat) {
		heatCapacityExceeded();
	}
}

int Reactor::addHeat(int heat) {
	pendingSimState.reactorHeat += heat;
	if(pendingSimState.reactorHeat >= maxHeat) {
		heatCapacityExceeded();
	}
	return pendingSimState.reactorHeat;
}

int Reactor::getMaxHeat() {
	return maxHeat;
}

void Reactor::componentDestroyed(int x, int y) {
	if(!ignoreComponentDestroyed) {
		pendingSimState.componentFailed = true;
	}
}

void Reactor::heatCapacityExceeded() {
	pendingSimState.meltdown = true;
}

void Reactor::generateEU(int eu) {
	pendingSimState.euGenerated += eu;
}

void Reactor::runTickPhase(SimPhase phase) {
	int x, y;
	for(y = 0; y < height; ++y) {
		for(x = 0; x < width; ++x) {
			ReactorComponent* comp = get(x, y);
			if(comp && !comp->isDestroyed()) {
				comp->tick(phase);
			}
		}
	}
}

void Reactor::runTick() {
	/*runTickPhase(PHASE_SETUP);
	runTickPhase(PHASE_POWER);
	runTickPhase(PHASE_HEAT_TRANSFER);
	runTickPhase(PHASE_HEAT_EXHAUST);*/
	maxHeat = 10000;
	runTickPhase(PHASE_HEAT_RUN);
	runTickPhase(PHASE_POWER);
	int totalHeat = getHeat();
	for(auto& comp : components) {
		if(comp.get()) {
			totalHeat += comp->getCurrentHeat();
		}
	}
	pendingSimState.totalHeat = totalHeat;
}

void Reactor::removeFuel() {
	for(std::vector<shared_ptr<ReactorComponent>>::iterator itr = components.begin(); itr != components.end(); ++itr) {
		if(itr->get()) {
			ComponentType type = (*itr)->type;
			if(type == URANIUM_CELL || type == DUAL_URANIUM_CELL || type == QUAD_URANIUM_CELL) {
				itr->reset();
			}
		}
	}
}

int Reactor::getTotalCost() {
	int total = 0;
	for(auto& cptr : components) {
		if(cptr.get()) {
			total += cptr->cost;
		}
	}
	return total;
}

// Returns before committing the tick that caused the stop condition
RunUntilStopReason Reactor::runUntil(bool stopOnMeltdown, bool stopOnFuelUsed, bool stopOnCooledDown, bool stopOnComponentFailed) {
	int maxTicks = timeoutTicks;
	bool firstIteration = true;
	int lastTotalHeat = -1;
	int noHeatLossCheckInterval = 8;
	for(;;) {
		if(pendingSimState.meltdown && stopOnMeltdown) {
			return STOPPED_ON_MELTDOWN;
		}
		if(pendingSimState.componentFailed && stopOnComponentFailed) {
			return STOPPED_ON_COMPONENT_FAILED;
		}
		if(pendingSimState.curTick >= fuelTicks && stopOnFuelUsed) {
			return STOPPED_ON_FUEL_USED;
		}
		if(pendingSimState.totalHeat <= 0 && stopOnCooledDown) {
			return STOPPED_ON_COOLED_DOWN;
		}
		if(pendingSimState.totalHeat < 100 && pendingSimState.totalHeat == curSimState.totalHeat && stopOnCooledDown) {
			// hack to get around small amounts of residual heat
			return STOPPED_ON_COOLED_DOWN;
		}
		if(pendingSimState.curTick >= maxTicks) {
			return STOPPED_ON_MAX_TICKS;
		}
		if(curSimState.curTick % noHeatLossCheckInterval == 0 && stopOnCooledDown) {
			if(lastTotalHeat == -1) {
				lastTotalHeat = curSimState.totalHeat;
			} else {
				if(lastTotalHeat <= curSimState.totalHeat) {
					// Try to catch timeouts early (where reactor is not cooling down)
					return STOPPED_ON_MAX_TICKS;
				}
				lastTotalHeat = curSimState.totalHeat;
			}
		}
		if(firstIteration) {
			firstIteration = false;
		} else {
			commit();
		}
		runTick();
		pendingSimState.curTick++;
	}
}

void Reactor::initializeSimulation() {
	// Reset simulation state
	curSimState = pendingSimState = SimulationState();

	// Initialize all components
	for(std::vector<shared_ptr<ReactorComponent>>::iterator itr = components.begin(); itr != components.end(); ++itr) {
		if(itr->get()) {
			(*itr)->init();
		}
	}

	// Calculate total number of uranium cells, and check for single use coolants
	numUraniumCells = 0;
	usesSingleUseCoolant = false;
	for(auto& component : components) {
		if(component.get()) {
			if(component->type == URANIUM_CELL) numUraniumCells++;
			else if(component->type == DUAL_URANIUM_CELL) numUraniumCells += 2;
			else if(component->type == QUAD_URANIUM_CELL) numUraniumCells += 4;
			else if(component->type == CONDENSATOR_RSH) usesSingleUseCoolant = true;
			else if(component->type == CONDENSATOR_LZH) usesSingleUseCoolant = true;
		}
	}
}

void Reactor::resetUsage() {
	for(auto& component : components) {
		if(component.get()) {
			component->resetUsage();
		}
	}
	curSimState.curTick = 0;
	curSimState.euGenerated = 0;
	pendingSimState = curSimState;
}

int getCyclesUntilFailure(int firstRunHeat, int secondRunHeat, int maxHeat) {
	if(maxHeat <= 0) return -1;
	int heatDiff = secondRunHeat - firstRunHeat;
	if(heatDiff <= 0) return -1;
	return (maxHeat - firstRunHeat - 1) / heatDiff + 1;
}

SimulationResults runSimulation(Reactor& initialReactor) {
	SimulationResults results;
	initialReactor.initializeSimulation();

	results.totalCost = initialReactor.getTotalCost();

	if(!initialReactor.numUraniumCells) {
		return results;	// no fuel
	}

	RunUntilStopReason firstStopReason = initialReactor.runUntil(true, true, false, true);

	if(firstStopReason == STOPPED_ON_FUEL_USED) {
		initialReactor.commit();
	}

	results.totalEUPerCycle = initialReactor.curSimState.euGenerated;
	results.euPerTick = results.totalEUPerCycle / initialReactor.curSimState.curTick;
	results.efficiency = (float)results.euPerTick / 5.0 / (float)initialReactor.numUraniumCells;
	results.usesSingleUseCoolant = initialReactor.usesSingleUseCoolant;

	if(firstStopReason == STOPPED_ON_COMPONENT_FAILED) {
		results.numIterationsBeforeFailure = 0;
		results.ticksUntilComponentFailure = initialReactor.curSimState.curTick;

		// Rollback the component failure and track time until cooled down
		Reactor cooldownReactor(initialReactor);
		cooldownReactor.rollback();
		cooldownReactor.removeFuel();
		cooldownReactor.ignoreComponentDestroyed = true;
		RunUntilStopReason cooldownStopReason = cooldownReactor.runUntil(false, false, true, false);
		cooldownReactor.commit();
		if(cooldownStopReason == STOPPED_ON_COOLED_DOWN) {
			results.cooldownTicks = cooldownReactor.curSimState.curTick - initialReactor.pendingSimState.curTick;
			results.cycleTicks = cooldownReactor.curSimState.curTick;
			results.overallEUPerTick = (float)results.totalEUPerCycle / (float)results.cycleTicks;
		} else if(cooldownStopReason == STOPPED_ON_MAX_TICKS) {
			results.timedOut = true;
			results.cycleTicks = -1;
			//std::cout << "\n\n";
			//printReactor(cooldownReactor);
			//std::cout << cooldownReactor.heat << "\n";
		} else {
			cout << "Invalid stop reason1\n";
			return results;
		}

		// Run another reactor until meltdown or the fuel is used up, with the component failed
		Reactor runUntilFinishReactor(initialReactor);
		runUntilFinishReactor.commit();
		RunUntilStopReason rufStopReason = runUntilFinishReactor.runUntil(true, true, false, false);

		if(initialReactor.curSimState.curTick * 100 / Reactor::fuelTicks >= 10) {
			// If the reactor ran for at least 10% of fuel lifetime before a component broke, it's a mark III
			results.mark = 3;
		} else if(runUntilFinishReactor.curSimState.curTick * 100 / Reactor::fuelTicks >= 10) {
			// If the reactor was able to go at least 10% of a cycle without melting down, but had components fry, it's a mark IV
			results.mark = 4;
		} else {
			// Mark V
			results.mark = 5;
		}

		if(rufStopReason == STOPPED_ON_MELTDOWN) {
			results.ticksUntilMeltdown = runUntilFinishReactor.curSimState.curTick;
		}

		// Now run it again until it's cooled down
		// THIS IS NOT ACTUALLY USED RIGHT NOW
		/*Reactor rufCooldownReactor(runUntilFinishReactor);
		if(rufStopReason == STOPPED_ON_MELTDOWN) rufCooldownReactor.rollback();
		else rufCooldownReactor.commit();
		rufCooldownReactor.removeFuel();
		RunUntilStopReason rufCooldownStopReason = rufCooldownReactor.runUntil(false, false, true, false);*/
	} else if(firstStopReason == STOPPED_ON_MELTDOWN) {
		results.numIterationsBeforeFailure = 0;
		results.ticksUntilMeltdown = initialReactor.curSimState.curTick;

		// Reactor is either mark III or mark V, depending on whether or not it made it at least 10% of a cycle
		if(initialReactor.curSimState.curTick * 100 / Reactor::fuelTicks >= 10) {
			results.mark = 3;
		} else {
			results.mark = 5;
		}

		// Roll back the meltdown and run until cooled down
		Reactor cooldownReactor(initialReactor);
		cooldownReactor.rollback();
		cooldownReactor.removeFuel();
		cooldownReactor.ignoreComponentDestroyed = true;
		RunUntilStopReason mdCooldownStopReason = cooldownReactor.runUntil(false, false, true, false);
		if(mdCooldownStopReason == STOPPED_ON_COOLED_DOWN) {
			results.cooldownTicks = cooldownReactor.curSimState.curTick - initialReactor.pendingSimState.curTick;
			results.cycleTicks = cooldownReactor.curSimState.curTick;
			results.overallEUPerTick = (float)results.totalEUPerCycle / (float)results.cycleTicks;
		} else if(mdCooldownStopReason == STOPPED_ON_MAX_TICKS) {
			results.timedOut = true;
			results.cycleTicks = -1;
		} else {
			cout << "Invalid stop reason2\n";
			return results;
		}
	} else if(firstStopReason == STOPPED_ON_FUEL_USED) {
		// Reactor is either a mark I or a mark II.
		if(initialReactor.curSimState.totalHeat <= 0) {
			// It's a mark I with no total heat at the end of each cycle
			results.mark = 1;
			results.overallEUPerTick = results.euPerTick;
			results.cycleTicks = Reactor::fuelTicks;
		} else {
			// It may still be a mark I, need to run additional tests

			// Test the cooldown time (may not be needed, but may as well include it in the results)
			Reactor cooldownReactor(initialReactor);
			cooldownReactor.removeFuel();
			cooldownReactor.ignoreComponentDestroyed = true;
			RunUntilStopReason cooldownStopReason = cooldownReactor.runUntil(false, false, true, false);
			if(cooldownStopReason == STOPPED_ON_COOLED_DOWN) {
				results.cooldownTicks = cooldownReactor.curSimState.curTick - initialReactor.pendingSimState.curTick;
				results.cycleTicks = cooldownReactor.curSimState.curTick;
				results.overallEUPerTick = (float)results.totalEUPerCycle / (float)results.cycleTicks;
			} else if(cooldownStopReason == STOPPED_ON_MAX_TICKS) {
				results.timedOut = true;
				results.cycleTicks = -1;
			} else {
				cout << "Invalid stop reason3\n";
				return results;
			}

			// Reset the reactor ticks, fuel usage, and condensators, but don't reset the heat.  Run it again and see what happens.
			Reactor rerunReactor(initialReactor);
			rerunReactor.resetUsage();
			RunUntilStopReason rerunStopReason = rerunReactor.runUntil(true, true, false, true);
			if(rerunStopReason == STOPPED_ON_MELTDOWN) {
				// It's a mark II that can only run 1 cycle before meltdown
				results.mark = 2;
				results.numIterationsBeforeFailure = 1;
			} else if(rerunStopReason == STOPPED_ON_COMPONENT_FAILED) {
				// Same as meltdown
				results.mark = 2;
				results.numIterationsBeforeFailure = 1;
			} else if(rerunStopReason == STOPPED_ON_FUEL_USED) {
				// Made it past the second run-through.  Compare heats for each component to
				// find which will fail first, and use that to calculate number of cycles.
				rerunReactor.commit();

				int minCyclesUntilFailure = getCyclesUntilFailure(initialReactor.getHeat(), rerunReactor.getHeat(), initialReactor.getMaxHeat());
				for(unsigned int i = 0; i < initialReactor.components.size(); ++i) {
					if(initialReactor.components[i].get()) {
						int cuf = getCyclesUntilFailure(initialReactor.components[i]->getCurrentHeat(), rerunReactor.components[i]->getCurrentHeat(), initialReactor.components[i]->getMaxHeat());
						if(cuf != -1) {
							if(minCyclesUntilFailure == -1 || cuf < minCyclesUntilFailure) {
								minCyclesUntilFailure = cuf;
							}
						}
					}
				}

				if(minCyclesUntilFailure == -1) {
					results.mark = 1;
					results.overallEUPerTick = results.euPerTick;
					results.cycleTicks = Reactor::fuelTicks;
				} else {
					results.mark = 2;
					results.numIterationsBeforeFailure = minCyclesUntilFailure;
				}

			} else {
				cout << "Invalid stop reason4\n";
				return results;
			}
		}

	} else {
		cout << "Invalid stop reason5\n";
		return results;
	}
	return results;
}


/***** HeatVent *****/

void HeatVent::tick(SimPhase phase) {
	if(phase == PHASE_HEAT_RUN) {
		if(heatFromReactor > 0) {
			int rh = reactor->getHeat();
			int rdrain = rh;
			if(rdrain > heatFromReactor) rdrain = heatFromReactor;
			rh -= rdrain;
			rdrain = alterHeat(rdrain);
			if(rdrain > 0) return;
			reactor->setHeat(rh);
		}

		/*int remaining = */alterHeat(-heatDissipated);
		//if(remaining <= 0) reactor->addEmitHeat(remaining + heatDissipated);
	}
}

/***** ComponentHeatVent *****/

void ComponentHeatVent::tick(SimPhase phase) {
	if(phase == PHASE_HEAT_RUN) {
		checkDissipate(reactor->left(x, y));
		checkDissipate(reactor->right(x, y));
		checkDissipate(reactor->above(x, y));
		checkDissipate(reactor->below(x, y));
	}
}

void ComponentHeatVent::checkDissipate(ReactorComponent* other) {
	if(!other) return;
	if(other->canStoreHeat()) {
		other->alterHeat(-heatFromEach);
	}
}


/***** HeatExchanger *****/

void HeatExchanger::tick(SimPhase phase) {
	int myHeat = 0;
	ReactorComponent* heatAcceptors[4];
	int heatAcceptorsLen = 0;
	double  med = (double)getCurrentHeat() / (double)getMaxHeat();
	int c = 1;

	if(transferToCore > 0) {
		c++;
		med += (double)reactor->getHeat() / (double)reactor->getMaxHeat();
	}

	if(transferToAdjacent > 0) {
		med += checkHeatAcceptor(reactor->left(x, y), heatAcceptors, heatAcceptorsLen);
		med += checkHeatAcceptor(reactor->right(x, y), heatAcceptors, heatAcceptorsLen);
		med += checkHeatAcceptor(reactor->above(x, y), heatAcceptors, heatAcceptorsLen);
		med += checkHeatAcceptor(reactor->below(x, y), heatAcceptors, heatAcceptorsLen);
	}

	med /= (c + heatAcceptorsLen);

	if(transferToAdjacent > 0) {
		for(int i = 0; i < heatAcceptorsLen; ++i) {
			ReactorComponent* comp = heatAcceptors[i];
			int add = (int)(med * (double)comp->getMaxHeat()) - comp->getCurrentHeat();
			if(add > transferToAdjacent) add = transferToAdjacent;
			if(add < -transferToAdjacent) add = -transferToAdjacent;
			myHeat -= add;
			add = comp->alterHeat(add);
			myHeat += add;
		}
	}

	if(transferToCore > 0) {
		int add = (int)(med * (double)reactor->getMaxHeat()) - reactor->getHeat();
		if(add > transferToCore) add = transferToCore;
		if(add < -transferToCore) add = -transferToCore;
		myHeat -= add;
		reactor->setHeat(reactor->getHeat() + add);
	}

	alterHeat(myHeat);
}

double HeatExchanger::checkHeatAcceptor(ReactorComponent* comp, ReactorComponent** heatAcceptors, int& heatAcceptorsLen) {
	if(comp) {
		if(comp->canStoreHeat()) {
			heatAcceptors[heatAcceptorsLen] = comp;
			heatAcceptorsLen++;
			double max = comp->getMaxHeat();
			if(max <= 0.0) return 0.0;
			double cur = comp->getCurrentHeat();
			return cur / max;
		}
	}
	return 0.0;
}


/***** Condensator *****/

bool Condensator::canStoreHeat() {
	return pendingStoredHeat < maxStoredHeat;
}

int Condensator::getMaxHeat() {
	return maxStoredHeat;
}

int Condensator::getCurrentHeat() {
	return 0;
}

int Condensator::alterHeat(int heat) {
	int can = maxStoredHeat - pendingStoredHeat;
	if(can > heat) can = heat;
	heat -= can;
	pendingStoredHeat += can;
	return heat;
}

void Condensator::resetUsage() {
	ReactorComponent::resetUsage();
	lastStoredHeat = pendingStoredHeat = 0;
}

void Condensator::commit() {
	ReactorComponent::commit();
	lastStoredHeat = pendingStoredHeat;
}

void Condensator::rollback() {
	ReactorComponent::rollback();
	pendingStoredHeat = lastStoredHeat;
}


/***** UraniumCell *****/

void UraniumCell::tick(SimPhase phase) {
	if(pendingUsage <= maxUsage) {

		for(int cellNum = 0; cellNum < numCells; ++cellNum) {
			int pulses = 1 + numCells / 2;
			if(phase != PHASE_HEAT_RUN) {
				for(int i = 0; i < pulses; ++i) {
					acceptUraniumPulse(this, phase);
				}
				pulses += checkPulseable(reactor->left(x, y), phase);
				pulses += checkPulseable(reactor->right(x, y), phase);
				pulses += checkPulseable(reactor->above(x, y), phase);
				pulses += checkPulseable(reactor->below(x, y), phase);
			} else {
				pulses += checkPulseable(reactor->left(x, y), phase);
				pulses += checkPulseable(reactor->right(x, y), phase);
				pulses += checkPulseable(reactor->above(x, y), phase);
				pulses += checkPulseable(reactor->below(x, y), phase);

				int heat = sumUp(pulses) * 4;

				ReactorComponent* heatAcceptors[4];
				int heatAcceptorsLen = 0;
				checkHeatAcceptor(reactor->left(x, y), heatAcceptors, heatAcceptorsLen);
				checkHeatAcceptor(reactor->right(x, y), heatAcceptors, heatAcceptorsLen);
				checkHeatAcceptor(reactor->above(x, y), heatAcceptors, heatAcceptorsLen);
				checkHeatAcceptor(reactor->below(x, y), heatAcceptors, heatAcceptorsLen);

				for(int i = 0; i < heatAcceptorsLen; ++i) {
					int dheat = heat / (heatAcceptorsLen - i);
					heat -= dheat;
					dheat = heatAcceptors[i]->alterHeat(dheat);
					heat += dheat;
				}
				if(heat > 0) reactor->addHeat(heat);
			}
		}

		if(phase == PHASE_HEAT_RUN) {
			pendingUsage++;
		}
	}
}

int UraniumCell::checkPulseable(ReactorComponent* comp, SimPhase phase) {
	if(comp) {
		if(comp->acceptUraniumPulse(this, phase)) {
			return 1;
		}
	}
	return 0;
}

int UraniumCell::sumUp(int x) {
	int sum = 0;
	for(int i = 1; i <= x; ++i) sum += i;
	return sum;
}

void UraniumCell::checkHeatAcceptor(ReactorComponent* comp, ReactorComponent** heatAcceptors, int& heatAcceptorsLen) {
	if(comp) {
		if(comp->canStoreHeat()) {
			heatAcceptors[heatAcceptorsLen] = comp;
			heatAcceptorsLen++;
		}
	}
}

bool UraniumCell::acceptUraniumPulse(ReactorComponent* fromComp, SimPhase phase) {
	if(pendingUsage <= maxUsage) {
		if(phase == PHASE_POWER) {
			reactor->generateEU(euPerPulse);
		}
		return true;
	} else {
		return false;
	}
}

void UraniumCell::commit() {
	ReactorComponent::commit();
	lastUsage = pendingUsage;
}

void UraniumCell::rollback() {
	ReactorComponent::rollback();
	pendingUsage = lastUsage;
}

void UraniumCell::resetUsage() {
	pendingUsage = lastUsage = 0;
}


/***** NeutronReflector *****/

bool NeutronReflector::acceptUraniumPulse(ReactorComponent* fromComp, SimPhase phase) {
	if(phase == PHASE_POWER) {
		reactor->generateEU(UraniumCell::euPerPulse);
	} else {
		pendingUsage++;
		if(pendingUsage > maxUsage) {
			setDestroyed(true);
		}
	}
	return true;
}

void NeutronReflector::commit() {
	ReactorComponent::commit();
	lastUsage = pendingUsage;
}

void NeutronReflector::rollback() {
	ReactorComponent::rollback();
	pendingUsage = lastUsage;
}

void NeutronReflector::resetUsage() {
	pendingUsage = lastUsage = 0;
}


/***** ReactorPlating *****/

void ReactorPlating::tick(SimPhase phase) {
	if(phase == PHASE_HEAT_RUN) {
		reactor->maxHeat += heatAddition;
	}
}


void printSimResults(SimulationResults r) {
	cout << "efficiency: " << r.efficiency << endl;
	cout << "totalEUPerCycle: " << r.totalEUPerCycle << endl;
	cout << "euPerTick: " << r.euPerTick << endl;
	cout << "overallEUPerTick: " << r.overallEUPerTick << endl;
	cout << "usesSingleUseCoolant: " << r.usesSingleUseCoolant << endl;
	cout << "timedOut: " << r.timedOut << endl;
	cout << "cooldownTicks: " << r.cooldownTicks << endl;
	cout << "cycleTicks: " << r.cycleTicks << endl;
	cout << "mark: " << r.mark << endl;
	cout << "numIterationsBeforeFailure: " << r.numIterationsBeforeFailure << endl;
	cout << "ticksUntilMeltdown: " << r.ticksUntilMeltdown << endl;
	cout << "ticksUntilComponentFailure: " << r.ticksUntilComponentFailure << endl;
	cout << "totalCost: " << r.totalCost << endl;
}


}
