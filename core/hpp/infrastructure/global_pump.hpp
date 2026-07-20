#ifndef GLOBAL_PUMP_HPP
#define GLOBAL_PUMP_HPP

template<typename IGetNextGlobal, typename IProcessGlobal>
struct global_pump {
    global_pump(IGetNextGlobal& get_next_global, IProcessGlobal& process_global);

    void pump();

private:
    IGetNextGlobal&  get_next_global_;
    IProcessGlobal&  process_global_;
};

template<typename IGN, typename IPG>
global_pump<IGN, IPG>::global_pump(IGN& get_next_global, IPG& process_global)
    : get_next_global_(get_next_global)
    , process_global_(process_global) {}

template<typename IGN, typename IPG>
void global_pump<IGN, IPG>::pump() {
    while (auto item = get_next_global_.get_next_global())
        process_global_.process_global(*item);
}

#endif
