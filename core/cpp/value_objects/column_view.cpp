#include "value_objects/column_view.hpp"

#include <numeric>

namespace AML {

bool column_view::operator==(const column_view& other) const {
    if (type != other.type || rows->size() != other.rows->size())
        return false;
    for (size_t row = 0; row < rows->size(); ++row) {
        if ((*cells)[(*rows)[row]] != (*other.cells)[(*other.rows)[row]])
            return false;
    }
    return true;
}

column_view column_view::of(typed_column dense) {
    std::vector<size_t> identity(dense.cells.size());
    std::iota(identity.begin(), identity.end(), size_t{0});
    return column_view{
        .type = dense.type,
        .cells = std::make_shared<const column>(std::move(dense.cells)),
        .rows = std::make_shared<const std::vector<size_t>>(std::move(identity))};
}

}
