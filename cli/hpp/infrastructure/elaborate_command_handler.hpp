#ifndef ELABORATE_COMMAND_HANDLER_HPP
#define ELABORATE_COMMAND_HANDLER_HPP

#include <string>
#include <vector>

struct elaborate_command_handler {
    elaborate_command_handler(std::vector<std::string> module_paths,
                              std::vector<std::string> statement_paths,
                              std::string output_path);

    void operator()();

private:
    std::vector<std::string> module_paths_;
    std::vector<std::string> statement_paths_;
    std::string              output_path_;
};

#endif
