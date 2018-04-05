#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>

#define MAX_AVAILABLE_MOL 5
#define MAX_SAMPLE 3
#define PROJECT_NB 3
using namespace std;

/* ----------------------- START UTILS   -------------------------- */
int random(int min, int max) //range : [min, max)
{
	static bool first = true;
	if (first)
	{
		srand(time(NULL)); //seeding for the first time only!
		first = false;
	}
	if (max == min) return max;
	return min + rand() % (max - min);
}

enum MoleculeType { UNKNOWN = -1, A = 0, B = 1, C = 2, D = 3, E = 4 };
enum ModuleType { NONE = -1, DIAGNOSIS = 0, MOLECULES = 1, LABORATORY = 2, SAMPLES = 3 };
enum CarrierType { CLOUD = -1, ME = 0, OPPONENT = 1 };

class Molecule;
class Robot;
class Sample;
class Project;

Robot *myRobot;
Robot *opRobot;
Sample *allSamples[100];
Sample *cloudSamples[100];
Project *project[PROJECT_NB];

int turns = 0;
int sampleCount;
int sampleC = 0;
int cloudSampleC = 0;
int availableA, availableB, availableC, availableD, availableE;
int myScore, opScore;

class MoleculeStack {
public:
	MoleculeStack(void) {};
	~MoleculeStack(void) {};

	MoleculeType type;
	int nb;
};

class Project {
public:
	Project(void) {};
	~Project(void) {};

	virtual void updateWithInputs(int pexpA, int pexpB, int pexpC, int pexpD, int pexpE)
	{
		expA = pexpA;
		expB = pexpB;
		expC = pexpC;
		expD = pexpD;
		expE = pexpE;
	}
	void print()
	{
		cerr << "Project expA:" << expA << " expB:" << expB << " expC:" << expC << " expD:" << expD << " expE:" << expE << endl;
	}

	int expA, expB, expC, expD, expE;
};

class Sample {
public:
	Sample(void) {};
	Sample(int pid, CarrierType pcarry = CLOUD, int php = 0) : id(pid), carrier(pcarry), hp(php) {};

	~Sample(void) {};

	void copy(Sample s)
	{
		id = s.id;
		hp = s.hp;
		carrier = s.carrier;
		costA = s.costA;
		costB = s.costB;
		costC = s.costC;
		costD = s.costD;
		costE = s.costE;
		diagnosed = s.diagnosed;
		isComplete = s.isComplete;
		canComplete = s.canComplete;
		expGain = s.expGain;
		score = s.score;
	}
	void reset()
	{
		id = -1;
		hp = -1;
		carrier = CLOUD;
		costA = -1;
		costB = -1;
		costC = -1;
		costD = -1;
		costE = -1;
		diagnosed = true;
		isComplete = true;
		canComplete = false;
		expGain = UNKNOWN;
		score = -1;
	}

	void print()
	{
		cerr << "Id:" << id;
		switch (carrier)
		{
		case -1: cerr << " cloud"; break;
		case 0: cerr << " me"; break;
		case 1: cerr << " op";  break;
		}
		cerr << " Score:" << score << " HP:" << hp << " A:" << costA << " B:" << costB << " C:" << costC << " D:" << costD << " E:" << costE << endl;
	}
	virtual void updateWithInputs(int pid, CarrierType pcarry, int phealth, string pexpertiseGain,
		int pcostA, int pcostB, int pcostC, int pcostD, int pcostE)
	{
		if (phealth == -1) // undiagnosed
		{
			diagnosed = false;
		}
		else diagnosed = true;
		id = pid;
		carrier = pcarry;
		hp = phealth;
		costA = pcostA;
		costB = pcostB;
		costC = pcostC;
		costD = pcostD;
		costE = pcostE;
		totalCost = pcostA + pcostB + pcostC + pcostD + pcostE;

		if (pexpertiseGain == "A") expGain = A;
		else if (pexpertiseGain == "B") expGain = B;
		else if (pexpertiseGain == "C") expGain = C;
		else if (pexpertiseGain == "D") expGain = D;
		else if (pexpertiseGain == "E") expGain = E;
	}
	virtual void updateCostsWithExpertise(int pexpertiseA, int pexpertiseB, int pexpertiseC, int pexpertiseD, int pexpertiseE)
	{
		costA -= pexpertiseA;
		costB -= pexpertiseB;
		costC -= pexpertiseC;
		costD -= pexpertiseD;
		costE -= pexpertiseE;
		if (costA < 0) costA = 0;
		if (costB < 0) costB = 0;
		if (costC < 0) costC = 0;
		if (costD < 0) costD = 0;
		if (costE < 0) costE = 0;
		totalCost = costA + costB + costC + costD + costE;
	}

	int id;
	int hp;
	CarrierType carrier;
	int costA, costB, costC, costD, costE;
	MoleculeType expGain;
	bool diagnosed;
	int totalCost;
	bool isComplete, canComplete;
	int score;
};

class Robot {
public:
	Robot(void) : currentModule(NONE) {
		moleculeCounter = 0;
	};
	~Robot(void) {};

	void reset()
	{
		sampleCounter = 0;
		moleculeCounter = 0;
		molA = 0;
		molB = 0;
		molC = 0;
		molD = 0;
		molE = 0;
		samples[0]->reset();
		samples[1]->reset();
		samples[2]->reset();
	}

