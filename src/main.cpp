#include "global.h"

int main(int argc, char* argv[])
{
    g_prog_name = argv[0];
    printf("ARG: %s\n", argv[1]);
    Global::init();
    Url* u = Url::gen_url(argv[1]);
    if (u)
    {
        u->print();
        Global::add_url(u);
    }
    Global::main_loop();
    Global::destroy();
    return 0;
}
