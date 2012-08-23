#include "CK2Title.h"
#include "..\Parsers\Object.h"
#include "CK2World.h"
#include "CK2Character.h"
#include "CK2Title.h"
#include "CK2Dynasty.h"
#include "CK2History.h"
#include "..\Log.h"



CK2Title::CK2Title(string _titleString)
{
	titleString			= _titleString;
	holder				= NULL;
	heir					= NULL;
	successionLaw		= "";
	genderLaw			= "";
	nominees.clear();
	history.clear();
	liegeString			= "";
	liege					= NULL;
	vassals.clear();
	deJureLiegeString	= "";
	deJureLiege			= NULL;
	independent			= true;
	inHRE					= false;
}


void CK2Title::init(Object* obj,  map<int, CK2Character*>& characters)
{
	titleString = obj->getKey();
	holder = NULL;
	vector<Object*> holderObjs = obj->getValue("holder");
	if (holderObjs.size() > 0)
	{
		holder = characters[ atoi( holderObjs[0]->getLeaf().c_str() ) ];
		if (holder != NULL)
		{
			holder->addTitle(this);
		}
	}
	heir = NULL;
	successionLaw = obj->getLeaf("succession");
	genderLaw = obj->getLeaf("gender");

	vector<Object*> leavesObj = obj->getLeaves();
	for (unsigned int i = 0; i < leavesObj.size(); i++)
	{
		if (leavesObj[i]->getKey() == "nomination")
		{
			vector<Object*> nomineeObj = leavesObj[i]->getValue("nominee");
			int nomineeId = atoi( nomineeObj[0]->getLeaf("id").c_str() );

			bool nomineeMarked = false;
			for (unsigned int j = 0; j < nominees.size(); j++)
			{
				if (nominees[j].first == nomineeId)
				{
					nominees[j].second++;
					nomineeMarked = true;
				}
			}
			if (nomineeMarked == false)
			{
				nominees.push_back( make_pair(nomineeId, 1) );
			}
		}
	}

	vector<Object*> historyObjs = obj->getValue("history");
	if (historyObjs.size() > 0)
	{
		historyObjs = historyObjs[0]->getLeaves();
		for (unsigned int i = 0; i < historyObjs.size(); i++)
		{
			CK2History* newHistory = new CK2History(historyObjs[i], characters);
			history.push_back(newHistory);
		}
	}

	vector<Object*> liegeObjs = obj->getValue("liege");
	if (liegeObjs.size() > 0)
	{
		liegeString = liegeObjs[0]->getLeaf();
	}
	liege = NULL;

	vassals.clear();

	vector<Object*> deJureLiegeObjs = obj->getValue("de_jure_liege");
	if (deJureLiegeObjs.size() > 0)
	{
		deJureLiegeString = deJureLiegeObjs[0]->getLeaf();
	}
	else
	{
		deJureLiegeString = "";
	}
	if (deJureLiegeString[0] == '"')
	{
		deJureLiegeString = deJureLiegeString.substr(1, deJureLiegeString.size() - 2);
	}

	independent = true;
	inHRE = false;
}


void CK2Title::addLiege(CK2Title* newLiege)
{
	liege = newLiege;
	liege->addVassal(this);

	independent = false;
}


void CK2Title::addVassal(CK2Title* vassal)
{
	vassals.push_back(vassal);
}


void CK2Title::addToHRE()
{
	inHRE = true;
}


void CK2Title::determineHeir(map<int, CK2Character*>& characters)
{
	if (holder != NULL)
	{
		if (successionLaw == "primogeniture")
		{
			CK2Character* tempHolder = holder;
			do
			{
				heir = tempHolder->getPrimogenitureHeir(genderLaw, holder);
				tempHolder = tempHolder->getFather();
				if (tempHolder == NULL)
				{
					break;
				}
			} while (heir == NULL);
		}
		else if (successionLaw == "gavelkind")
		{
			if(heir == NULL) // if the heir is not null, we've already set this
			{
				holder->setGavelkindHeirs(genderLaw);
			}
		}
		else if (successionLaw == "seniority")
		{
			heir = holder->getDynasty()->getSenoirityHeir(genderLaw);
		}
		else if (successionLaw == "feudal_elective")
		{
			heir = getFeudalElectiveHeir(characters);
		}
		else if (successionLaw == "turkish_succession")
		{
			heir = getTurkishSuccessionHeir();
		}
	}
}


void CK2Title::setHeir(CK2Character* newHeir)
{
	heir = newHeir;
}


void CK2Title::setDeJureLiege(const map<string, CK2Title*>& titles)
{
	if (  (deJureLiegeString != "") && (deJureLiegeString != "---") && ( (deJureLiege == NULL) || (deJureLiege->getTitleString() != deJureLiegeString ) )  )
	{
		map<string, CK2Title*>::const_iterator titleItr = titles.find(deJureLiegeString);
		if (titleItr != titles.end())
		{
			deJureLiege = titleItr->second;
		}
	}
}