	void print()
	{
		cerr << "sampleCounter:" << sampleCounter;
		cerr << " moleculeCounter:" << moleculeCounter << endl;
		cerr << "molA:" << molA << " molB:" << molB << " molC:" << molC << " molD:" << molD << " molE:" << molE << endl;

		for (int i = 0; i < sampleCounter; ++i)
		{
			cerr << "sample " << samples[i]->id;
			cerr << " score:" << samples[i]->score;
			cerr << " A:" << samples[i]->costA;
			cerr << " B:" << samples[i]->costB;
			cerr << " C:" << samples[i]->costC;
			cerr << " D:" << samples[i]->costD;
			cerr << " E:" << samples[i]->costE << endl;;
		}
		cerr << endl;
	}

	virtual void updateWithInputs(string pcurrentModule,
		int pstorageA, int pstorageB, int pstorageC, int pstorageD, int pstorageE,
		int pexpertiseA, int pexpertiseB, int pexpertiseC, int pexpertiseD, int pexpertiseE)
	{
		if (pcurrentModule == "DIAGNOSIS") currentModule = DIAGNOSIS;
		else if (pcurrentModule == "MOLECULES") currentModule = MOLECULES;
		else if (pcurrentModule == "LABORATORY") currentModule = LABORATORY;
		else if (pcurrentModule == "SAMPLES") currentModule = SAMPLES;
		else currentModule = NONE;
		molA = pstorageA; expA = pexpertiseA;
		molB = pstorageB; expB = pexpertiseB;
		molC = pstorageC; expC = pexpertiseC;
		molD = pstorageD; expD = pexpertiseD;
		molE = pstorageE; expE = pexpertiseE;
		moleculeCounter = pstorageA + pstorageB + pstorageC + pstorageD + pstorageE;
	}
	virtual void updateCostsWithExpertise()
	{
		samples[0]->updateCostsWithExpertise(expA, expB, expC, expD, expE);
		samples[1]->updateCostsWithExpertise(expA, expB, expC, expD, expE);
		samples[2]->updateCostsWithExpertise(expA, expB, expC, expD, expE);
	}

	void setSamplesScore()
	{
		for (int i = 0; i < 3; ++i)
		{
			if(samples[i]->totalCost > 0) samples[i]->score = (samples[i]->hp * 100) / samples[i]->totalCost;		
			else samples[i]->score = -1;
		}
	}
	void sortSamplesByScore()
	{
		int i, j;
		for (i = 1; i < 3; ++i)
		{
			Sample s = *samples[i];
			for (j = i; j > 0 && samples[j - 1]->score < s.score; j--)
				samples[i]->copy(*samples[j - 1]);
			samples[j]->copy(s);
		}
	}
	void setSamplesOrder()
	{
		setSamplesScore();
		sortSamplesByScore();	
	}
	

	MoleculeType selectBestMoleculeV2()
	{
		MoleculeType bestMol = UNKNOWN;


		int eGainA = 0, eGainB = 0, eGainC = 0, eGainD = 0, eGainE = 0;
		for (int i = 1; i < 3; ++i)
		{
			if (myRobot->samples[0]->isComplete)
			{
				switch (myRobot->samples[0]->expGain)
				{
				case A:	eGainA += 1;
				case B:	eGainB += 1;
				case C:	eGainC += 1;
				case D:	eGainD += 1;
				case E:	eGainE += 1;
				}
				samples[i]->costA += samples[0]->costA - eGainA;
				samples[i]->costB += samples[0]->costB - eGainB;
				samples[i]->costC += samples[0]->costC - eGainC;
				samples[i]->costD += samples[0]->costD - eGainD;
				samples[i]->costE += samples[0]->costE - eGainE;
			}
		}

		if (!myRobot->samples[0]->isComplete)
		{
			if (myRobot->samples[1]->isComplete)
			{
				if (availableA > 0 && samples[0]->costA + samples[1]->costA + samples[2]->costA > molA) bestMol = A;
				if (availableB > 0 && samples[0]->costB + samples[1]->costB + samples[2]->costB > molB) bestMol = B;
				if (availableC > 0 && samples[0]->costC + samples[1]->costC + samples[2]->costC > molC) bestMol = C;
				if (availableD > 0 && samples[0]->costD + samples[1]->costD + samples[2]->costD > molD) bestMol = D;
				if (availableE > 0 && samples[0]->costE + samples[1]->costE + samples[2]->costE > molE) bestMol = E;

			}

		}

		return bestMol;
	}

