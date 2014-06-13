#include <node.h>
#include <v8.h>
#include <vector>
#include <iostream>
#include <string>
#include <memory>
#include <uv.h>
#include "reactorsim.hpp"
#include "gridio.hpp"

using namespace v8;
using namespace reactorsim;
using std::vector;

Local<Object> simResultsToV8Object(SimulationResults& results) {
	Local<Object> obj = Object::New();

#define RES_NUMBER(name) obj->Set(String::New(#name), Number::New(results.name));
#define RES_INT(name) obj->Set(String::New(#name), Integer::New(results.name));
#define RES_BOOL(name) obj->Set(String::New(#name), Boolean::New(results.name));

	RES_NUMBER(efficiency)
	RES_NUMBER(totalEUPerCycle)
	RES_INT(euPerTick)
	RES_INT(overallEUPerTick)
	RES_BOOL(usesSingleUseCoolant)
	RES_BOOL(timedOut)
	RES_INT(cooldownTicks)
	RES_INT(cycleTicks)
	RES_INT(mark)
	RES_INT(numIterationsBeforeFailure)
	RES_INT(ticksUntilMeltdown)
	RES_INT(ticksUntilComponentFailure)
	RES_INT(totalCost)

	return obj;
}

struct SimData {
	uv_work_t request;
	Persistent<Function> callback;

	std::shared_ptr<Reactor> reactor;
	SimulationResults simResults;
};

void runSimWork(uv_work_t* req) {
	SimData* simData = static_cast<SimData*>(req->data);
	simData->simResults = runSimulation(*(simData->reactor));
}

void runSimAfter(uv_work_t* req, int status) {
	HandleScope scope;
	SimData* simData = static_cast<SimData*>(req->data);
	Local<Object> results = simResultsToV8Object(simData->simResults);

	// Call callback
	Local<Value> cbArgs[] = { Local<Value>::New(Null()), results };
	TryCatch tryCatch;
	simData->callback->Call(Context::GetCurrent()->Global(), 2, cbArgs);
	if(tryCatch.HasCaught()) {
		node::FatalException(tryCatch);
	}

	simData->callback.Dispose();
	delete simData;
}

Handle<Value> nodeRunSimulation(const Arguments& args) {
	HandleScope scope;

	if(args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if(!args[0]->IsArray()) {
		ThrowException(Exception::TypeError(String::New("Argument must be array")));
		return scope.Close(Undefined());
	}

	if(!args[1]->IsFunction()) {
		ThrowException(Exception::TypeError(String::New("Second argument must be callback")));
		return scope.Close(Undefined());
	}

	Local<Function> callback = Local<Function>::Cast(args[1]);

	Handle<Array> componentArray = args[0].As<Array>();
	uint32_t len = componentArray->Length();

	if(len % 6 != 0 || len < 3*6 || len > 9*6) {
		ThrowException(Exception::TypeError(String::New("Invalid number of components")));
		return scope.Close(Undefined());
	}

	int extraChambers = len / 6 - 3;
	std::shared_ptr<Reactor> reactor(new Reactor(extraChambers));

	vector<ComponentType> components;
	components.reserve(len);
	char buf[10];
	for(uint32_t i = 0; i < len; i++) {
		Local<Value> val = componentArray->Get(i);
		Local<String> valString;
		if(val->IsString()) {
			valString = val.As<String>();
		} else if(val->IsStringObject()) {
			valString = val.As<StringObject>()->StringValue();
		} else {
			ThrowException(Exception::TypeError(String::New("Components must be string codes")));
			return scope.Close(Undefined());
		}
		int written = valString->WriteAscii(buf, 0, 9);
		buf[written] = 0;
		std::string stlString(buf);
		if(!isValidComponentTypeAbbr(stlString)) {
			ThrowException(Exception::TypeError(String::New((std::string("Invalid component code: ") + stlString).c_str())));
			return scope.Close(Undefined());
		}
		ComponentType type = getComponentTypeByAbbr(stlString);
		components.push_back(type);
	}

	reactor->setComponentTypes(components);

	SimData* simData = new SimData();
	simData->request.data = simData;
	simData->callback = Persistent<Function>::New(callback);
	simData->reactor = reactor;
	uv_queue_work(uv_default_loop(), &simData->request, runSimWork, runSimAfter);

	return scope.Close(Undefined());

	//SimulationResults simResults = runSimulation(reactor);

	//return scope.Close(simResultsToV8Object(simResults));
}

void nodeInit(Handle<Object> exports) {
	exports->Set(String::NewSymbol("runSimulation"), FunctionTemplate::New(nodeRunSimulation)->GetFunction());
}

NODE_MODULE(nodereactorsim, nodeInit)