void CK2Title::addDeJureVassals(vector<Object*> obj, map<string, CK2Title*>& titles, CK2World* world)
{
	for (vector<Object*>::iterator itr = obj.begin(); itr < obj.end(); itr++)
	{
		string substr = (*itr)->getKey().substr(0, 2);
		if ( (substr != "e_") && (substr != "k_") && (substr != "d_") && (substr != "c_") && (substr != "b_") )
		{
			continue;
		}
		map<string, CK2Title*>::iterator titleItr = titles.find( (*itr)->getKey() );
		if (titleItr == titles.end())
		{
			CK2Title* newTitle = new CK2Title( (*itr)->getKey() );
			titles.insert( make_pair((*itr)->getKey(), newTitle) );
			titleItr = titles.find( (*itr)->getKey() );
		}
		else
		{
			log("Note! The CK2Title::addDeJureVassals() condition is needed!\n");
		}
		titleItr->second->setDeJureLiege(this);
		titleItr->second->addDeJureVassals( (*itr)->getLeaves(), titles, world );
	}
}


void CK2Title::getCultureWeights(map<string, int>& cultureWeights, const cultureMapping& cultureMap) const
{
	for (vector<CK2Title*>::const_iterator vassalItr = vassals.begin(); vassalItr < vassals.end(); vassalItr++)
	{
		(*vassalItr)->getCultureWeights(cultureWeights, cultureMap);
	}

	int weight = 0;
	if (titleString.substr(0, 2) == "e_")
	{
		weight = 5;
	}
	else if (titleString.substr(0, 2) == "k_")
	{
		weight = 4;
	}
	else if (titleString.substr(0, 2) == "d_")
	{
		weight = 3;
	}
	else if (titleString.substr(0, 2) == "c_")
	{
		weight = 2;
	}
	else if (titleString.substr(0, 2) == "b_")
	{
		weight = 1;
	}

	CK2Province* capital = holder->getCapital();
	if (capital != NULL)
	{
		string culture = determineEU3Culture(holder->getCulture(), cultureMap, capital);
		cultureWeights[culture] += weight;
	}
}

CK2Character* CK2Title::getFeudalElectiveHeir(map<int, CK2Character*>& characters)
{
	int nominee = -1;
	int mostVotes = 0;
	for (unsigned int i = 0; i < nominees.size(); i++)
	{
		if (nominees[i].second > mostVotes)
		{
			nominee		= nominees[i].first;
			mostVotes	= nominees[i].second;
		}
	}

	return characters[nominee];
}


CK2Character* CK2Title::getTurkishSuccessionHeir()
{
	vector<CK2Character*> potentialHeirs;
	potentialHeirs.clear();
	CK2Character* tempHolder = holder;
	do
	{
		potentialHeirs = tempHolder->getPotentialOpenHeirs(genderLaw, holder);
		tempHolder = tempHolder->getFather();
		if (tempHolder == NULL)
		{
			break;
		}
	} while (potentialHeirs.size() == 0);

	int largestDemesne = 0;
	for (vector<CK2Character*>::iterator i = potentialHeirs.begin(); i != potentialHeirs.end(); i++)
	{
		vector<CK2Title*> titles = (*i)->getTitles();
		int demesne = 0;
		for (vector<CK2Title*>::iterator j = titles.begin(); j != titles.end(); j++)
		{
			if ( (*j)->getTitleString().substr(0, 2) == "k_" )
			{
				demesne++;
			}
		}
		if (demesne > largestDemesne)
		{
			heir = *i;
			largestDemesne = demesne;
		}
	}

	if (heir == NULL)
	{
		for (vector<CK2Character*>::iterator i = potentialHeirs.begin(); i != potentialHeirs.end(); i++)
		{
			vector<CK2Title*> titles = (*i)->getTitles();
			int demesne = 0;
			for (vector<CK2Title*>::iterator j = titles.begin(); j != titles.end(); j++)
			{
				if ( (*j)->getTitleString().substr(0, 2) == "d_" )
				{
					demesne++;
				}
			}
			if (demesne > largestDemesne)
			{
				heir = *i;
				largestDemesne = demesne;
			}
		}
	}

	if (heir == NULL)
	{
		for (vector<CK2Character*>::iterator i = potentialHeirs.begin(); i != potentialHeirs.end(); i++)
		{
			vector<CK2Title*> titles = (*i)->getTitles();
			int demesne = 0;
			for (vector<CK2Title*>::iterator j = titles.begin(); j != titles.end(); j++)
			{
				if ( (*j)->getTitleString().substr(0, 2) == "c_" )
				{
					demesne++;
				}
			}
			if (demesne > largestDemesne)
			{
				heir = *i;
				largestDemesne = demesne;
			}
		}
	}

	if (heir == NULL)
	{
		for (vector<CK2Character*>::iterator i = potentialHeirs.begin(); i != potentialHeirs.end(); i++)
		{
			vector<CK2Title*> titles = (*i)->getTitles();
			int demesne = 0;
			for (vector<CK2Title*>::iterator j = titles.begin(); j != titles.end(); j++)
			{
				if ( (*j)->getTitleString().substr(0, 2) == "b_" )
				{
					demesne++;
				}
			}
			if (demesne > largestDemesne)
			{
				heir = *i;
				largestDemesne = demesne;
			}
		}
	}

	if ( (heir == NULL) && (potentialHeirs.size() > 0) )
	{
		heir = potentialHeirs[0];
	}

	return heir;
}