	MoleculeType selectBestMolecule()
	{
		MoleculeType bestMol = UNKNOWN;
		
		// Pick the last molecule so he cant get enough to complete
		if (bestMol == UNKNOWN)
		{
			for (int i = 0; i < 3; ++i)
			{
				// He needs 2 to complete
				if (opRobot->molA + 2 == opRobot->samples[i]->costA && availableA == 2) bestMol = A;
				if (opRobot->molB + 2 == opRobot->samples[i]->costB && availableB == 2) bestMol = B;
				if (opRobot->molC + 2 == opRobot->samples[i]->costC && availableC == 2) bestMol = C;
				if (opRobot->molD + 2 == opRobot->samples[i]->costD && availableD == 2) bestMol = D;
				if (opRobot->molE + 2 == opRobot->samples[i]->costE && availableE == 2) bestMol = E;
			}
		}
		if (bestMol != UNKNOWN) cerr << "bestMol Pick the last molecule so he cant get enough to complete " << bestMol << endl;
		// If all op samples needs more than 2 molecule of same type, pick 3 so he cant complete any sample
		if (bestMol == UNKNOWN)
		{
			switch (opRobot->sampleCounter)
			{
				//case 1:
				//case 2:
				//  if (availableA > 0 && myRobot->molA < 2 && opRobot->samples[0]->costA > 2 && opRobot->samples[1]->costA > 2) bestMol = A;
				//  if (availableB > 0 && myRobot->molB < 2 && opRobot->samples[0]->costB > 2 && opRobot->samples[1]->costB > 2) bestMol = B;
				//  if (availableC > 0 && myRobot->molC < 2 && opRobot->samples[0]->costC > 2 && opRobot->samples[1]->costC > 2) bestMol = C;
				//  if (availableD > 0 && myRobot->molD < 2 && opRobot->samples[0]->costD > 2 && opRobot->samples[1]->costD > 2) bestMol = D;
				//  if (availableE > 0 && myRobot->molE < 2 && opRobot->samples[0]->costE > 2 && opRobot->samples[1]->costE > 2) bestMol = E;
				//  break;
			case 3:
				if (availableA > 0 && myRobot->molA < 2 && opRobot->samples[0]->costA > 2 && opRobot->samples[1]->costA > 2 && opRobot->samples[2]->costA > 2) bestMol = A;
				if (availableB > 0 && myRobot->molB < 2 && opRobot->samples[0]->costB > 2 && opRobot->samples[1]->costB > 2 && opRobot->samples[2]->costB > 2) bestMol = B;
				if (availableC > 0 && myRobot->molC < 2 && opRobot->samples[0]->costC > 2 && opRobot->samples[1]->costC > 2 && opRobot->samples[2]->costC > 2) bestMol = C;
				if (availableD > 0 && myRobot->molD < 2 && opRobot->samples[0]->costD > 2 && opRobot->samples[1]->costD > 2 && opRobot->samples[2]->costD > 2) bestMol = D;
				if (availableE > 0 && myRobot->molE < 2 && opRobot->samples[0]->costE > 2 && opRobot->samples[1]->costE > 2 && opRobot->samples[2]->costE > 2) bestMol = E;
				break;
			}
		}
		if (bestMol != UNKNOWN) cerr << "bestMol all " << bestMol << endl;
		// Pick a molecule so he cant get enough to complete the samples with req 5
		if (bestMol == UNKNOWN)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (opRobot->samples[i]->costA == 5 && availableA == 5) bestMol = A;
				if (opRobot->samples[i]->costB == 5 && availableB == 5) bestMol = B;
				if (opRobot->samples[i]->costC == 5 && availableC == 5) bestMol = C;
				if (opRobot->samples[i]->costD == 5 && availableD == 5) bestMol = D;
				if (opRobot->samples[i]->costE == 5 && availableE == 5) bestMol = E;
			}
		}
		if (bestMol != UNKNOWN) cerr << "bestMol req 5 " << bestMol << endl;
		
		// Pick needed molecules for samples that cannot be blocked
		/* if (bestMol == UNKNOWN)
		{
		int bestMols[3];
		for (int i = 0; i < 3; ++i)
		{
		if (samples[i]->canComplete && !samples[i]->isComplete)
		{
		// If the opponent cannot take enough molecules to block us
		if(samples[i]->costA - molA < availableA/2 &&
		samples[i]->costB - molB < availableB/2 &&
		samples[i]->costC - molC < availableC/2 &&
		samples[i]->costD - molD < availableD/2 &&
		samples[i]->costE - molE < availableE/2 )
		{
		// List the remaining molecule number before its too late
		MoleculeStack remainings[5];
		remainings[0].type = A;  remainings[0].nb = (availableA > 0 && samples[i]->costA > 0) ? availableA - samples[i]->costA + molA : 10;
		remainings[1].type = B;  remainings[1].nb = (availableB > 0 && samples[i]->costB > 0) ? availableB - samples[i]->costB + molB : 10;
		remainings[2].type = C;  remainings[2].nb = (availableC > 0 && samples[i]->costC > 0) ? availableC - samples[i]->costC + molC : 10;
		remainings[3].type = D;  remainings[3].nb = (availableD > 0 && samples[i]->costD > 0) ? availableD - samples[i]->costD + molD : 10;
		remainings[4].type = E;  remainings[4].nb = (availableE > 0 && samples[i]->costE > 0) ? availableE - samples[i]->costE + molE : 10;

		// Sort remaining molecules
		int k, j;
		for (k = 1; k < 5; ++k)
		{
		MoleculeStack m = remainings[k];
		for (j = k; j > 0 && remainings[j - 1].nb > m.nb; j--)
		remainings[j] = remainings[j - 1];
		remainings[j] = m;
		}
		//for (k = 0; k < 5; ++k)
		//{
		//	cerr << "remainings[k] " << k << " remainings[k] " << remainings[k].type << endl;
		//	cerr << "remainings[k] " << k << " remainings[k] " << remainings[k].nb << endl;
		//}
		bestMol = remainings[0].type;
		cerr << "cannot be blocked " << i << " best " << bestMol << endl;
		}
		}
		}

		for (int i = 0; i < 3; ++i)
		{
		}

		}*/
		// Pick needed molecules with lowest totalcost sample
		/*if (bestMol == UNKNOWN)
		{
		// Sample 0 lower cost
		if (samples[0]->canComplete && samples[0]->totalCost < samples[1]->totalCost && samples[0]->totalCost < samples[2]->totalCost)
		{
		if (availableA > 0 && samples[0]->costA > molA) bestMol = A;
		else if (availableB > 0 && samples[0]->costB > molB) bestMol = B;
		else if (availableC > 0 && samples[0]->costC > molC) bestMol = C;
		else if (availableD > 0 && samples[0]->costD > molD) bestMol = D;
		else if (availableE > 0 && samples[0]->costE > molE) bestMol = E;
		}
		else // Sample 1 lower cost
		if (samples[1]->canComplete && samples[1]->totalCost < samples[0]->totalCost && samples[1]->totalCost < samples[2]->totalCost)
		{
		if (availableA > 0 && samples[1]->costA > molA) bestMol = A;
		else if (availableB > 0 && samples[1]->costB > molB) bestMol = B;
		else if (availableC > 0 && samples[1]->costC > molC) bestMol = C;
		else if (availableD > 0 && samples[1]->costD > molD) bestMol = D;
		else if (availableE > 0 && samples[1]->costE > molE) bestMol = E;
		}
		else if (samples[2]->canComplete)// Sample 2 lower cost
		{
		if (availableA > 0 && samples[2]->costA > molA) bestMol = A;
		else if (availableB > 0 && samples[2]->costB > molB) bestMol = B;
		else if (availableC > 0 && samples[2]->costC > molC) bestMol = C;
		else if (availableD > 0 && samples[2]->costD > molD) bestMol = D;
		else if (availableE > 0 && samples[2]->costE > molE) bestMol = E;
		}
		}*/
		
		// Pick molecules that the opponent needs the most
		// It makes the bot pick molecules from the best sample in the order that disturbs opponent the most
		if (bestMol == UNKNOWN)
		{
			int totalOpNeed[5] = { 0,0,0,0,0 };
			for (int i = 0; i < 3; ++i)
			{
				// total he needs
				totalOpNeed[0] += opRobot->samples[i]->costA;
				totalOpNeed[1] += opRobot->samples[i]->costB;
				totalOpNeed[2] += opRobot->samples[i]->costC;
				totalOpNeed[3] += opRobot->samples[i]->costD;
				totalOpNeed[4] += opRobot->samples[i]->costE;
			}
			// get mostNeeded molecule type
			int mostNeeded = -1, counterNeed = 0;
			for (int i = 0; i < 5; ++i)
			{
				if(totalOpNeed[i] > counterNeed)
				{
					counterNeed = totalOpNeed[i];
					mostNeeded = i;
				}
			}
			// Look if we need this type too, for our bestScore sample
			switch (MoleculeType(mostNeeded))
			{
			case A: if (availableA > 0 && !samples[0]->isComplete && samples[0]->costA > molA) bestMol = A; break;
			case B:	if (availableB > 0 && !samples[0]->isComplete && samples[0]->costB > molB) bestMol = B; break;
			case C:	if (availableC > 0 && !samples[0]->isComplete && samples[0]->costC > molC) bestMol = C; break;
			case D:	if (availableD > 0 && !samples[0]->isComplete && samples[0]->costD > molD) bestMol = D; break;
			case E:	if (availableE > 0 && !samples[0]->isComplete && samples[0]->costE > molE) bestMol = E; break;
			}
		}
		if (bestMol != UNKNOWN) cerr << "bestMol opNeeded " << bestMol << endl;
		
		
		// Pick needed molecules
		if (bestMol == UNKNOWN)
		{
			if (availableA > 0 && samples[0]->id != -1 && !samples[0]->isComplete && samples[0]->costA > molA) bestMol = A;
			else if (availableB > 0 && samples[0]->id != -1 && !samples[0]->isComplete && samples[0]->costB > molB) bestMol = B;
			else if (availableC > 0 && samples[0]->id != -1 && !samples[0]->isComplete && samples[0]->costC > molC) bestMol = C;
			else if (availableD > 0 && samples[0]->id != -1 && !samples[0]->isComplete && samples[0]->costD > molD) bestMol = D;
			else if (availableE > 0 && samples[0]->id != -1 && !samples[0]->isComplete && samples[0]->costE > molE) bestMol = E;
			else if (availableA > 0 && samples[1]->id != -1 && !samples[1]->isComplete && samples[1]->costA > molA) bestMol = A;
			else if (availableB > 0 && samples[1]->id != -1 && !samples[1]->isComplete && samples[1]->costB > molB) bestMol = B;
			else if (availableC > 0 && samples[1]->id != -1 && !samples[1]->isComplete && samples[1]->costC > molC) bestMol = C;
			else if (availableD > 0 && samples[1]->id != -1 && !samples[1]->isComplete && samples[1]->costD > molD) bestMol = D;
			else if (availableE > 0 && samples[1]->id != -1 && !samples[1]->isComplete && samples[1]->costE > molE) bestMol = E;
			else if (availableA > 0 && samples[2]->id != -1 && !samples[2]->isComplete && samples[2]->costA > molA) bestMol = A;
			else if (availableB > 0 && samples[2]->id != -1 && !samples[2]->isComplete && samples[2]->costB > molB) bestMol = B;
			else if (availableC > 0 && samples[2]->id != -1 && !samples[2]->isComplete && samples[2]->costC > molC) bestMol = C;
			else if (availableD > 0 && samples[2]->id != -1 && !samples[2]->isComplete && samples[2]->costD > molD) bestMol = D;
			else if (availableE > 0 && samples[2]->id != -1 && !samples[2]->isComplete && samples[2]->costE > molE) bestMol = E;
		}

		cerr << "isComplete!! " << myRobot->samples[0]->isComplete << myRobot->samples[1]->isComplete << myRobot->samples[2]->isComplete << endl;
		if (bestMol != UNKNOWN) cerr << "bestMol needed " << bestMol << endl;
		/*for (int i = 1; i < 3; ++i)
		{
		if (bestMol == UNKNOWN && samples[0]->isComplete && samples[i]->isComplete && samples[0]->canComplete && samples[i]->canComplete)
		{
		int sTotalCost = samples[0]->costA + samples[i]->costA;
		sTotalCost += samples[0]->costB + samples[i]->costB;
		sTotalCost += samples[0]->costC + samples[i]->costC;
		sTotalCost += samples[0]->costD + samples[i]->costD;
		sTotalCost += samples[0]->costE + samples[i]->costE;
		if (sTotalCost < 10)
		{
		if (availableA > 0 && samples[0]->costA + samples[i]->costA > molA) bestMol = A;
		else if (availableB > 0 && samples[0]->costB + samples[i]->costB > molB) bestMol = B;
		else if (availableC > 0 && samples[0]->costC + samples[i]->costC > molC) bestMol = C;
		else if (availableD > 0 && samples[0]->costD + samples[i]->costD > molD) bestMol = D;
		else if (availableE > 0 && samples[0]->costE + samples[i]->costE > molE) bestMol = E;
		}
		}
		}
		if (bestMol == UNKNOWN && samples[1]->isComplete && samples[2]->isComplete && samples[1]->canComplete && samples[2]->canComplete)
		{
		int sTotalCost = samples[1]->costA + samples[2]->costA;
		sTotalCost += samples[1]->costB + samples[2]->costB;
		sTotalCost += samples[1]->costC + samples[2]->costC;
		sTotalCost += samples[1]->costD + samples[2]->costD;
		sTotalCost += samples[1]->costE + samples[2]->costE;
		if (sTotalCost < 10)
		{
		if (availableA > 0 && samples[1]->costA + samples[2]->costA > molA) bestMol = A;
		else if (availableB > 0 && samples[1]->costB + samples[2]->costB > molB) bestMol = B;
		else if (availableC > 0 && samples[1]->costC + samples[2]->costC > molC) bestMol = C;
		else if (availableD > 0 && samples[1]->costD + samples[2]->costD > molD) bestMol = D;
		else if (availableE > 0 && samples[1]->costE + samples[2]->costE > molE) bestMol = E;
		}
		}*/

		return bestMol;
	}

	bool canCompleteNothing()
	{
		return (
			(sampleCounter == 1 && !samples[0]->canComplete) ||
			(sampleCounter == 2 && !samples[0]->canComplete && !samples[1]->canComplete) ||
			(sampleCounter == 3 && !samples[0]->canComplete && !samples[1]->canComplete && !samples[2]->canComplete)
			);
	}

	void setSampleData()
	{
		for (int i = 0; i<3; ++i)
		{
			if (molA < samples[i]->costA ||
				molB < samples[i]->costB ||
				molC < samples[i]->costC ||
				molD < samples[i]->costD ||
				molE < samples[i]->costE) samples[i]->isComplete = false;

			if (samples[i]->costA - molA <= availableA &&
				samples[i]->costB - molB <= availableB &&
				samples[i]->costC - molC <= availableC &&
				samples[i]->costD - molD <= availableD &&
				samples[i]->costE - molE <= availableE) samples[i]->canComplete = true;
			/*{
			int totalNeeded = 0;
			totalNeeded += (samples[i]->costA > 0) ? samples[i]->costA - molA : 0;
			totalNeeded += (samples[i]->costB > 0) ? samples[i]->costB - molB : 0;
			totalNeeded += (samples[i]->costC > 0) ? samples[i]->costC - molC : 0;
			totalNeeded += (samples[i]->costD > 0) ? samples[i]->costD - molD : 0;
			totalNeeded += (samples[i]->costE > 0) ? samples[i]->costE - molE : 0;
			if(totalNeeded+molA+molB+molC+molD+molE <= 10 ) samples[i]->canComplete = true;
			cerr << "totalNeeded " << totalNeeded << endl;
			}*/
			if (!samples[i]->diagnosed) samples[i]->canComplete = true;
		}

		for (int i = 1; i < 3; ++i)
		{
			int eGainA = 0, eGainB = 0, eGainC = 0, eGainD = 0, eGainE = 0;
			if (myRobot->samples[0]->isComplete)
			{
				switch (myRobot->samples[0]->expGain)
				{
				case A:	eGainA += 1; break;
				case B:	eGainB += 1; break;
				case C:	eGainC += 1; break;
				case D:	eGainD += 1; break;
				case E:	eGainE += 1; break;
				}
				samples[i]->costA += samples[0]->costA - eGainA;
				samples[i]->costB += samples[0]->costB - eGainB;
				samples[i]->costC += samples[0]->costC - eGainC;
				samples[i]->costD += samples[0]->costD - eGainD;
				samples[i]->costE += samples[0]->costE - eGainE;
			}
		}
		for (int i = 1; i < 3; ++i)
		{
			if (molA < samples[i]->costA ||
				molB < samples[i]->costB ||
				molC < samples[i]->costC ||
				molD < samples[i]->costD ||
				molE < samples[i]->costE) samples[i]->isComplete = false;
		}
		if (myRobot->samples[0]->isComplete && myRobot->samples[1]->isComplete)
		{
			int eGainA = 0, eGainB = 0, eGainC = 0, eGainD = 0, eGainE = 0;
			switch (myRobot->samples[1]->expGain)
			{
			case A:	eGainA += 1; break;
			case B:	eGainB += 1; break;
			case C:	eGainC += 1; break;
			case D:	eGainD += 1; break;
			case E:	eGainE += 1; break;
			}
			samples[2]->costA += samples[1]->costA - eGainA;
			samples[2]->costB += samples[1]->costB - eGainB;
			samples[2]->costC += samples[1]->costC - eGainC;
			samples[2]->costD += samples[1]->costD - eGainD;
			samples[2]->costE += samples[1]->costE - eGainE;
		}

		if (molA < samples[2]->costA ||
			molB < samples[2]->costB ||
			molC < samples[2]->costC ||
			molD < samples[2]->costD ||
			molE < samples[2]->costE) samples[2]->isComplete = false;
	}



	int movingTurns;
	int sampleCounter;
	int moleculeCounter;
	int molA, molB, molC, molD, molE;
	int expA, expB, expC, expD, expE;
	Sample *samples[3];
	ModuleType currentModule;
};




