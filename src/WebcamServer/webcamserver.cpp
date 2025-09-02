#pragma once
#include <iostream>
#include "rtspserver.h"


int main()
{  
	RTSPServer server;
	server.Init();
	server.Invoke();
	printf("Press any key to exit...\r\n");
	getchar();
	server.Stop();
    return 0;
}
