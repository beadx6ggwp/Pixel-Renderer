#ifndef APPLICATION_H
#define APPLICATION_H

#include "screen_manager.h"
#include "render_device.h"

class Application {
public:
	Application(int width, int height, const wchar_t* title);
	virtual ~Application();

	void Run(); // game loop

protected:
	virtual void OnInit() {}
	virtual void OnUpdate(float delta_time) {}
	virtual void OnRender() {}
	virtual void OnClose() {}

	ScreenManager screen;
	RenderDevice* device = nullptr;
	bool running = false;
};

#endif