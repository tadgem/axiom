#include "axiom.hpp"

AXM_OVERRIDE_GLOBAL_NEW(false)

using namespace axm;

int main(int argc, char** argv) {
    AxiomEngine engine = AxiomEngine::Init();
    AXM_ASSERT(engine.m_OK, "Failed to start AXIOM");

    const char* script = argc > 1 ? argv[1] : "sample.lua";
    return lua::RunScript(engine, script);
}
