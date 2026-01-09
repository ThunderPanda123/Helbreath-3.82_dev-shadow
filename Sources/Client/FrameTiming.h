#pragma once

#include <chrono>
#include <stdint.h>

// FrameTiming: Per-frame timing metrics
//
// Works alongside GameClock to provide frame-specific timing:
//   - Delta time between frames
//   - Accurate FPS calculation
//   - Frame counting
//
// GameClock provides: GetTimeMS() for general elapsed time
// FrameTiming provides: Per-frame delta and FPS metrics
//
// Usage:
//   FrameTiming::Initialize();  // Call once at startup (after GameClock::Initialize)
//
//   // In game loop:
//   FrameTiming::BeginFrame();
//   // ... update and render ...
//   FrameTiming::EndFrame();
//
//   double dt = FrameTiming::GetDeltaTime();  // Seconds since last frame
//   uint32_t fps = FrameTiming::GetFPS();     // Current FPS
//
class FrameTiming
{
public:
	static void Initialize();
	static void BeginFrame();
	static void EndFrame();
	static void CountDisplayedFrame();   // Call only when frame is actually flipped

	// Accessors
	static double GetDeltaTime();        // Seconds since last frame
	static double GetDeltaTimeMS();      // Milliseconds since last frame
	static uint32_t GetFPS();            // Displayed frames per second
	static uint64_t GetFrameCount();     // Total displayed frames

private:
	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::steady_clock::time_point;

	static TimePoint s_frameStart;        // Frame start time
	static TimePoint s_lastFrame;         // Previous frame start

	static double s_deltaTime;            // Current delta in seconds
	static double s_accumulator;          // Time accumulator for FPS calc
	static uint32_t s_displayedThisSecond; // Displayed frames in current second
	static uint32_t s_fps;                // Last calculated FPS (displayed)
	static uint64_t s_totalDisplayed;     // Total displayed frame count
};
