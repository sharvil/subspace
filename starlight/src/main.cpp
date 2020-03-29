#include "common.h"
#include "Application.h"
#include "LogLib.h"

#include <gtk/gtk.h>

int main(int argc, char ** argv)
{
	gtk_init(&argc, &argv);

	Logger & log = Logger::Instance();
	log.SetLogFile("error.log");

	try
	{
		Application a;
		gtk_main();
	}
	catch(const std::runtime_error & error)
	{
		Logger::Instance().Log(KLogError, "Caught exception: %s", error.what());
	}
	return 0;
}

#ifdef PLATFORM_WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char * argv[] = { "" };
	return main(0, argv);
}

#endif
