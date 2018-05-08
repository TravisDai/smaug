#include <string>

#include <boost/program_options.hpp>

#include "modelconf/read_model_conf.h"

namespace po = boost::program_options;

using namespace smaug;

int main(int argc, char* argv[]) {
    std::string modelconf;
    po::options_description options("SMAUG options");
    options.add_options()("help", "Display this help message");

    po::options_description hidden;
    hidden.add_options()("model-config", po::value(&modelconf)->required(),
                         "Model configuration file");
    po::options_description all;
    all.add(options);
    all.add(hidden);

    po::positional_options_description p;
    p.add("model-config", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                      .options(all)
                      .positional(p)
                      .run(),
              vm);
    try {
        po::notify(vm);
    } catch (po::error& e) {
        std::cout << "ERROR: " << e.what() << "\n";
        exit(1);
    }

    if (vm.count("help")) {
        std::cout << all << "\n";
        return 1;
    }
    std::cout << "Model configuration: " << modelconf << "\n";

    Workspace* workspace = new Workspace();
    Network* network = readModelConfiguration(modelconf, workspace);
    network->dumpDataflowGraph();

    return 0;
}
