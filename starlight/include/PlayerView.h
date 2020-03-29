#pragma once

#include "common.h"

#include <map>
#include <gtk/gtk.h>

class ConfigDialog;

class PlayerView
{
	public:
		explicit PlayerView(GtkWidget * view, ConfigDialog * config);
		~PlayerView();

		void AddPlayer(const Player & player);
		void RemovePlayer(plid_t uid);
		void Clear();

		plid_t GetSelectedId();
		plid_t GetId(const std::string & name) const;
		std::string GetName(plid_t uid) const;
		std::string GetSquadName(plid_t uid) const;

		void SelectNext();
		void SelectPrevious();

	private:
		typedef std::map <plid_t, Player> Players;

		GtkTreeView * iView;
		GtkListStore * iStore;
		ConfigDialog * iConfig;
		Players iPlayers;
};
