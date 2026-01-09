#include "FrameTiming.h"

// Static member initialization
FrameTiming::TimePoint FrameTiming::s_frameStart;
FrameTiming::TimePoint FrameTiming::s_lastFrame;

double FrameTiming::s_deltaTime = 0.0;
double FrameTiming::s_accumulator = 0.0;
uint32_t FrameTiming::s_displayedThisSecond = 0;
uint32_t FrameTiming::s_fps = 0;
uint64_t FrameTiming::s_totalDisplayed = 0;

void FrameTiming::Initialize()
{
	s_lastFrame = Clock::now();
	s_deltaTime = 0.0;
	s_accumulator = 0.0;
	s_displayedThisSecond = 0;
	s_fps = 0;
	s_totalDisplayed = 0;
}

void FrameTiming::BeginFrame()
{
	s_frameStart = Clock::now();

	// Calculate delta time in seconds
	auto elapsed = std::chrono::duration<double>(s_frameStart - s_lastFrame);
	s_deltaTime = elapsed.count();

	// Clamp delta to prevent spiral of death (e.g., debugger pause)
	if (s_deltaTime > 0.25) s_deltaTime = 0.25;
}

void FrameTiming::EndFrame()
{
	s_lastFrame = s_frameStart;
	s_accumulator += s_deltaTime;

	// Update FPS every second (based on displayed frames counted separately)
	if (s_accumulator >= 1.0)
	{
		s_fps = s_displayedThisSecond;
		s_displayedThisSecond = 0;
		s_accumulator -= 1.0;  // Preserve remainder for accuracy
	}
}

void FrameTiming::CountDisplayedFrame()
{
	// Called only when a frame is actually flipped/displayed
	s_totalDisplayed++;
	s_displayedThisSecond++;
}

double FrameTiming::GetDeltaTime()
{
	return s_deltaTime;
}

double FrameTiming::GetDeltaTimeMS()
{
	return s_deltaTime * 1000.0;
}

uint32_t FrameTiming::GetFPS()
{
	return s_fps;
}

uint64_t FrameTiming::GetFrameCount()
{
	return s_totalDisplayed;
}
