#include "cli_parser.h"
#include "fuzzing_application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        CliOptions options = CliParser::parse(argc, argv);
        
        // Handle help request
        if (options.show_help) {
            CliParser::printUsage(argv[0]);
            return 0;
        }
        
        // Validate options
        if (!CliParser::validateOptions(options)) {
            CliParser::printUsage(argv[0]);
            return 1;
        }
        
        // Create and run the fuzzing application
        FuzzingApplication app(options);
        return app.run();
        
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        CliParser::printUsage(argv[0]);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
