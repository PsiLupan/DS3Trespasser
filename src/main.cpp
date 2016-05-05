#include "hacklib/Main.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/Hooker.h"
#include "hacklib/PatternScanner.h"
#include "steam/steam_api.h"

#include <windows.h>

ISteamFriends* sFriends = nullptr;
ISteamNetworking* sNetwork = nullptr;

hl::StaticInit<class Trespasser> g_main;
const hl::IHook *g_sendHook;
const hl::IHook *g_readHook;

hl::ConsoleEx m_con;

std::map<uint64_t, bool> allowed;

bool __fastcall tSendP2PPacket(void* __this, CSteamID steamIDRemote, const void* pubData, uint32 cubData, EP2PSend eP2PSendType, int nChannel)
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

bool __fastcall tReadP2PPacket(void* __this, void* pubDest, uint32 cubDest, uint32* pcubMsgSize, CSteamID* psteamIDRemote, int nChannel)
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

class Trespasser : public hl::Main
{
public:
	bool init() override {
		hl::CONSOLEEX_PARAMETERS params = hl::ConsoleEx::GetDefaultParameters();
		
		m_con.create("Tres_passer");
		m_con.printf("Tres_passer initiated\n");

		HMODULE hSteam = nullptr; 
		while(hSteam == nullptr){
			hSteam = GetModuleHandle(TEXT("steam_api64.dll"));
			Sleep(500);
		}
		m_con.printf("[Core::Init] steam_api64.dll module: %p\n", hSteam);
		
		while(sFriends == nullptr){
			sFriends = ((ISteamFriends*(*)(void))GetProcAddress(hSteam, "SteamFriends"))();
			Sleep(500);
		}
		m_con.printf("[Core::Init] SteamFriends: %p\n", sFriends);
		
		while(sNetwork == nullptr){
			sNetwork = ((ISteamNetworking*(*)(void))GetProcAddress(hSteam, "SteamNetworking"))();
			Sleep(500);
		}
		m_con.printf("[Core::Init] SteamNetworking: %p\n", sNetwork);
		
		//Store the original function location and update the VT to ours
		g_sendHook = m_hooker.hookVT((uintptr_t)sNetwork, 0, (uintptr_t)tSendP2PPacket); //VT[0]
		g_readHook = m_hooker.hookVT((uintptr_t)sNetwork, 2, (uintptr_t)tReadP2PPacket); //VT[2]

		m_con.printf("[Core::Init] sendHook Original: %p\n", (uintptr_t)g_sendHook->getLocation());
		m_con.printf("[Core::Init] sendHook Redirect: %p\n", (uintptr_t)tSendP2PPacket);
		m_con.printf("[Core::Init] readHook Original: %p\n", (uintptr_t)g_readHook->getLocation());
		m_con.printf("[Core::Init] readHook Redirect: %p\n", (uintptr_t)tReadP2PPacket);

		return true;
	}
	bool step() override {
		if(!m_con.isOpen()){
			return false;
		}

		/*TODO: Render things*/
		return true;
	}

private:
	hl::Hooker m_hooker;
};