#include "app.h"

int main(int argc, char** argv)
{
	mns_Init(argc, argv);

	while (mns_ShouldRun())
	{
		mns_InitFrame();
		mns_Update();
		mns_RenderFrame();
		mns_PollEvents();
	}

	mns_Shutdown();

	return 0;
}