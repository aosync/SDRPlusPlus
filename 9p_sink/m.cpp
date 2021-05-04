#include <9p.hpp>

int main() {
    NinePServer s(5640);
    s.run();
}