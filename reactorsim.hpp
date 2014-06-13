#ifndef REACTORSIM_HPP
#define REACTORSIM_HPP

#include <vector>
#include <memory>
#include <utility>

using std::shared_ptr;

namespace reactorsim {

enum ComponentType {
	COMPONENT_NONE,

	HEAT_VENT,
	REACTOR_HEAT_VENT,
	ADVANCED_HEAT_VENT,
	COMPONENT_HEAT_VENT,
	OVERCLOCKED_HEAT_VENT,

	HEAT_EXCHANGER,
	ADVANCED_HEAT_EXCHANGER,
	CORE_HEAT_EXCHANGER,
	COMPONENT_HEAT_EXCHANGER,

	COOLANT_CELL_10,
	COOLANT_CELL_30,
	COOLANT_CELL_60,

	CONDENSATOR_RSH,
	CONDENSATOR_LZH,

	URANIUM_CELL,
	DUAL_URANIUM_CELL,
	QUAD_URANIUM_CELL,

	NEUTRON_REFLECTOR,
	THICK_NEUTRON_REFLECTOR,

	REACTOR_PLATING,
	CONTAINMENT_REACTOR_PLATING,
	HEAT_CAPACITY_REACTOR_PLATING,

	COMPONENT_COUNT
};

enum SimPhase {
	/*PHASE_SETUP,
	PHASE_POWER,
	PHASE_HEAT_TRANSFER,
	PHASE_HEAT_EXHAUST*/
	PHASE_HEAT_RUN,
	PHASE_POWER
};

struct SimulationResults {
	float efficiency = 0;			// efficiency value (eu during operation / 5 / numUraniumCells)
	float totalEUPerCycle = 0;	// total EU produced in each complete cycle (run/stop/cooldown) of the reactor, until meltdown, component failure, or fuel used
	int euPerTick = 0;			// EU/t during reactor operation
	int overallEUPerTick = 0;	// Average eu/t including cooldown (cooldown starts after component failure or meltdown)
	bool usesSingleUseCoolant = false;	// Whether or not any condensators are used
	bool timedOut = false;		// If the reactor reached a timeout before cooling down
	int cooldownTicks = 0;		// Number of ticks to cooldown after a cycle
	int cycleTicks = 0;			// Number of ticks in a cycle (including cooldown if necessary)
	int mark = 0;				// Mark level (0-5)
	int numIterationsBeforeFailure = -1;	// Number of complete fuel-used-up iterations the reactor undergoes before meltdown or component failure, without cooldown
	int ticksUntilMeltdown = -1;	// If meltdown before 10000 ticks, number of ticks until the meltdown
	int ticksUntilComponentFailure = -1;	// If component failure before 10000, number of ticks until the failure
	int totalCost = 0;			// Sum of component costs
};

class Committable {
public:
	virtual void commit() = 0;
	virtual void rollback() = 0;
};

class Reactor;

class ReactorComponent : public Committable {	// Equivalent of IReactorComponent
public:
	static shared_ptr<ReactorComponent> create(ComponentType type, Reactor* reactor, int x, int y);

	ComponentType type;
	Reactor* reactor;
	int x;
	int y;
	int cost;
	bool lastDestroyed;
	bool pendingDestroyed;

	virtual void init() {}	// for things like adding to the reactor's heat capacity
	virtual void tick(SimPhase phase) {}
	virtual bool acceptUraniumPulse(ReactorComponent* fromComp, SimPhase phase) { return false; }
	virtual bool canStoreHeat() { return false; }
	virtual int getMaxHeat() { return 0; }
	virtual int getCurrentHeat() { return 0; }
	virtual int alterHeat(int heat) { return heat; }
	virtual void resetUsage() {}	// resets condensator use, reflector use, and uranium cell use
	bool isDestroyed() { return pendingDestroyed; }
	void setDestroyed(bool d);

	ReactorComponent(ComponentType _type, Reactor* _reactor, int _x, int _y) : type(_type), reactor(_reactor), x(_x), y(_y), cost(2), lastDestroyed(false), pendingDestroyed(false) {
		init();
	}

