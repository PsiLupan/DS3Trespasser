#include "main.h"

ISteamFriends* sFriends = nullptr;
ISteamMatchmaking* sMatchmaking = nullptr;
ISteamNetworking* sNetwork = nullptr;

hl::StaticInit<class Trespasser> g_main;

Trespasser *GetMain()
{
	return g_main.getMain();
}

const hl::IHook *g_sendHook;
const hl::IHook *g_readHook;
const hl::IHook *g_setHook;
const hl::IHook *g_createHook;
const hl::IHook *g_joinHook;

hl::ConsoleEx m_con;
static const DWORD fontColor = 0xffffffff;

std::map<uint64_t, bool> allowed;

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
			m_con.printf("[NetHook::Send] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
		
		allowed[id] = true;
		m_con.printf("[NetHook::Send] Accepted: %I64d (kFR = %d)\n", id, kFriend);
		return ((bool(__thiscall*)(void*, CSteamID, const void*, uint32, EP2PSend, int))orgFunc)(__this, steamIDRemote, pubData, cubData, eP2PSendType, nChannel);
	}
	/*Otherwise, if online make sure the person isn't blocked.*/
	else {
		if (kFriend == k_EFriendRelationshipBlocked || kFriend == k_EFriendRelationshipIgnored || kFriend == k_EFriendRelationshipIgnoredFriend) {
			allowed[id] = false;
			m_con.printf("[NetHook::Send] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}
		
		allowed[id] = true;
		m_con.printf("[NetHook::Send] Accepted: %I64d (kFR = %d)\n", id, kFriend);
		return ((bool(__thiscall*)(void*, CSteamID, const void*, uint32, EP2PSend, int))orgFunc)(__this, steamIDRemote, pubData, cubData, eP2PSendType, nChannel);
	}

	allowed[id] = true;
	m_con.printf("[NetHook::Send] Accepted: %I64d (kFR = %d)\n", id, kFriend);
	
	return ((bool(__thiscall*)(void*, CSteamID, const void*, uint32, EP2PSend, int))orgFunc)(__this, steamIDRemote, pubData, cubData, eP2PSendType, nChannel);
}

bool __fastcall ReadP2PPacket(void* __this, void* pubDest, uint32 cubDest, uint32* pcubMsgSize, CSteamID* psteamIDRemote, int nChannel)
{
	auto orgFunc = g_readHook->getLocation();
	uint64_t id = psteamIDRemote->ConvertToUint64();

	if (allowed[id] == true || id == 0) {
		return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
	}

	EPersonaState kPersonaState = sFriends->GetPersonaState();
	EFriendRelationship kFriend = sFriends->GetFriendRelationship(*psteamIDRemote);
	
	/*If set to Away, only allow friends*/
	if (kPersonaState != k_EPersonaStateOnline){
		if (kFriend != k_EFriendRelationshipFriend) {
			allowed[id] = false;
			m_con.printf("[NetHook::Read] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}

		allowed[id] = true;
		m_con.printf("[NetHook::Read] Accepted: %I64d (kFR = %d)\n", id, kFriend);
		return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
	}
	/*Otherwise, if online make sure the person isn't blocked.*/
	else {
		if (kFriend == k_EFriendRelationshipBlocked || kFriend == k_EFriendRelationshipIgnored || kFriend == k_EFriendRelationshipIgnoredFriend) {
			allowed[id] = false;
			m_con.printf("[NetHook::Read] Denied: %I64d (kFR = %d)\n", id, kFriend);
			return false;
		}

		allowed[id] = true;
		m_con.printf("[NetHook::Read] Accepted: %I64d (kFR = %d)\n", id, kFriend);
		return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
	}

	allowed[id] = true;
	m_con.printf("[NetHook::Read] Accepted: %I64d (kFR = %d)\n", id, kFriend);
	
	return ((bool(__thiscall*)(void*, void*, uint32, uint32*, CSteamID*, int))orgFunc)(__this, pubDest, cubDest, pcubMsgSize, psteamIDRemote, nChannel);
}

SteamAPICall_t __fastcall CreateLobby(void* __this, ELobbyType eLobbyType, int cMaxMembers) {
	m_con.printf("[MatchHook::Create] Create: %d : %d\n", eLobbyType, cMaxMembers);
	auto orgFunc = g_createHook->getLocation();

	return ((SteamAPICall_t(__thiscall*)(void*, ELobbyType, int))orgFunc)(__this, eLobbyType, cMaxMembers);
}

SteamAPICall_t __fastcall JoinLobby(void* __this, CSteamID steamIDLobby) {
	m_con.printf("[MatchHook::Join] Join: %I64d\n", steamIDLobby.ConvertToUint64());
	auto orgFunc = g_joinHook->getLocation();

	return ((SteamAPICall_t(__thiscall*)(void*, CSteamID))orgFunc)(__this, steamIDLobby);
}

bool __fastcall SetLobbyData(void* __this, CSteamID steamIDLobby, const char *pchKey, const char *pchValue) {
	m_con.printf("[MatchHook::SetLobbyData]  %I64d : %s : %s\n", steamIDLobby.ConvertToUint64(), pchKey, pchValue);

	auto orgFunc = g_setHook->getLocation();
	return ((bool(__thiscall*)(void*, CSteamID, const char*, const char*))orgFunc)(__this, steamIDLobby, pchKey, pchValue);
}

bool Trespasser::init() {
		hl::CONSOLEEX_PARAMETERS params = hl::ConsoleEx::GetDefaultParameters();
		
		m_con.create("TRES_PASSER");
		m_con.printf("TRES_PASSER initiated\n");

		hl::ModuleHandle hSteam = nullptr; 
		while(hSteam == nullptr){
			hSteam = hl::GetModuleByName("steam_api64.dll");
			Sleep(500);
		}
		m_con.printf("[Core::Init] steam_api64.dll module: %p\n", hSteam);
		
		while(sFriends == nullptr){
			sFriends = ((ISteamFriends*(*)(void))GetProcAddress(hSteam, "SteamFriends"))();
			Sleep(500);
		}
		m_con.printf("[Core::Init] SteamFriends: %p\n", sFriends);

		while (sMatchmaking == nullptr)
		{
			sMatchmaking = ((ISteamMatchmaking*(*)(void))GetProcAddress(hSteam, "SteamMatchmaking"))();
			Sleep(500);
		}
		m_con.printf("[Core::Init] SteamMatchmaking: %p\n", sMatchmaking);
		
		while(sNetwork == nullptr){
			sNetwork = ((ISteamNetworking*(*)(void))GetProcAddress(hSteam, "SteamNetworking"))();
			Sleep(500);
		}
		m_con.printf("[Core::Init] SteamNetworking: %p\n", sNetwork);
		
		//Store the original function location and update the VT to ours
		g_sendHook = m_hooker.hookVT((uintptr_t)sNetwork, 0, (uintptr_t)SendP2PPacket);
		g_readHook = m_hooker.hookVT((uintptr_t)sNetwork, 2, (uintptr_t)ReadP2PPacket);
		g_createHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 13, (uintptr_t)CreateLobby);
		g_joinHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 14, (uintptr_t)JoinLobby);
		g_setHook = m_hooker.hookVT((uintptr_t)sMatchmaking, 20, (uintptr_t)SetLobbyData);

		m_con.printf("[Core::Init] sendHook: %p to %p\n", (uintptr_t)g_sendHook->getLocation(), (uintptr_t)SendP2PPacket);
		m_con.printf("[Core::Init] readHook: %p to %p\n", (uintptr_t)g_readHook->getLocation(), (uintptr_t)ReadP2PPacket);
		m_con.printf("[Core::Init] createHook: %p to %p\n", (uintptr_t)g_createHook->getLocation(), (uintptr_t)CreateLobby);
		m_con.printf("[Core::Init] joinHook: %p to %p\n", (uintptr_t)g_joinHook->getLocation(), (uintptr_t)JoinLobby);
		m_con.printf("[Core::Init] setHook: %p to %p\n", (uintptr_t)g_setHook->getLocation(), (uintptr_t)SetLobbyData);

		/*if (overlay.create() != hl::WindowOverlay::Error::Okay)
			return false;

		overlay.registerResetHandlers([&] { drawer.OnLostDevice(); }, [&] { drawer.OnResetDevice(); });
		overlay.setTargetRefreshRate(60);
		drawer.SetDevice(overlay.getContext());
		
		text = reinterpret_cast<const hl::Font*>(drawer.AllocFont("Arial", 14, true));*/

		return true;
}

bool Trespasser::step() {
		if(!m_con.isOpen()){
			return false;
		}

		/*overlay.beginDraw();
		drawer.DrawFont(text, 10, 25, fontColor, "Test");
		overlay.swapBuffers();*/
		return true;
}