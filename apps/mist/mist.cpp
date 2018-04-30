#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <libconfig.h++>

using namespace libconfig;
using namespace std;

int main(int argc, char ** argv) {
	Config cfg;
	const char * configFile = "mist.cfg";

	try {
		cfg.readFile(configFile);
	}
	catch(const FileIOException &fioex) {
		std::cerr << "I/O error while reading file." << std::endl;
		return -1;
	}
	catch(const ParseException &pex) {
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
        		      << " - " << pex.getError() << std::endl;
		return -1;
	}

	// required parameters
	string mode;
	int port;
	cfg.lookupValue("mode", mode);
	cfg.lookupValue("port", port);

	string primaryHash, replicaHash, primaryKey;
	vector<int> keysStructure;
	cfg.lookupValue("primaryHash", primaryHash);
	cfg.lookupValue("replicaHash", replicaHash);
	const Setting &keys_settings = cfg.lookup("keys_structure");
	for (int n = 0; n < keys_settings.getLength(); ++n)
		keysStructure.push_back(keys_settings[n]);
	cfg.lookupValue("primaryKey", primaryKey);

	// optional parameters
	string pmem_path = "";
	int pmem_size = 0;
	string loggingLevel = "WARN";
	cfg.lookupValue("pmem_path", pmem_path);
	cfg.lookupValue("pmem_size", pmem_size);
	cfg.lookupValue("logging_level", loggingLevel);

	cout << "FogKV/mode=" << mode << "; file=" << pmem_path
				<< "; size=" << pmem_size
				<< endl;
	cout << "keys structure=";
	for (auto n: keysStructure)
		cout << n << " ";
	cout << endl;

	return 0;
}
