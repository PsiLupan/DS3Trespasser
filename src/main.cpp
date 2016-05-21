#include "main.h"
#include <map>

#ifdef DEBUG
#include "hacklib/ConsoleEx.h"
hl::ConsoleEx m_con;
#endif

hl::StaticInit<class Trespasser> g_main;

Trespasser *GetMain()
{
	return g_main.getMain();
}


ISteamFriends* sFriends = nullptr;
ISteamMatchmaking* sMatchmaking = nullptr;
ISteamNetworking* sNetwork = nullptr;

const hl::IHook *g_sendHook;
const hl::IHook *g_readHook;
const hl::IHook *g_setHook;
const hl::IHook *g_createHook;
const hl::IHook *g_joinHook;

uintptr_t players[6];

std::map<uint64_t, bool> allowed;

void log(const char* format, ...) {
#ifdef DEBUG
	va_list vl;
	va_start(vl, format);
	m_con.vprintf(format, vl);
	va_end(vl);
#endif
}

bool __fastcall SendP2PPacket(void* __this, CSteamID steamIDRemote, const void* pubData, uint32 cubData, EP2PSend eP2PSendType, int nChannel)
{
	auto orgFunc = g_sendHook->getLocation();
	uint64_t id = steamIDRemote.ConvertToUint64();

	if(allowed[id] == true){
		return ((bool(__thiscall*)(void*, CSteamID, const void*, uint32, EP2PSend, int))orgFunc)(__this, steamIDRemote, pubData, cubData, eP2PSendType, nChannel);
	}

	EPersonaState kPersonaState = sFriends->GetPersonaState();
	EFriendRelationship kFriend = sFriends->GetFriendRelationship(steamIDRemote);
	
	/*If set to Away, only allow friends*/
	if (kPersonaState != k_EPersonaStateOnline) {
		if (kFriend != k_EFriendRelationshipFriend){
			allowed[id] = false;
			log("[SteamAPI::SendP2PPacket] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
	}
	/*Otherwise, if online make sure the person isn't blocked.*/
	else {
		if (kFriend == k_EFriendRelationshipBlocked || kFriend == k_EFriendRelationshipIgnored || kFriend == k_EFriendRelationshipIgnoredFriend) {
			allowed[id] = false;
			log("[SteamAPI::SendP2PPacket] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
	}

	allowed[id] = true;
	log("[SteamAPI::SendP2PPacket] Accepted: %I64d (kFR = %d)\n", id, kFriend);
	
	return ((bool(__thiscall*)(void*, CSteamID, const void*, uint32, EP2PSend, int))orgFunc)(__this, steamIDRemote, pubData, cubData, eP2PSendType, nChannel);
}

bool __fastcall ReadP2PPacket(void* __this, void* pubDest, uint32 cubDest, uint32* pcubMsgSize, CSteamID* psteamIDRemote, int nChannel)
{
	auto orgFunc = g_readHook->getLocation();
	uint64_t id = psteamIDRemote->ConvertToUint64();

	if (id == 0 || allowed[id] == true) {
		return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
	}

	EPersonaState kPersonaState = sFriends->GetPersonaState();
	EFriendRelationship kFriend = sFriends->GetFriendRelationship(*psteamIDRemote);
	
	/*If set to Away, only allow friends*/
	if (kPersonaState != k_EPersonaStateOnline){
		if (kFriend != k_EFriendRelationshipFriend) {
			allowed[id] = false;
			log("[SteamAPI::ReadP2PPacket] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
	}
	/*Otherwise, if online make sure the person isn't blocked.*/
	else {
		if (kFriend == k_EFriendRelationshipBlocked || kFriend == k_EFriendRelationshipIgnored || kFriend == k_EFriendRelationshipIgnoredFriend) {
			allowed[id] = false;
			log("[SteamAPI::ReadP2PPacket] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
	}

	allowed[id] = true;
	log("[SteamAPI::ReadP2PPacket] Accepted: %I64d (kFR = %d)\n", id, kFriend);
	
	return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
}

SteamAPICall_t __fastcall CreateLobby(void* __this, ELobbyType eLobbyType, int cMaxMembers) {
	log("[SteamAPI::CreateLobby] %d : %d\n", eLobbyType, cMaxMembers);

	return ((SteamAPICall_t(__thiscall*)(void*, ELobbyType, int))g_createHook->getLocation())(__this, eLobbyType, cMaxMembers);
}

SteamAPICall_t __fastcall JoinLobby(void* __this, CSteamID steamIDLobby) {
	log("[SteamAPI::JoinLobby] %I64d\n", steamIDLobby.ConvertToUint64());

	return ((SteamAPICall_t(__thiscall*)(void*, CSteamID))g_joinHook->getLocation())(__this, steamIDLobby);
}

bool __fastcall SetLobbyData(void* __this, CSteamID steamIDLobby, const char *pchKey, const char *pchValue) {
	log("[SteamAPI::SetLobbyData]  %I64d : %s : %s\n", steamIDLobby.ConvertToUint64(), pchKey, pchValue);

	return ((bool(__thiscall*)(void*, CSteamID, const char*, const char*))g_setHook->getLocation())(__this, steamIDLobby, pchKey, pchValue);
}

bool Trespasser::init() {
#ifdef DEBUG
	m_con.create("TRES_PASSER DEBUG");
#endif
	log("TRES_PASSER Initiated\n");

	hl::ModuleHandle hSteam = hl::GetModuleByName("steam_api64.dll");

	sFriends = ((ISteamFriends*(*)(void))GetProcAddress(hSteam, "SteamFriends"))();
	sMatchmaking = ((ISteamMatchmaking*(*)(void))GetProcAddress(hSteam, "SteamMatchmaking"))();
	sNetwork = ((ISteamNetworking*(*)(void))GetProcAddress(hSteam, "SteamNetworking"))();

	log("[Core::Init] steam_api64.dll module: %p\n", hSteam);
	log("[Core::Init] SteamFriends: %p\n", sFriends);
	log("[Core::Init] SteamMatchmaking: %p\n", sMatchmaking);
	log("[Core::Init] SteamNetworking: %p\n", sNetwork);

	//Store the original function location and update the VT to ours
	g_sendHook = m_hooker.hookVT((uintptr_t)sNetwork, 0, (uintptr_t)SendP2PPacket);
	g_readHook = m_hooker.hookVT((uintptr_t)sNetwork, 2, (uintptr_t)ReadP2PPacket);
	g_createHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 13, (uintptr_t)CreateLobby);
	g_joinHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 14, (uintptr_t)JoinLobby);
	g_setHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 20, (uintptr_t)SetLobbyData);

	log("[Core::Init] sendHook: %p to %p\n", g_sendHook->getLocation(), (uintptr_t)SendP2PPacket);
	log("[Core::Init] readHook: %p to %p\n", g_readHook->getLocation(), (uintptr_t)ReadP2PPacket);
	log("[Core::Init] createHook: %p to %p\n", g_createHook->getLocation(), (uintptr_t)CreateLobby);
	log("[Core::Init] joinHook: %p to %p\n", g_joinHook->getLocation(), (uintptr_t)JoinLobby);
	log("[Core::Init] setHook: %p to %p\n", g_setHook->getLocation(), (uintptr_t)SetLobbyData);

	//uintptr_t addr = hl::FindPattern("48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 40 50", "DarkSoulsIII.exe");
	uintptr_t addr = (uintptr_t)hl::GetModuleByName("DarkSoulsIII.exe") + 0x469D118;
	addr = *reinterpret_cast<uintptr_t*>(addr); // [addr]
	players[0] = ((uintptr_t)(*reinterpret_cast<uintptr_t*>(((uintptr_t)addr + 0x10)))) + 0x88;
	addr = *reinterpret_cast<uintptr_t*>(((uintptr_t)addr + 0x18)); // [addr + 0x18]
 
	players[1] = (uintptr_t)addr + 0x88;
	players[2] = (uintptr_t)addr + 0x9B8;
	players[3] = (uintptr_t)addr + 0x12E8;
	players[4] = (uintptr_t)addr + 0x1C18;
	players[5] = (uintptr_t)addr + 0x2548;
	log("[Core::Init] Session:\n [P0]: %p\n [P1]: %p\n [P2]: %p\n [P3]: %p\n [P4]: %p\n [P5]: %p\n", players[0], players[1], players[2], players[3], players[4], players[5]);
	
	return true;
}

bool Trespasser::step() {
	if (allowed.size() > 32) {
		allowed.clear();
	}
	return true;
}