	virtual ReactorComponent* clone() = 0;
	virtual ~ReactorComponent() {}

	virtual void commit() { lastDestroyed = pendingDestroyed; }
	virtual void rollback() { pendingDestroyed = lastDestroyed; }
};

class Heatable : public ReactorComponent {	// Equivalent of IC2's ItemReactorHeatStorage
public:
	int lastHeat;
	int pendingHeat;
	int maxHeat;

	Heatable(ComponentType type, Reactor* reactor, int x, int y, int maxHeat) : ReactorComponent(type, reactor, x, y), lastHeat(0), pendingHeat(0), maxHeat(maxHeat) {}

	virtual bool canStoreHeat() { return true; }
	virtual int getMaxHeat() { return maxHeat; }
	virtual int getCurrentHeat() { return pendingHeat; }
	virtual int alterHeat(int heat);
	virtual ~Heatable() {}
	virtual void commit() { ReactorComponent::commit(); lastHeat = pendingHeat; }
	virtual void rollback() { ReactorComponent::rollback(); pendingHeat = lastHeat; }
};

enum RunUntilStopReason {
	STOPPED_ON_MELTDOWN,
	STOPPED_ON_FUEL_USED,
	STOPPED_ON_COOLED_DOWN,
	STOPPED_ON_COMPONENT_FAILED,
	STOPPED_ON_MAX_TICKS
};

class Reactor : public Committable {

public:

	static const int timeoutTicks = 50000;
	static const int fuelTicks = 10000;

	int width;
	int height;
	int numExtraChambers;
	std::vector<shared_ptr<ReactorComponent>> components;
	int maxHeat;

	bool ignoreComponentDestroyed;

	Reactor(int extraChambers);
	Reactor(const Reactor& other);

	std::vector<ComponentType> getComponentTypes();
	void setComponentTypes(std::vector<ComponentType> types);

	ReactorComponent* get(int x, int y) const;
	void set(int x, int y, shared_ptr<ReactorComponent> component);
	void set(int x, int y, ComponentType type);

	ReactorComponent* left(int x, int y) { return this->get(x - 1, y); }
	ReactorComponent* right(int x, int y) { return this->get(x + 1, y); }
	ReactorComponent* above(int x, int y) { return this->get(x, y - 1); }
	ReactorComponent* below(int x, int y) { return this->get(x, y + 1); }

	void init();
	void componentDestroyed(int x, int y);
	void heatCapacityExceeded();
	void generateEU(int eu);

	void commit();
	void rollback();

	int getHeat();
	void setHeat(int heat);
	int addHeat(int heat);
	int getMaxHeat();

	int getTotalCost();

	struct SimulationState {
		int curTick = 0;
		bool meltdown = false;
		bool componentFailed = false;
		int euGenerated = 0;
		int totalHeat = 0;
		int reactorHeat = 0;
	};

	int numUraniumCells;
	bool usesSingleUseCoolant;

	SimulationState curSimState;
	SimulationState pendingSimState;

	RunUntilStopReason runUntil(bool stopOnMeltdown, bool stopOnFuelUsed, bool stopOnCooledDown, bool stopOnComponentFailed);
	void runTickPhase(SimPhase phase);
	void runTick();
	void removeFuel();
	void initializeSimulation();

	void resetUsage();
};

SimulationResults runSimulation(Reactor& reactor);


class HeatVent : public Heatable {
public:
	int heatDissipated;
	int heatFromReactor;
	HeatVent(ComponentType type, Reactor* reactor, int x, int y, int heatDissipated, int heatFromReactor, int maxHeat) :
		Heatable(type, reactor, x, y, maxHeat),
		heatDissipated(heatDissipated),
		heatFromReactor(heatFromReactor)
	{}
	void tick(SimPhase phase);

	HeatVent* clone() {
		return new HeatVent(*this);
	}
};

// This seems to work much differently from other heat vents
class ComponentHeatVent : public ReactorComponent {	// Equivalent of IC2 ItemReactorVentSpread
public:
	int heatFromEach;
	ComponentHeatVent(ComponentType type, Reactor* reactor, int x, int y, int heatFromEach) :
		ReactorComponent(type, reactor, x, y),
		heatFromEach(heatFromEach)
	{}
	void tick(SimPhase phase);