void initDefault()
{
	myRobot = new Robot();
	opRobot = new Robot();
	for (int i = 0; i < MAX_SAMPLE; ++i)
	{
		myRobot->samples[i] = new Sample(0, ME);
		opRobot->samples[i] = new Sample(0, OPPONENT);
	}
	for (int i = 0; i < PROJECT_NB; ++i)
	{
		project[i] = new Project();
	}

	for (int i = 0; i < 100; ++i)
	{
		allSamples[i] = new Sample(0, CLOUD);
		cloudSamples[i] = new Sample(0, CLOUD);
	}
	myRobot->movingTurns = 0;
}
void resetBeforeInputs()
{
	sampleC = 0;
	cloudSampleC = 0;
	myRobot->reset();
	opRobot->reset();

	for (int i = 0; i < 100; ++i)
	{
		allSamples[i]->reset();
		cloudSamples[i]->reset();
	}
}
void getInputProjects()
{
	int projectCount;
	cin >> projectCount; cin.ignore();
	for (int i = 0; i < projectCount; i++)
	{
		int a, b, c, d, e;
		cin >> a >> b >> c >> d >> e; cin.ignore();
		project[i]->updateWithInputs(a, b, c, d, e);
	}
}
void getInputEntities()
{
	string target;
	int eta;
	int storageA, storageB, storageC, storageD, storageE;
	int expertiseA, expertiseB, expertiseC, expertiseD, expertiseE;
	cin >> target >> eta >> myScore >> storageA >> storageB >> storageC >> storageD >> storageE >> expertiseA >> expertiseB >> expertiseC >> expertiseD >> expertiseE; cin.ignore();
	myRobot->updateWithInputs(target, storageA, storageB, storageC, storageD, storageE,
		expertiseA, expertiseB, expertiseC, expertiseD, expertiseE);
	cin >> target >> eta >> opScore >> storageA >> storageB >> storageC >> storageD >> storageE >> expertiseA >> expertiseB >> expertiseC >> expertiseD >> expertiseE; cin.ignore();
	opRobot->updateWithInputs(target, storageA, storageB, storageC, storageD, storageE,
		expertiseA, expertiseB, expertiseC, expertiseD, expertiseE);

	cin >> availableA >> availableB >> availableC >> availableD >> availableE; cin.ignore();

	cin >> sampleCount; cin.ignore();
	cerr << "sampleCount " << sampleCount << endl;
	for (int i = 0; i < sampleCount; i++)
	{
		int sampleId, carriedBy, rank;
		string expertiseGain;
		int health;
		int costA, costB, costC, costD, costE;
		cin >> sampleId >> carriedBy >> rank >> expertiseGain >> health >> costA >> costB >> costC >> costD >> costE; cin.ignore();

		allSamples[sampleC]->updateWithInputs(sampleId, (CarrierType)carriedBy, health, expertiseGain,
			costA, costB, costC, costD, costE);

		switch (allSamples[sampleC]->carrier)
		{
		case ME:    myRobot->samples[myRobot->sampleCounter]->copy(*allSamples[sampleC]);
			myRobot->sampleCounter++;
			break;
		case OPPONENT:  opRobot->samples[opRobot->sampleCounter]->copy(*allSamples[sampleC]);
			opRobot->sampleCounter++;
			break;
		case CLOUD: cloudSamples[cloudSampleC++]->copy(*allSamples[sampleC]);
			break;
		}
		allSamples[sampleC]->print();

		++sampleC;
	}
}

