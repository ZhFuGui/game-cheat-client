#include <MouseboxTarget.h>

void setup()
{
    mousebox_target::runtime.begin();
}
void loop()
{
    mousebox_target::runtime.step();
}
