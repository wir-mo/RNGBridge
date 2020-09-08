#include <ESPUI.h>

void setup(void)
{
    Serial.begin(115200);
    ESPUI.setVerbosity(Verbosity::VerboseJSON);
    ESPUI.prepareFileSystem();
}

void loop()
{
}