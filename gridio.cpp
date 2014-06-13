#include "gridio.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <iomanip>

namespace reactorsim {


std::map<std::string, ComponentType> componentTypeByAbbr;
std::map<ComponentType, std::string> abbrByComponentType;

void addComponentString(ComponentType type, std::string abbr) {
	componentTypeByAbbr[std::string(abbr)] = type;
	abbrByComponentType[type] = std::string(abbr);
}

void loadStringMaps() {
	if(componentTypeByAbbr.size()) return;
	addComponentString(COMPONENT_NONE, "XX");
	addComponentString(HEAT_VENT, "VV");
	addComponentString(REACTOR_HEAT_VENT, "VR");
	addComponentString(ADVANCED_HEAT_VENT, "VA");
	addComponentString(COMPONENT_HEAT_VENT, "VC");
	addComponentString(OVERCLOCKED_HEAT_VENT, "VO");
	addComponentString(HEAT_EXCHANGER, "EE");
	addComponentString(ADVANCED_HEAT_EXCHANGER, "EA");
	addComponentString(CORE_HEAT_EXCHANGER, "ER");
	addComponentString(COMPONENT_HEAT_EXCHANGER, "EC");
	addComponentString(COOLANT_CELL_10, "C1");
	addComponentString(COOLANT_CELL_30, "C3");
	addComponentString(COOLANT_CELL_60, "C6");
	addComponentString(CONDENSATOR_RSH, "CR");
	addComponentString(CONDENSATOR_LZH, "CL");
	addComponentString(URANIUM_CELL, "U1");
	addComponentString(DUAL_URANIUM_CELL, "U2");
	addComponentString(QUAD_URANIUM_CELL, "U4");
	addComponentString(NEUTRON_REFLECTOR, "NN");
	addComponentString(THICK_NEUTRON_REFLECTOR, "NT");
	addComponentString(REACTOR_PLATING, "PP");
	addComponentString(CONTAINMENT_REACTOR_PLATING, "PC");
	addComponentString(HEAT_CAPACITY_REACTOR_PLATING, "PH");
}

std::string getComponentTypeAbbr(ComponentType type) {
	loadStringMaps();
	if(abbrByComponentType.find(type) == abbrByComponentType.end()) {
		return std::to_string((int)type);
	} else {
		return abbrByComponentType[type];
	}
}

ComponentType getComponentTypeByAbbr(std::string& str) {
	loadStringMaps();
	return componentTypeByAbbr[str];
}

bool isValidComponentTypeAbbr(std::string& str) {
	loadStringMaps();
	return componentTypeByAbbr.find(str) != componentTypeByAbbr.end();
}

void loadTypesGrid(const std::string& filename, std::vector<ComponentType>&components, int& width, int& height) {
	loadStringMaps();
	std::ifstream ifs;
	ifs.open(filename, std::ifstream::in);
	width = 0;
	height = 0;
	while(ifs.good()) {
		std::string line;
		std::getline(ifs, line);
		size_t start;
		if(ifs.good() && line.length()) {
			size_t pos = 0;
			size_t num = 0;
			for(;;) {
				start = line.find_first_not_of(" \t", pos);
				if(start == std::string::npos) break;
				pos = line.find_first_of(" \t", start);
				if(pos == std::string::npos) pos = line.length();
				if(pos > start) {
					std::string abbr = line.substr(start, pos - start);
					num++;
					components.push_back(getComponentTypeByAbbr(abbr));
				}
			}
			if(num > 0) {
				width = num;
				height++;
			}
		}
	}
}

void printTypesGrid(std::vector<ComponentType>& types, int width, int height) {
	loadStringMaps();
	int n = 0;
	for(std::vector<ComponentType>::iterator itr = types.begin(); itr != types.end(); ++itr) {
		if(n % width != 0) std::cout << ' ';
		std::cout << getComponentTypeAbbr(*itr);
		if(n % width == width - 1) std::cout << std::endl;
		n++;
	}
}

void printReactor(Reactor& reactor) {
	loadStringMaps();
	int n = 0;
	for(auto& component : reactor.components) {
		if(n % reactor.width != 0) std::cout << ' ';
		if(component.get()) {
			std::cout << getComponentTypeAbbr(component->type) << ":" << std::setfill('0') << std::setw(5) << component->getCurrentHeat();
		} else {
			std::cout << "XX:00000";
		}
		if(n % reactor.width == reactor.width - 1) std::cout << std::endl;
		n++;
	}
}


}
