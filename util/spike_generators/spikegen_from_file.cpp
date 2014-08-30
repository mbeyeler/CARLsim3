#include <spikegen_from_file.h>

#include <carlsim.h>
//#include <user_errors.h>		// fancy user error messages

#include <stdio.h>				// fopen, fread, fclose
#include <string.h>				// std::string


SpikeGeneratorFromFile::SpikeGeneratorFromFile(std::string fileName) {
	fileName_ = fileName;
	fpBegin_ = NULL;
	fpNeur_ = NULL;
	needToInit_ = true;

	// move unsafe operations out of constructor
	openFile();
}

SpikeGeneratorFromFile::~SpikeGeneratorFromFile() {
	if (fpNeur_!=NULL) delete[] fpNeur_;
	fclose(fpBegin_);
}


void SpikeGeneratorFromFile::openFile() {
	std::string funcName = "openFile("+fileName_+")";
	fpBegin_ = fopen(fileName_.c_str(),"rb");
	UserErrors::assertTrue(fpBegin_!=NULL, UserErrors::FILE_CANNOT_OPEN, funcName, fileName_);

	// \TODO add signature to spike file so that we can make sure we have the right file
}

unsigned int SpikeGeneratorFromFile::nextSpikeTime(CARLsim* sim, int grpId, int nid, unsigned int currentTime, 
	unsigned int lastScheduledSpikeTime) {

	if (needToInit_) {
		int nNeur = sim->getGroupNumNeurons(grpId);

		// first time we call this function, create array to store file pointer of last read for each neuron
		fpNeur_ = new FILE*[nNeur];
		for (int i=0; i<nNeur; i++)
			fpNeur_[i] = fpBegin_;

		needToInit_ = false;
	}

	FILE* fp = fpNeur_[nid]; // current fp for this neuron
	int tmpTime = -1;
	int tmpNeurId = -1;

	// read the next time and neuron ID in the file
	fread(&tmpTime, sizeof(int), 1, fp); // i-th time
	fread(&tmpNeurId, sizeof(int), 1, fp); // i-th nid

	// chances are this neuron ID is not the one we want, so we have to keep reading until we find the right one
	while (tmpNeurId!=nid && !feof(fp)) {
		fread(&tmpTime, sizeof(int), 1, fp); // j-th time
		fread(&tmpNeurId, sizeof(int), 1, fp); // j-th nid
	}

	// once we found the right neuron ID, store the fp for the next iteration (so we don't schedule the
	// same spike twice)
	fpNeur_[nid] = fp;

	// if eof was reached, there are no more spikes for this neuron ID
	if (feof(fp))
		return -1; // large pos number

	// else return the valid spike time
	return tmpTime;
}
