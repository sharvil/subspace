#include "common.h"
#include "ArenaView.h"
#include "ConfigDialog.h"

ArenaView::ArenaView(GtkWidget * view, ConfigDialog * config)
	: iView(GTK_TREE_VIEW(view)),
	  iStore(gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_STRING)),
	  iConfig(config)
{
	GtkCellRenderer * renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn * column1 = gtk_tree_view_column_new_with_attributes("Arena", renderer, "text", 0, NULL);
	GtkTreeViewColumn * column2 = gtk_tree_view_column_new_with_attributes("Pop", renderer, "text", 1, NULL);

	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_view_column_set_sort_column_id(column2, 1);

	GtkTreeSelection * selection = gtk_tree_view_get_selection(iView);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)SSelectionFunction, this, 0);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iStore), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_column_set_expand(column1, true);

	gtk_tree_view_set_model(iView, GTK_TREE_MODEL(iStore));
	gtk_tree_view_append_column(iView, column1);
	gtk_tree_view_append_column(iView, column2);

	g_object_unref(G_OBJECT(iStore));
}

ArenaView::~ArenaView()
{
}

std::string ArenaView::GetArena(GtkTreePath * path)
{
	std::string ret;

	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(iStore), &iter, path))
		return ret;

	gchar * name;
	gtk_tree_model_get(GTK_TREE_MODEL(iStore), &iter, 3, &name, -1);
	if(name != 0)
	{
		ret = name;
		g_free(name);
	}

	return ret;
}

void ArenaView::Populate(const ArenaList & arenas)
{
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iView);
	for(ArenaList::const_iterator i = arenas.begin(); i != arenas.end(); ++i)
	{
		GtkTreeIter item;
		gtk_list_store_append(iStore, &item);
		gtk_list_store_set(iStore, &item, 0, i->iDisplayName.c_str(), 1, i->iPopulation, 2, i->iInArena ? TRUE : FALSE, 3, i->iName.c_str(), -1);

		if(i->iInArena)
			gtk_tree_selection_select_iter(selection, &item);
	}
}

void ArenaView::Clear()
{
	gtk_list_store_clear(iStore);
}

gboolean ArenaView::SelectionFunction(GtkTreePath * path, gboolean currentSelection)
{
	if(currentSelection)
		return TRUE;

	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(iStore), &iter, path))
		return FALSE;

	gboolean inArena = FALSE;
	gtk_tree_model_get(GTK_TREE_MODEL(iStore), &iter, 2, &inArena, -1);
	return inArena;
}
