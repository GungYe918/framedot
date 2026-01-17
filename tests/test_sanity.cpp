#include "framedot/framedot.hpp"
int main() {
    auto v = framedot::core::version();
    return (v.major >= 0) ? 0 : 1;
}