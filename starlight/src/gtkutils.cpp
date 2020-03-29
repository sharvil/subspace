#include "common.h"
#include "gtkutils.h"

#include <gio/gio.h>

GtkWidget * gtk_widget_get_by_name(GtkWidget * widget, const std::string & name)
{
	std::string nameCopy = name;

	for(char * token = strtok(&nameCopy[0], "."); token; token = strtok(0, "."))
	{
		if(!GTK_IS_CONTAINER(widget))
			return 0;

		bool foundChild = false;
		for(GList * children = gtk_container_get_children(GTK_CONTAINER(widget));
		    children;
		    children = g_list_next(children))
		{
			if(gtk_widget_get_name(GTK_WIDGET(children->data)) == std::string(token))
			{
				foundChild = true;
				widget = GTK_WIDGET(children->data);
				break;
			}
		}

		if(!foundChild)
			return 0;
	}

	return widget;
}

GtkWidget * gtk_widget_find_by_name(GtkWidget * root, const std::string & name)
{
	typedef std::list <GtkWidget *> Stack;
	Stack stack;
	stack.push_front(root);

	while(!stack.empty())
	{
		GtkWidget * here = stack.front();
		stack.pop_front();

		if(gtk_widget_get_name(here) == name)
			return here;

		if(!GTK_IS_CONTAINER(here))
			continue;

		for(GList * children = gtk_container_get_children(GTK_CONTAINER(here));
		    children;
		    children = g_list_next(children))
		{
			stack.push_front(GTK_WIDGET(children->data));
		}
	}

	return 0;
}
