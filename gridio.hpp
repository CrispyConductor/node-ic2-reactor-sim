#ifndef GRIDIO_HPP
#define GRIDIO_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "reactorsim.hpp"

namespace reactorsim {

extern std::map<std::string, ComponentType> componentTypeByAbbr;
extern std::map<ComponentType, std::string> abbrByComponentType;

std::string getComponentTypeAbbr(ComponentType type);
ComponentType getComponentTypeByAbbr(std::string& str);
bool isValidComponentTypeAbbr(std::string& str);


void loadTypesGrid(const std::string& filename, std::vector<ComponentType>&components, int& width, int& height);
void printTypesGrid(std::vector<ComponentType>& types, int width, int height);
void printReactor(Reactor& reactor);


}

#endif
