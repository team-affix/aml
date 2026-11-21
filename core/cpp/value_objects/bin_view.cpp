#include "value_objects/bin_view.hpp"

#include <memory>
#include <numeric>

namespace AML {

bin_view bin_view::of(const bin& dense) {
    std::vector<size_t> identity(dense.labels.cells.size());
    std::iota(identity.begin(), identity.end(), size_t{0});
    const std::shared_ptr<const std::vector<size_t>> rows =
        std::make_shared<const std::vector<size_t>>(std::move(identity));
    bin_view viewed{.inputs = {}, .labels = column_view{
        .type = dense.labels.type, .cells = std::make_shared<const column>(dense.labels.cells), .rows = rows}};
    viewed.inputs.reserve(dense.inputs.size());
    for (const typed_column& input : dense.inputs)
        viewed.inputs.push_back(
            column_view{.type = input.type, .cells = std::make_shared<const column>(input.cells), .rows = rows});
    return viewed;
}

}
