#include <CLI/CLI.hpp>
#include "infrastructure/elaborate_command_handler.hpp"

#ifndef AML_GIT_TAG
#define AML_GIT_TAG "unknown"
#endif

int main(int argc, char** argv) {
    CLI::App app{"AML elaborator"};
    app.name("aml");
    app.set_version_flag("-v,--version", AML_GIT_TAG);
    app.require_subcommand(1);

    struct {
        std::vector<std::string> modules;
        std::vector<std::string> statements;
        std::string              output;
    } elaborate_opts;

    auto* elaborate_sub = app.add_subcommand(
        "elaborate", "Elaborate AML modules/statements into CHC initial goals");
    elaborate_sub->add_option("-m,--module", elaborate_opts.modules,
                              "AML module file(s)")
        ->expected(0, -1);
    elaborate_sub->add_option("-s,--statement", elaborate_opts.statements,
                              "AML statement file(s)")
        ->expected(0, -1);
    elaborate_sub->add_option("-o,--output", elaborate_opts.output,
                              "Output file for comma-separated goals "
                              "(default: print to stdout)");
    elaborate_sub->callback([&]() {
        elaborate_command_handler h(elaborate_opts.modules,
                                    elaborate_opts.statements,
                                    elaborate_opts.output);
        h();
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}