void sortSampleByHP()
{
	int i, j;
	for (i = 1; i < 100; ++i)
	{
		Sample m = *allSamples[i];
		for (j = i; j > 0 && allSamples[j - 1]->hp < m.hp; j--)
			allSamples[j] = allSamples[j - 1];
		*allSamples[j] = m;
	}
}
void outputBestMol(MoleculeType bestMol)
{
	switch (bestMol)
	{
	case A: cout << "CONNECT A" << endl; break;
	case B: cout << "CONNECT B" << endl; break;
	case C: cout << "CONNECT C" << endl; break;
	case D: cout << "CONNECT D" << endl; break;
	case E: cout << "CONNECT E" << endl; break;
	}

}
/**
* Bring data on patient samples from the diagnosis machine to the laboratory with enough molecules to produce medicine!
**/
int main()
{
	initDefault();
	getInputProjects();
	// game loop
	while (1)
	{
		++turns;
		resetBeforeInputs();
		getInputEntities();

		myRobot->updateCostsWithExpertise();
		opRobot->updateCostsWithExpertise();
		if (myRobot->movingTurns > 0) myRobot->movingTurns--;

		//project[0]->print();
		//project[1]->print();
		//project[2]->print();	
		myRobot->setSamplesOrder();
		myRobot->setSampleData();	
		myRobot->print();
		
		sortSampleByHP();


		cerr << "isComplete " << myRobot->samples[0]->isComplete << myRobot->samples[1]->isComplete << myRobot->samples[2]->isComplete << endl;
		cerr << "canComplete" << myRobot->samples[0]->canComplete << myRobot->samples[1]->canComplete << myRobot->samples[2]->canComplete << endl;


		int foundInTheCloud = -1;
		int cloudScore = 0;
		int bestCloudScore = 0;
		for (int i = 0; i < cloudSampleC; ++i)
		{
			if (cloudSamples[i]->hp > 1)
			{
				if (cloudSamples[i]->costA - myRobot->expA <= availableA &&
					cloudSamples[i]->costB - myRobot->expB <= availableB &&
					cloudSamples[i]->costC - myRobot->expC <= availableC &&
					cloudSamples[i]->costD - myRobot->expD <= availableD &&
					cloudSamples[i]->costE - myRobot->expE <= availableE)
				{

					cloudScore = cloudSamples[i]->hp - cloudSamples[i]->totalCost;

					if (myRobot->molA < cloudSamples[i]->costA ||
						myRobot->molB < cloudSamples[i]->costB ||
						myRobot->molC < cloudSamples[i]->costC ||
						myRobot->molD < cloudSamples[i]->costD ||
						myRobot->molE < cloudSamples[i]->costE) cloudSamples[i]->isComplete = false;
					if (myRobot->moleculeCounter == 10 && !cloudSamples[i]->isComplete) cloudScore = 0;

					if (cloudScore > bestCloudScore)
					{
						foundInTheCloud = cloudSamples[i]->id;
						bestCloudScore = cloudScore;
					}
				}
			}
		}




		cerr << "foundInTheCloud " << foundInTheCloud << endl;

		if (myRobot->movingTurns == 0)
		{
			switch (myRobot->currentModule)
			{
			case NONE:
				cout << "GOTO SAMPLES" << endl;
				myRobot->movingTurns = 2;
				break;

			case SAMPLES:

				cerr << "sampleCounter " << myRobot->sampleCounter << endl;
				if (myRobot->sampleCounter == 2 && foundInTheCloud != -1)
				{
					cout << "GOTO DIAGNOSIS" << endl;
					myRobot->movingTurns = 3;
				}
				else if (myRobot->sampleCounter == 3)
				{
					cout << "GOTO DIAGNOSIS" << endl;
					myRobot->movingTurns = 3;
				}
				else
				{

					if (myScore == 0) cout << "CONNECT " << 1 << endl;
					else if (myRobot->expA + myRobot->expB + myRobot->expC + myRobot->expD + myRobot->expE < 6) cout << "CONNECT " << 1 << endl;
					else if (myRobot->expA > 4 &&
						myRobot->expB > 4 &&
						myRobot->expC > 4 &&
						myRobot->expD > 4 &&
						myRobot->expE > 4)
					{
						cout << "CONNECT " << 3 << endl;
					}
					else if (myRobot->expA + myRobot->expB + myRobot->expC + myRobot->expD + myRobot->expE> 12) cout << "CONNECT " << 3 << endl;
					else cout << "CONNECT " << 2 << endl;
				}

				break;

			case MOLECULES:
				cerr << "Current Module MOLECULES" << endl;

				if (myRobot->sampleCounter == 0)
				{
					cout << "GOTO SAMPLES" << endl;
					myRobot->movingTurns = 3;
				}
				else if (myRobot->moleculeCounter == 10 &&
					((myRobot->sampleCounter == 1 && !myRobot->samples[0]->isComplete) ||
					(myRobot->sampleCounter == 2 && !myRobot->samples[0]->isComplete && !myRobot->samples[1]->isComplete) ||
						(myRobot->sampleCounter == 3 && !myRobot->samples[0]->isComplete && !myRobot->samples[1]->isComplete && !myRobot->samples[2]->isComplete)
						)
					)
				{
					// We are full 10 molecules but cant complete any sample
					if (myRobot->sampleCounter == 3)
					{
						cout << "GOTO DIAGNOSIS" << endl;
						myRobot->movingTurns = 3;
					}
					else
					{
						cout << "GOTO SAMPLES" << endl;
						myRobot->movingTurns = 3;
					}
				}
				else
				{
					if (myRobot->canCompleteNothing())
					{
						if (myRobot->sampleCounter == 3 || foundInTheCloud != -1)
						{
							cout << "GOTO DIAGNOSIS" << endl;
							myRobot->movingTurns = 3;
						}
						else
						{
							cout << "GOTO SAMPLES" << endl;
							myRobot->movingTurns = 3;
						}
					}
					else if (myRobot->moleculeCounter < 10)
					{
						MoleculeType bestMol = myRobot->selectBestMolecule();
						if (bestMol != UNKNOWN)
						{
							outputBestMol(bestMol);
						}
						else
						{
							cout << "GOTO LABORATORY Lab" << endl;
							myRobot->movingTurns = 3;
						}
					}
					else
					{
						cout << "GOTO LABORATORY" << endl;
						myRobot->movingTurns = 3;
					}
				}
				break;
			case DIAGNOSIS:
				cerr << "Current Module DIAGNOSIS" << endl;

				if (foundInTheCloud != -1 && myRobot->sampleCounter < 3)
				{
					cout << "CONNECT " << foundInTheCloud << endl;
				}
				else if (myRobot->sampleCounter > 0)
				{
					if (myRobot->moleculeCounter == 10 && myRobot->sampleCounter == 3 &&
						!myRobot->samples[0]->isComplete &&
						!myRobot->samples[1]->isComplete &&
						!myRobot->samples[2]->isComplete)
					{
						// We are full 10 molecules but cant complete any sample
						cout << "CONNECT " << myRobot->samples[0]->id << endl;
					}
					else if (myRobot->sampleCounter == 3 &&
						!myRobot->samples[0]->canComplete &&
						!myRobot->samples[1]->canComplete &&
						!myRobot->samples[2]->canComplete)
					{
						cout << "CONNECT " << myRobot->samples[0]->id << endl;
					}
					else
					{
						if (!myRobot->samples[0]->diagnosed) cout << "CONNECT " << myRobot->samples[0]->id << endl;
						else if (!myRobot->samples[1]->diagnosed) cout << "CONNECT " << myRobot->samples[1]->id << endl;
						else if (!myRobot->samples[2]->diagnosed) cout << "CONNECT " << myRobot->samples[2]->id << endl;
						else if (myRobot->sampleCounter == 3)
						{
							//if (myRobot->samples[0]->hp == 1 && 
							//	(	myRobot->samples[0]->costA > 3 ||
							//		myRobot->samples[0]->costB > 3 ||
							//		myRobot->samples[0]->costC > 3 ||
							//		myRobot->samples[0]->costD > 3 ||
							//		myRobot->samples[0]->costE > 3))
							//{
							//	cout << "CONNECT " << myRobot->samples[0]->id << endl; // Bad sample rank 1 in the cloud
							//}
							//else
							{
								cout << "GOTO MOLECULES" << endl;
								myRobot->movingTurns = 3;
							}
						}
						else if (myRobot->sampleCounter == 2 && !myRobot->canCompleteNothing())
						{
							cout << "GOTO MOLECULES" << endl;
							myRobot->movingTurns = 3;
						}
						else
						{
							cout << "GOTO SAMPLES" << endl;
							myRobot->movingTurns = 3;
						}
					}
				}
				else
				{
					if (foundInTheCloud == -1)
					{
						cout << "GOTO SAMPLES" << endl;
						myRobot->movingTurns = 3;
					}
					else  cout << "CONNECT " << foundInTheCloud << endl;
				}

				break;
			case LABORATORY:
				cerr << "Current Module LABORATORY" << endl;

				if (myRobot->sampleCounter > 0)
				{
					if (myRobot->samples[0]->isComplete)
					{
						cout << "CONNECT " << myRobot->samples[0]->id << endl;
						myRobot->samples[0]->copy(*myRobot->samples[1]);
						myRobot->samples[1]->copy(*myRobot->samples[2]);
					}
					else if (myRobot->sampleCounter > 1 &&
						myRobot->samples[1]->isComplete)
					{
						cout << "CONNECT " << myRobot->samples[1]->id << endl;
						myRobot->samples[1]->copy(*myRobot->samples[2]);
					}
					else if (myRobot->sampleCounter > 2 &&
						myRobot->samples[2]->isComplete)
					{
						cout << "CONNECT " << myRobot->samples[2]->id << endl;
					}
					else
					{
						if (myRobot->canCompleteNothing())
						{
							if (myRobot->sampleCounter == 3)
							{
								cout << "GOTO DIAGNOSIS" << endl;
								myRobot->movingTurns = 3;
							}
							else
							{
								cout << "GOTO SAMPLES" << endl;
								myRobot->movingTurns = 3;
							}
						}
						else if (myRobot->sampleCounter == 1 && myRobot->samples[0]->hp < 20)
						{
							cout << "GOTO SAMPLES" << endl;
							myRobot->movingTurns = 3;
						}
						else
						{
							cout << "GOTO MOLECULES" << endl;
							myRobot->movingTurns = 3;
						}
					}
				}
				else
				{
					cout << "GOTO SAMPLES" << endl;
					myRobot->movingTurns = 3;
				}
				break;
			}

		}
		else cout << "WAIT" << endl;
	}
}
