#ifndef STATEMENT_PUMP_HPP
#define STATEMENT_PUMP_HPP

template<typename IGetNextStatement, typename IProcessStatement>
struct statement_pump {
    statement_pump(IGetNextStatement& get_next_statement,
                   IProcessStatement& process_statement);

    void pump();

private:
    IGetNextStatement&  get_next_statement_;
    IProcessStatement&  process_statement_;
};

template<typename IGN, typename IPS>
statement_pump<IGN, IPS>::statement_pump(IGN& get_next_statement,
                                         IPS& process_statement)
    : get_next_statement_(get_next_statement)
    , process_statement_(process_statement) {}

template<typename IGN, typename IPS>
void statement_pump<IGN, IPS>::pump() {
    while (auto stmt = get_next_statement_.get_next_statement())
        process_statement_.process_statement(*stmt);
}

#endif
