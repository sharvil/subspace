#pragma once

#include "common.h"
#include "SubSpaceProtocol.h"

#include <gtk/gtk.h>

class ConfigDialog;

class ArenaView
{
	public:
		explicit ArenaView(GtkWidget * view, ConfigDialog * config);
		~ArenaView();

		std::string GetArena(GtkTreePath * path);

		void Populate(const ArenaList & arenas);
		void Clear();

	private:
		static gboolean SSelectionFunction(GtkTreeSelection * selection, GtkTreeModel * model, GtkTreePath * path, gboolean currentSelection, ArenaView * self) { return self->SelectionFunction(path, currentSelection); }

		gboolean SelectionFunction(GtkTreePath * path, gboolean currentSelection);

		GtkTreeView * iView;
		GtkListStore * iStore;
		ConfigDialog * iConfig;
};
