#pragma once

#include "common.h"
#include "ArenaView.h"
#include "ChatView.h"
#include "PlayerView.h"
#include "ConfigDialog.h"
#include "Network.h"
#include "IgnoreList.h"

class Application : public SubSpaceProtocol, public IConfigListener
{
	public:
		enum EState
		{
			KStateNormal,
			KStateNotified,
			KStateUrgent
		};

		static const std::string KThemeFilename;
		static const std::string KThemeDirectory;

		Application();
		~Application();

		bool IsFocused() const;
		void SetState(const EState & state);
		void NetworkDied() const;

	private:
		static void SOnStatusIconClick(Application * self) { self->OnUiStatusIconClick(); }
		static void SOnWindowStateChanged(Application * self, GdkEventWindowState * event) { self->OnUiWindowStateChanged(event); }
		static gboolean SOnQuit(Application * self) { self->OnUiQuit(); return false; }
		static void SOnConnect(Application * self) { self->OnUiConnect(); }
		static void SOnDisconnect(Application * self) { self->OnUiDisconnect(); }
		static void SOnSend(Application * self) { self->OnUiSend(); }
		static void SOnPreferences(Application * self) { self->OnUiPreferences(); }
		static void SOnToggleShowTimestamps(Application * self) { self->OnUiToggleShowTimestamps(); }
		static void SOnToggleShowEnter(Application * self) { self->OnUiToggleShowEnter(); }
		static void SOnToggleShowKills(Application * self) { self->OnUiToggleShowKills(); }
		static void SOnAbout(Application * self) { self->OnUiAbout(); }
		static void SOnNewUserResponse(Application * self, int32 response) { self->OnUiNewUserResponse(response); }
		static void SOnRegistrationFormResponse(Application * self, int32 response) { self->OnUiRegistrationFormResponse(response); }
		static void SOnFocusChange(Application * self, GdkEventFocus * event) { self->OnUiFocusChange(event); }
		static gboolean SOnKeyPress(Application * self, GdkEventKey * event) { return self->OnUiKeyPress(event); }
		static void SOnPlayerSelected(Application * self, GtkTreePath * path, GtkTreeViewColumn * column) { self->OnUiPlayerSelected(self->iPlayers->GetName(self->iPlayers->GetSelectedId())); }
		static void SOnArenaSelected(Application * self, GtkTreePath * path, GtkTreeViewColumn * column) { self->OnUiArenaSelected(self->iArena->GetArena(path)); }

		bool PreParseCommand(const std::string & command);
		void ExpandMacros(std::string & chatText);
		bool ParseCommand(const std::string & command);

		void OnUiStatusIconClick();
		void OnUiWindowStateChanged(GdkEventWindowState * event);
		void OnUiQuit();
		void OnUiConnect();
		void OnUiDisconnect();
		void OnUiSend();
		void OnUiPreferences();
		void OnUiToggleShowTimestamps();
		void OnUiToggleShowEnter();
		void OnUiToggleShowKills();
		void OnUiAbout();
		void OnUiNewUserResponse(int32 response);
		void OnUiRegistrationFormResponse(int32 response);
		void OnUiFocusChange(GdkEventFocus * event);
		bool OnUiKeyPress(GdkEventKey * event);
		void OnUiPlayerSelected(const std::string & username);
		void OnUiArenaSelected(const std::string & arenaName);

		void OnPoll();
		void OnLoginResponse(const ELoginResponse & response, bool requestRegistration);
		void OnLoginMessage(const std::string & message);
		void OnChatMessage(plid_t from, const EChatType & type, const std::string & message);
		void OnPlayerEntering(const Player & player);
		void OnPlayerLeaving(plid_t uid);
		void OnPlayerDied(plid_t player, plid_t killer);
		void OnLogout();
		void OnArenaList(const ArenaList & arenas);
		void OnArenaLogin();
		void OnShipTeamChange(plid_t player, uint8 ship, uint16 team);
		void OnCommandResponse(const std::string & response);

		void ConfigurationChanged(ConfigDialog & config);

		GtkStatusIcon * iStatusIcon;
		GtkWidget * iWindow;
		GtkWidget * iPreferencesDialog;
		GtkWidget * iAboutDialog;
		GtkWidget * iNewUserDialog;
		GtkWidget * iFileSaveDialog;
		GtkWidget * iRegistrationFormDialog;
		GtkEntry * iInput;

		ArenaView * iArena;
		ChatView * iChat;
		PlayerView * iPlayers;

		ConfigDialog * iConfig;
		Network iNetwork;
		IgnoreList iIgnores;

		bool iIsFocused;
		uint32 iLastArenaListTime;
};
