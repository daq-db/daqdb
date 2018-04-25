#include "deps/config4cpp/include/config4cpp/Configuration.h"
#include <iostream>
using namespace config4cpp;
using namespace std;

int main(int argc, char ** argv)
{
  Configuration *  cfg = Configuration::create();
  const char * mode;
  const char * configFile = "mist.cfg";
  const char * pmem_path;
  uint64 pmem_size = 0;

  try {
    cfg->parse(configFile);
    mode = cfg->lookupString(scope, "mode");
    pmem_path = cfg->lookupString(scope, "pmem_path");
    pmem_size = cfg->lookupBoolean(scope, "pmem-size");
  } 
  catch(const ConfigurationException & ex) {
    cerr << ex.c_str() << endl;
    cfg->destroy();
    return 1;
  }

  cout << "mode=" << mode << "; fille=" << pmem_path
	  << "; size=" << pmem_size
	  << endl;
  cfg->destroy();
  return 0;
}
