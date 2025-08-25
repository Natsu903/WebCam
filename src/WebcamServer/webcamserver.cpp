#pragma once
#include <iostream>
#include "rtspserver.h"



int main()
{  
    RTSPServer server;
    server.Init();
    server.Invoke();
    getchar();
    server.Stop();
    return 0;
}
