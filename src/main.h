#pragma once
#include "hacklib/Main.h"
#include "hacklib/Hooker.h"
#include "hacklib/PatternScanner.h"
#include "steam/steam_api.h"

#include <windows.h>

class Trespasser : public hl::Main 
{
public:
	bool init() override;
	bool step() override;

private:
	hl::Hooker m_hooker;
};