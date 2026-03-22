//
//  main.mm
//  test
//
//  Created by Albert Yang on 2026-02-09.
//

#include "mtl_engine.hpp"

int main() {

    MTLEngine engine;
    engine.init();
    engine.run();
    engine.cleanup();

    return 0;
}
