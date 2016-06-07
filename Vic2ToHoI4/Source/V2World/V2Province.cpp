/*Copyright (c) 2016 The Paradox Game Converters Project

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/



#include "V2Province.h"
#include "Log.h"
#include "Object.h"
#include "V2Pop.h"
using namespace std;



V2Province::V2Province(Object* obj)
{
	num = _wtoi(obj->getKey().c_str());

	vector<Object*> ownerObjs;				// the object holding the owner
	ownerObjs = obj->getValue(L"owner");
	(ownerObjs.size() == 0) ? ownerString = L"" : ownerString = ownerObjs[0]->getLeaf();
	owner = NULL;

	cores.clear();
	vector<Object*> coreObjs;				// the object holding the cores
	coreObjs = obj->getValue(L"core");
	for (auto coreObj: coreObjs)
	{
		cores.push_back(coreObj->getLeaf());
	}

	vector<Object*> buildingObjs;
	fortLevel = 0;
	buildingObjs = obj->getValue(L"fort");
	if (buildingObjs.size() > 0)
	{
		vector<wstring> tokens = buildingObjs[0]->getTokens();
		if (tokens.size() > 0)
		{
			fortLevel = _wtoi(tokens[0].c_str());
		}
	}
	navalBaseLevel = 0;
	buildingObjs = obj->getValue(L"naval_base");
	if (buildingObjs.size() > 0)
	{
		vector<wstring> tokens = buildingObjs[0]->getTokens();
		if (tokens.size() > 0)
		{
			navalBaseLevel = _wtoi(tokens[0].c_str());
		}
	}
	railLevel = 0;
	buildingObjs = obj->getValue(L"railroad");
	if (buildingObjs.size() > 0)
	{
		vector<wstring> tokens = buildingObjs[0]->getTokens();
		if (tokens.size() > 0)
		{
			railLevel = _wtoi(tokens[0].c_str());
		}
	}

	// read pops
	vector<Object*> children = obj->getLeaves();
	for (auto itr: children)
	{
		wstring key = itr->getKey();
		if (key == L"aristocrats" || key == L"artisans" || key == L"bureaucrats" || key == L"capitalists" || key == L"clergymen"
			|| key == L"craftsmen" || key == L"clerks" || key == L"farmers" || key == L"soldiers" || key == L"officers"
			|| key == L"labourers" || key == L"slaves")
		{
			V2Pop* pop = new V2Pop(itr);
			pops.push_back(pop);
		}
	}

	employedWorkers = 0;
}


vector<V2Country*> V2Province::getCores(const map<wstring, V2Country*>& countries) const
{
	vector<V2Country*> coreOwners;
	for (auto core: cores)
	{
		map<wstring, V2Country*>::const_iterator j = countries.find(core);
		if (j != countries.end())
		{
			coreOwners.push_back(j->second);
		}
	}

	return coreOwners;
}


void V2Province::addCore(wstring newCore)
{
	// only add if unique
	if ( find(cores.begin(), cores.end(), newCore) == cores.end() )
	{
		cores.push_back(newCore);
	}
}


void V2Province::removeCore(wstring tag)
{
	for (auto core = cores.begin(); core != cores.end(); core++)
	{
		if (*core == tag)
		{
			cores.erase(core);
			if (cores.size() == 0)
			{
				break;
			}
			core = cores.begin();
		}
	}
}


int V2Province::getTotalPopulation() const
{
	int total = 0;
	for (auto itr: pops)
	{
		total += itr->getSize();
	}
	return total;
}


//static bool PopSortBySizePredicate(const V2Pop* pop1, const V2Pop* pop2)
//{
//	return (pop1->getSize() > pop2->getSize());
//}



int V2Province::getPopulation(wstring type) const
{
	int totalPop = 0;
	for (auto pop: pops)
	{
		// empty string for type gets total population
		if (type == L"" || type == pop->getType())
		{
			totalPop += pop->getSize();
		}
	}
	return totalPop;
}


int V2Province::getLiteracyWeightedPopulation(wstring type) const
{
	double literacyWeight = Configuration::getLiteracyWeight();
	int totalPop = 0;
	for (auto pop: pops)
	{
		// empty string for type gets total population
		if (type == L"" || type == pop->getType())
		{
			totalPop += int(pop->getSize() * (pop->getLiteracy() * literacyWeight + (1.0 - literacyWeight)));
		}
	}
	return totalPop;
}