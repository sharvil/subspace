#include "common.h"
#include "PlayerView.h"
#include "ConfigDialog.h"

PlayerView::PlayerView(GtkWidget * view, ConfigDialog * config)
	: iView(GTK_TREE_VIEW(view)),
	  iStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING))
{
	GtkCellRenderer * renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn * nameColumn = gtk_tree_view_column_new_with_attributes("Nickname", renderer, "text", 0, NULL);
	GtkTreeViewColumn * squadColumn = gtk_tree_view_column_new_with_attributes("Squad", renderer, "text", 1, NULL);

	gtk_tree_view_column_set_sort_column_id(nameColumn, 0);
	gtk_tree_view_column_set_sort_column_id(squadColumn, 1);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iStore), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_column_set_expand(nameColumn, true);

	gtk_tree_view_set_model(iView, GTK_TREE_MODEL(iStore));
	gtk_tree_view_append_column(iView, nameColumn);
	gtk_tree_view_append_column(iView, squadColumn);

	g_object_unref(G_OBJECT(iStore));
}

PlayerView::~PlayerView()
{
}

void PlayerView::AddPlayer(const Player & player)
{
	if(iPlayers.find(player.iId) != iPlayers.end())
		return;

	iPlayers.insert(std::make_pair(player.iId, player));

	GtkTreeIter i;
	gtk_list_store_append(iStore, &i);
	gtk_list_store_set(iStore, &i, 0, player.iName.c_str(), 1, player.iSquad.c_str(), -1);
}

void PlayerView::RemovePlayer(plid_t uid)
{
	Players::iterator p = iPlayers.find(uid);
	if(p == iPlayers.end())
		return;

	GtkTreeIter i;
	for(bool ret = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iStore), &i) != 0;
	    ret;
	    ret = gtk_tree_model_iter_next(GTK_TREE_MODEL(iStore), &i) != 0
	   )
	{
		gchar * pointer;
		gtk_tree_model_get(GTK_TREE_MODEL(iStore), &i, 0, &pointer, -1);
		if(pointer == 0)
			continue;

		std::string name = pointer;
		g_free(pointer);
		if(p->second.iName == name)
		{
			gtk_list_store_remove(iStore, &i);
			break;
		}
	}
	iPlayers.erase(p);
}

void PlayerView::Clear()
{
	iPlayers.clear();
	gtk_list_store_clear(iStore);
}

plid_t PlayerView::GetId(const std::string & name) const
{
	for(Players::const_iterator i = iPlayers.begin(); i != iPlayers.end(); ++i)
		if(i->second.iName == name)
			return i->first;

	return KInvalidPlid;
}

std::string PlayerView::GetName(plid_t uid) const
{
	if(uid == KInvalidPlid)
		return std::string();

	Players::const_iterator i = iPlayers.find(uid);
	if(i == iPlayers.end())
		return std::string();

	return i->second.iName;
}

std::string PlayerView::GetSquadName(plid_t uid) const
{
	if(uid == KInvalidPlid)
		return std::string();

	Players::const_iterator i = iPlayers.find(uid);
	if(i == iPlayers.end())
		return std::string();

	return i->second.iSquad;
}

plid_t PlayerView::GetSelectedId()
{
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iView);
	GtkTreeIter iter;

	if(!gtk_tree_selection_get_selected(selection, 0, &iter))
		return KInvalidPlid;

	gchar * pointer;
	gtk_tree_model_get(GTK_TREE_MODEL(iStore), &iter, 0, &pointer, -1);

	std::string name(pointer);
	g_free(pointer);

	return GetId(name);
}

void PlayerView::SelectNext()
{
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iView);
	GtkTreeIter iter;

	GtkTreePath * path;
	if(gtk_tree_selection_get_selected(selection, 0, &iter) && gtk_tree_model_iter_next(GTK_TREE_MODEL(iStore), &iter))
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(iStore), &iter);
	else
	{
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iStore), &iter))
			return;

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(iStore), &iter);
	}

	gtk_tree_selection_select_path(selection, path);
	gtk_tree_view_scroll_to_cell(iView, path, 0, FALSE, 0.0, 0.0);
	gtk_tree_path_free(path);
}

void PlayerView::SelectPrevious()
{
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iView);
	GtkTreeIter iter, last;

	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iStore), &iter))
		return;

	do
	{
		last = iter;
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(iStore), &iter));

	GtkTreePath * path;
	if(gtk_tree_selection_get_selected(selection, 0, &iter))
	{
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(iStore), &iter);
		if(!gtk_tree_path_prev(path))
		{
			gtk_tree_path_free(path);
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(iStore), &last);
		}
	}
	else
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(iStore), &last);

	gtk_tree_selection_select_path(selection, path);
	gtk_tree_view_scroll_to_cell(iView, path, 0, FALSE, 0.0, 0.0);
	gtk_tree_path_free(path);
}
