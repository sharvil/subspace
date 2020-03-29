#pragma once

#include "common.h"
#include "IConfigListener.h"
#include "IProtocol.h"
#include "NetLib.h"

#include <gtk/gtk.h>

class Application;
class Network;

class ConfigDialog : public IProtocol, public IListenable <IConfigListener>
{
	public:
		explicit ConfigDialog(Application & app, GtkWidget * dialog, Network & network);
		~ConfigDialog();

		const std::string & GetUsername() const;
		const std::string & GetPassword() const;
		const InetAddress & GetAddress() const;
		const std::string & GetChannels() const;
		const std::string & GetInitialArena() const;
		std::string GetColor(const std::string & type);
		const std::string & GetLastUser(uint32 index) const;
		bool GetMinimizeToTray() const;
		bool GetLogChat() const;
		bool GetShowTimestamps() const;
		bool GetShowKillMessages() const;
		bool GetShowEnterMessages() const;
		const StringSequence & GetNotifyStrings() const;

		void SetShowTimestamps(bool show);
		void SetShowKillMessages(bool show);
		void SetShowEnterMessages(bool show);
		void SetLastUser(const std::string & name);

		void Show();

	private:
		typedef DataWriter <LittleEndian, Length16LittleEndianAscii> writer_t;
		typedef DataReader <LittleEndian, Length16LittleEndianAscii> reader_t;

		static const std::string KColorButtons[13];
		static const std::string KFilename;

		static void SDismissed(ConfigDialog * self, int32 response) { self->Dismissed(response); }
		static void SZoneSelected(ConfigDialog * self, GtkTreeSelection * selection) { self->ZoneSelected(selection); }
		static void SThemeSelected(ConfigDialog * self, GtkTreeSelection * selection) { self->ThemeSelected(selection); }
		static bool SThemeUpdate(ConfigDialog * self) { return self->ThemeUpdate(); }
		static void SNotifyAddPressed(ConfigDialog * self) { self->NotifyAddPressed(); }
		static void SNotifyEditPressed(ConfigDialog * self) { self->NotifyEditPressed(); }
		static void SNotifyRemovePressed(ConfigDialog * self) { self->NotifyRemovePressed(); }
		static void SNotifyEdited(ConfigDialog * self, cstring pathText, cstring text) { self->NotifyEdited(pathText, text); }

		void OnConnect(SSConnection & connection);
		void OnReceive(SSConnection & connection, const bstring & packet);
		void Poll(SSConnection & connection) {}
		void OnDisconnect() {}

		void Save();
		void Restore();
		void ResetToDefaults();
		bool ReloadTheme();

		void SetColor(const std::string & type, const std::string & value);

		void NotifyAddPressed();
		void NotifyEditPressed();
		void NotifyRemovePressed();
		void NotifyEdited(cstring pathText, cstring text);

		void ZoneSelected(GtkTreeSelection * selection);
		void ThemeSelected(GtkTreeSelection * selection);
		bool ThemeUpdate();
		void Dismissed(int32 response);

		Application & iApplication;
		Network & iNetwork;
		GtkDialog * iDialog;
		GtkTreeView * iZoneView;
		GtkListStore * iZoneStore;
		GtkTreeView * iThemeView;
		GtkListStore * iThemeStore;
		GtkTreeView * iNotifyView;
		GtkListStore * iNotifyStore;
		std::string iUsername;
		std::string iPassword;
		InetAddress iAddress;
		std::string iChannels;
		std::string iInitialArena;
		std::string iLastUsers[5];
		bool iMinimizeToTray;
		bool iLogChat;
		bool iShowTimestamps;
		bool iShowKillMessages;
		bool iShowEnterLeaveMessages;
		StringSequence iNotifyStrings;
		std::string iOriginalThemeName;
		std::string iCurrentThemeName;
		guint iThemeTimeoutId;
		bool iIsRepopulatingThemeList;
};