	ComponentHeatVent* clone() {
		return new ComponentHeatVent(*this);
	}

private:
	void checkDissipate(ReactorComponent* other);
};


class HeatExchanger : public Heatable {	// Equivalent of ItemReactorHeatSwitch
public:

	int transferToAdjacent;
	int transferToCore;

	HeatExchanger(ComponentType type, Reactor* reactor, int x, int y, int transferToAdjacent, int transferToCore, int maxHeat) :
		Heatable(type, reactor, x, y, maxHeat),
		transferToAdjacent(transferToAdjacent),
		transferToCore(transferToCore)
	{}
	void tick(SimPhase phase);

	HeatExchanger* clone() {
		return new HeatExchanger(*this);
	}

private:
	// array of pointers
	double checkHeatAcceptor(ReactorComponent* comp, ReactorComponent** heatAcceptors, int& heatAcceptorsLen);
};


class CoolantCell : public Heatable {
public:
	CoolantCell(ComponentType type, Reactor* reactor, int x, int y, int maxHeat) : Heatable(type, reactor, x, y, maxHeat) {}
	CoolantCell* clone() {
		return new CoolantCell(*this);
	}
};


class Condensator : public ReactorComponent {
public:
	int maxStoredHeat;
	int lastStoredHeat;
	int pendingStoredHeat;
	Condensator(ComponentType type, Reactor* reactor, int x, int y, int maxHeat) :
		ReactorComponent(type, reactor, x, y),
		maxStoredHeat(maxHeat),
		lastStoredHeat(0),
		pendingStoredHeat(0)
	{}

	bool canStoreHeat();
	int getMaxHeat();
	int getCurrentHeat();
	int alterHeat(int heat);

	Condensator* clone() {
		return new Condensator(*this);
	}

	void resetUsage();
	void commit();
	void rollback();
};

class UraniumCell : public ReactorComponent {
public:
	static const int euPerPulse = 5;	// eu to generate per each pulse received

	int numCells;
	int lastUsage;
	int pendingUsage;
	int maxUsage;

	UraniumCell(ComponentType type, Reactor* reactor, int x, int y, int numCells) :
		ReactorComponent(type, reactor, x, y),
		numCells(numCells),
		lastUsage(0),
		pendingUsage(0),
		maxUsage(10000)
	{}

	void tick(SimPhase phase);
	bool acceptUraniumPulse(ReactorComponent* fromComp, SimPhase phase);

	void commit();
	void rollback();
	void resetUsage();

	UraniumCell* clone() {
		return new UraniumCell(*this);
	}

private:
	int checkPulseable(ReactorComponent* comp, SimPhase phase);
	int sumUp(int x);
	void checkHeatAcceptor(ReactorComponent* comp, ReactorComponent** heatAcceptors, int& heatAcceptorsLen);

};

class NeutronReflector : public ReactorComponent {
public:

	int lastUsage;
	int pendingUsage;
	int maxUsage;

	NeutronReflector(ComponentType type, Reactor* reactor, int x, int y, int durability) :
		ReactorComponent(type, reactor, x, y),
		lastUsage(0),
		pendingUsage(0),
		maxUsage(durability)
	{}

	void tick(SimPhase phase) {}

	bool acceptUraniumPulse(ReactorComponent* fromComp, SimPhase phase);

	void commit();
	void rollback();
	void resetUsage();

	NeutronReflector* clone() {
		return new NeutronReflector(*this);
	}
};

class ReactorPlating : public ReactorComponent {
public:
	int heatAddition;
	ReactorPlating(ComponentType type, Reactor* reactor, int x, int y, int heatAddition) :
		ReactorComponent(type, reactor, x, y),
		heatAddition(heatAddition)
	{}
	void tick(SimPhase phase);

	ReactorPlating* clone() {
		return new ReactorPlating(*this);
	}
};

void printSimResults(SimulationResults r);


}
#endif
