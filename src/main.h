#pragma once
#include "hacklib/Main.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/Drawer.h"
#include "hacklib/Hooker.h"
#include "hacklib/PatternScanner.h"
#include "hacklib/WindowOverlay.h"
#include "steam/steam_api.h"

#include <windows.h>

class Trespasser : public hl::Main 
{
public:
	bool init() override;
	bool step() override;

private:
	hl::Drawer drawer;
	hl::WindowOverlay overlay;
	hl::Hooker m_hooker;

	const hl::Font* text;
};