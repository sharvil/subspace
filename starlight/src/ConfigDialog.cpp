#include "common.h"
#include "ConfigDialog.h"
#include "Network.h"
#include "Application.h"

const std::string ConfigDialog::KColorButtons[13] =
	{
		"system", "arena", "public", "frequency", "private", "warning",
		"timestamp", "channel", "publicmacro", "remotefrequency", "remoteprivate", "error",
		"background"
	};

const std::string ConfigDialog::KFilename("data/config.dat");

ConfigDialog::ConfigDialog(Application & app, GtkWidget * dialog, Network & network)
	: iApplication(app),
	  iNetwork(network),
	  iDialog(GTK_DIALOG(dialog)),
	  iZoneStore(gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING)),
	  iThemeStore(gtk_list_store_new(1, G_TYPE_STRING)),
	  iNotifyStore(gtk_list_store_new(1, G_TYPE_STRING)),
	  iLogChat(true),
	  iShowTimestamps(true),
	  iShowKillMessages(true),
	  iShowEnterLeaveMessages(false),
	  iThemeTimeoutId(0),
	  iIsRepopulatingThemeList(false)
{
//
// ------------------------------------------------------------ Zone View
//
	iZoneView = GTK_TREE_VIEW(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.vbox3.hbox3.scrolledwindow4.zones"));

	GtkCellRenderer * renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn * column1 = gtk_tree_view_column_new_with_attributes("Zone", renderer, "text", 0, NULL);
	GtkTreeViewColumn * column2 = gtk_tree_view_column_new_with_attributes("Pop", renderer, "text", 1, NULL);

	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_view_column_set_sort_column_id(column2, 1);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iZoneStore), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(iZoneView, GTK_TREE_MODEL(iZoneStore));
	gtk_tree_view_append_column(iZoneView, column1);
	gtk_tree_view_append_column(iZoneView, column2);

	GtkTreeSelection * selection = gtk_tree_view_get_selection(iZoneView);
	g_signal_connect_swapped(selection, "changed", G_CALLBACK(SZoneSelected), this);

	g_object_unref(G_OBJECT(iZoneStore));

//
// ------------------------------------------------------------ Theme View
//
	iThemeView = GTK_TREE_VIEW(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.scrolledwindow6.themelist"));

	renderer = gtk_cell_renderer_text_new();
	column1 = gtk_tree_view_column_new_with_attributes("Theme", renderer, "text", 0, NULL);

	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iThemeStore), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(iThemeView, GTK_TREE_MODEL(iThemeStore));
	gtk_tree_view_append_column(iThemeView, column1);

	selection = gtk_tree_view_get_selection(iThemeView);
	g_signal_connect_swapped(selection, "changed", G_CALLBACK(SThemeSelected), this);

	g_object_unref(G_OBJECT(iThemeStore));

//
// ---------------------------------------------------------- Notify View
//
	iNotifyView = GTK_TREE_VIEW(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.vbox5.table4.scrolledwindow5.notify"));

	renderer = gtk_cell_renderer_text_new();
	column1 = gtk_tree_view_column_new_with_attributes("Notify Text", renderer, "text", 0, NULL);

	g_object_set(renderer, "editable", true, NULL);
	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(iNotifyStore), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model(iNotifyView, GTK_TREE_MODEL(iNotifyStore));
	gtk_tree_view_append_column(iNotifyView, column1);

	g_object_unref(G_OBJECT(iNotifyStore));

	g_signal_connect_swapped(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.vbox5.table4.add"), "clicked", G_CALLBACK(SNotifyAddPressed), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.vbox5.table4.edit"), "clicked", G_CALLBACK(SNotifyEditPressed), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(dialog, "dialog-vbox1.notebook1.vbox5.table4.remove"), "clicked", G_CALLBACK(SNotifyRemovePressed), this);
	g_signal_connect_swapped(renderer, "edited", G_CALLBACK(SNotifyEdited), this);

	gtk_widget_hide_on_delete(GTK_WIDGET(dialog));
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(&ConfigDialog::SDismissed), this);

	Restore();
}

ConfigDialog::~ConfigDialog()
{
	Save();
}

const std::string & ConfigDialog::GetUsername() const
{
	return iUsername;
}

const std::string & ConfigDialog::GetPassword() const
{
	return iPassword;
}

const InetAddress & ConfigDialog::GetAddress() const
{
	return iAddress;
}

const std::string & ConfigDialog::GetChannels() const
{
	return iChannels;
}

const std::string & ConfigDialog::GetInitialArena() const
{
	return iInitialArena;
}

std::string ConfigDialog::GetColor(const std::string & type)
{
	GtkWidget * widget = gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.table2." + type);
	if(widget == 0 || !GTK_IS_COLOR_BUTTON(widget))
		return std::string();

	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(widget), &color);

	gchar * text = gdk_color_to_string(&color);
	std::string ret = text;
	g_free(text);

	return ret;
}

const std::string & ConfigDialog::GetLastUser(uint32 index) const
{
	uint32 nonempty = 0;
	for(uint32 i = 0; i < 5; ++i)
		if(!iLastUsers[i].empty())
			++nonempty;

	if(nonempty == 0)
		return iLastUsers[0];
	return iLastUsers[index % nonempty];
}

bool ConfigDialog::GetMinimizeToTray() const
{
	return iMinimizeToTray;
}

bool ConfigDialog::GetLogChat() const
{
	return iLogChat;
}

bool ConfigDialog::GetShowTimestamps() const
{
	return iShowTimestamps;
}

bool ConfigDialog::GetShowKillMessages() const
{
	return iShowKillMessages;
}

bool ConfigDialog::GetShowEnterMessages() const
{
	return iShowEnterLeaveMessages;
}

const StringSequence & ConfigDialog::GetNotifyStrings() const
{
	return iNotifyStrings;
}

void ConfigDialog::SetShowTimestamps(bool show)
{
	iShowTimestamps = show;
}

void ConfigDialog::SetShowKillMessages(bool show)
{
	iShowKillMessages = show;
}

void ConfigDialog::SetShowEnterMessages(bool show)
{
	iShowEnterLeaveMessages = show;
}

void ConfigDialog::SetLastUser(const std::string & name)
{
	if(name.empty())
		return;

	uint32 evict = 4;
	for(uint32 i = 0; i < 5; ++i)
		if(name == iLastUsers[i])
		{
			evict = i;
			break;
		}

	for(uint32 i = evict; i > 0; --i)
		iLastUsers[i] = iLastUsers[i - 1];

	iLastUsers[0] = name;
}

void ConfigDialog::SetColor(const std::string & type, const std::string & value)
{
	GtkWidget * widget = gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.table2." + type);
	if(widget == 0 || !GTK_IS_COLOR_BUTTON(widget))
		return;

	GdkColor color;
	if(gdk_color_parse(value.c_str(), &color))
		gtk_color_button_set_color(GTK_COLOR_BUTTON(widget), &color);
}

void ConfigDialog::Show()
{
	try
	{
		File fp(Application::KThemeFilename, "rt");
		std::string theme = fp.ReadLine();
		fp.Close();

		if(!theme.empty())
			iOriginalThemeName = iCurrentThemeName = theme.substr(1);
	}
	catch(const std::runtime_error &)
	{
		iOriginalThemeName = iCurrentThemeName = "(default)";
	}

	try
	{
		iIsRepopulatingThemeList = true;

		GtkTreeSelection * selection = gtk_tree_view_get_selection(iThemeView);
		GtkTreeIter iter;
		gtk_list_store_clear(iThemeStore);

		Directory dir(Application::KThemeDirectory);
		Directory::List themes = dir.ListAll();
		for(Directory::List::iterator i = themes.begin(); i != themes.end(); ++i)
			if(i->iIsDirectory && i->iName.find('.') != 0 && File::Exists(Application::KThemeDirectory + i->iName + "/gtk-2.0/gtkrc"))
			{
				gtk_list_store_append(iThemeStore, &iter);
				gtk_list_store_set(iThemeStore, &iter, 0, i->iName.c_str(), -1);
				if(iOriginalThemeName == i->iName)
					gtk_tree_selection_select_iter(selection, &iter);
			}

		iIsRepopulatingThemeList = false;
	}
	catch(const std::runtime_error &)
	{
	}

	try
	{
		iNetwork.Connect(InetAddress("sscentral.sscuservers.net", 4990), *this);
	}
	catch(const std::runtime_error &)
	{
	}

	gtk_widget_show_all(GTK_WIDGET(iDialog));
}

void ConfigDialog::NotifyAddPressed()
{
	GtkTreeIter iter;
	gtk_list_store_prepend(iNotifyStore, &iter);

	GtkTreePath * path = gtk_tree_model_get_path(GTK_TREE_MODEL(iNotifyStore), &iter);
	gtk_widget_grab_focus(GTK_WIDGET(iNotifyView));
	gtk_tree_view_set_cursor(iNotifyView, path, gtk_tree_view_get_column(iNotifyView, 0), true);
}

void ConfigDialog::NotifyEditPressed()
{
	GtkTreeIter iter;
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iNotifyView);

	if(!gtk_tree_selection_get_selected(selection, 0, &iter))
		return;

	GtkTreePath * path = gtk_tree_model_get_path(GTK_TREE_MODEL(iNotifyStore), &iter);
	gtk_widget_grab_focus(GTK_WIDGET(iNotifyView));
	gtk_tree_view_set_cursor(iNotifyView, path, gtk_tree_view_get_column(iNotifyView, 0), true);
}

void ConfigDialog::NotifyRemovePressed()
{
	GtkTreeIter iter;
	GtkTreeSelection * selection = gtk_tree_view_get_selection(iNotifyView);

	if(!gtk_tree_selection_get_selected(selection, 0, &iter))
		return;

	gtk_list_store_remove(iNotifyStore, &iter);
}

void ConfigDialog::NotifyEdited(cstring pathText, cstring text)
{
	GtkTreeIter iter;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(iNotifyStore), &iter, pathText);

	std::string newText = text;
	if(String::Trim(newText).empty())
		gtk_list_store_remove(iNotifyStore, &iter);
	else
		gtk_list_store_set(iNotifyStore, &iter, 0, newText.c_str(), -1);
}

void ConfigDialog::Dismissed(int32 response)
{
	iNetwork.Disconnect(*this);
	gtk_widget_hide(GTK_WIDGET(iDialog));

	if(response == 0)
	{
		try
		{
			File fp(Format("data/themes/%s/colors.txt", iCurrentThemeName.c_str()), "wt");
			for(uint32 i = 0; i < ARRAY_SIZE(KColorButtons) - 1; ++i)
				fp.Write(bstring(Format("%s=%s\n", KColorButtons[i].c_str(), GetColor(KColorButtons[i]).c_str())));
			fp.Close();
		}
		catch(const std::runtime_error &)
		{
		}

		Save();

		for(Listeners::iterator i = iListeners.begin(); i != iListeners.end(); ++i)
			(*i)->ConfigurationChanged(*this);
	}
	else
	{
		iCurrentThemeName = iOriginalThemeName;

		File fp(Application::KThemeFilename, "wt");
		fp.Write(bstring(Format("#%s\ninclude \"themes/%s/gtk-2.0/gtkrc\"\n", iCurrentThemeName.c_str(), iCurrentThemeName.c_str())));
		fp.Close();

		if(!ReloadTheme())
		{
			if(iThemeTimeoutId != 0)
				g_source_remove(iThemeTimeoutId);

			iThemeTimeoutId = g_timeout_add(250, (gboolean (*)(void *))(SThemeUpdate), this);
		}

		Restore();
	}
}

void ConfigDialog::OnConnect(SSConnection & connection)
{
	connection.Send(bstring("\x01\x00\x00\x00\x00", 5), true);
}

void ConfigDialog::OnReceive(SSConnection & connection, const bstring & packet)
{
	gtk_list_store_clear(iZoneStore);

	DataReader <LittleEndian, AsciiZ> reader(packet, 1);

	while(!reader.IsEndOfData())
	{
		uint32 ipAddress = reader.ReadUint8() << 24;
		ipAddress |= reader.ReadUint8() << 16;
		ipAddress |= reader.ReadUint8() << 8;
		ipAddress |= reader.ReadUint8();

		uint16 port = reader.ReadUint16();
		uint16 count = reader.ReadUint16();
		bool billed = (reader.ReadUint16() != 0);
		uint32 version = reader.ReadUint32();

		UNUSED_VARIABLE(billed);
		UNUSED_VARIABLE(version);

		std::string name = (const char *)reader.Readbstring(64).Data();
		std::string description = reader.ReadString();

		InetAddress remoteAddress(ipAddress, port);

		GtkTreeIter iter;
		gtk_list_store_append(iZoneStore, &iter);
		gtk_list_store_set(iZoneStore, &iter, 0, name.c_str(), 1, count, 2, description.c_str(), 3, remoteAddress.AsString().c_str(), -1);

		if(iAddress == remoteAddress)
		{
			GtkTreeSelection * selection = gtk_tree_view_get_selection(iZoneView);
			gtk_tree_selection_select_iter(selection, &iter);

			GtkTreePath * path = gtk_tree_model_get_path(GTK_TREE_MODEL(iZoneStore), &iter);
			gtk_tree_view_scroll_to_cell(iZoneView, path, 0, false, 0.0, 0.0);
		}
	}

	iNetwork.Disconnect(*this);
}

void ConfigDialog::ZoneSelected(GtkTreeSelection * selection)
{
	GtkEntry * textentry = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox3.hbox2.host"));
	GtkTextView * textview = GTK_TEXT_VIEW(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox3.hbox3.description"));
	GtkTextBuffer * buffer = gtk_text_view_get_buffer(textview);

	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(selection, 0, &iter))
		gtk_text_buffer_set_text(buffer, "", 0);
	else
	{
		gchar * description, * address;

		gtk_tree_model_get(GTK_TREE_MODEL(iZoneStore), &iter, 2, &description, 3, &address, -1);
		gtk_text_buffer_set_text(buffer, description, -1);
		gtk_entry_set_text(textentry, address);

		g_free(description);
		g_free(address);
	}
}

void ConfigDialog::ThemeSelected(GtkTreeSelection * selection)
{
	if(iIsRepopulatingThemeList)
		return;

	if(iThemeTimeoutId != 0)
		g_source_remove(iThemeTimeoutId);

	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selection, 0, &iter))
	{
		gchar * name_ptr;
		gtk_tree_model_get(GTK_TREE_MODEL(iThemeStore), &iter, 0, &name_ptr, -1);
		iCurrentThemeName = name_ptr;
		g_free(name_ptr);
	}
	else
		iCurrentThemeName = "(default)";

	File fp(Application::KThemeFilename, "wt");
	fp.Write(bstring(Format("#%s\ninclude \"themes/%s/gtk-2.0/gtkrc\"\n", iCurrentThemeName.c_str(), iCurrentThemeName.c_str())));
	fp.Close();

	if(!ReloadTheme())
		iThemeTimeoutId = g_timeout_add(250, (gboolean (*)(void *))(SThemeUpdate), this);
}

bool ConfigDialog::ThemeUpdate()
{
	File fp(Application::KThemeFilename, "at");
	fp.Write(bstring("\n"));
	fp.Close();

	if(ReloadTheme())
	{
		iThemeTimeoutId = 0;
		return false;
	}

	return true;
}

bool ConfigDialog::ReloadTheme()
{
	if(!gtk_rc_reparse_all())
		return false;

	try
	{
		File fp;
		fp.Open(Format("data/themes/%s/colors.txt", iCurrentThemeName.c_str()), "rt");
		while(!fp.IsEndOfFile())
		{
			std::string line = String::TrimCopy(fp.ReadLine());

			std::string::size_type offset = line.find(';');
			if(offset != std::string::npos)
				line.erase(offset);

			offset = line.find('=');
			if(offset == std::string::npos)
				continue;

			std::string key = line.substr(0, offset);
			std::string value = line.substr(offset + 1);

			String::ToLower(String::Trim(key));
			String::Trim(value);

			for(uint32 i = 0; i < ARRAY_SIZE(KColorButtons) - 1; ++i)
				if(KColorButtons[i] == key)
					SetColor(KColorButtons[i], value);
		}
		fp.Close();
	}
	catch(const std::runtime_error &)
	{
	}

	return true;
}

void ConfigDialog::Save()
{
	GtkWidget * username = gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.username");
	GtkWidget * password = gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.password");
	GtkEntry * address  = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox3.hbox2.host"));
	GtkEntry * channels = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.channels"));
	GtkEntry * initialArena = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.initialarena"));
	GtkToggleButton * logChat = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.logchat"));
	GtkToggleButton * minToTray = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.minimizetotray"));

	iUsername = gtk_entry_get_text(GTK_ENTRY(username));
	iPassword = gtk_entry_get_text(GTK_ENTRY(password));
	iAddress = InetAddress(gtk_entry_get_text(address));
	iChannels = gtk_entry_get_text(GTK_ENTRY(channels));
	iInitialArena = gtk_entry_get_text(GTK_ENTRY(initialArena));
	iLogChat = gtk_toggle_button_get_active(logChat) != 0;
	iMinimizeToTray = gtk_toggle_button_get_active(minToTray) != 0;

	iNotifyStrings.clear();
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iNotifyStore), &iter))
		do
		{
			cstring str;
			gtk_tree_model_get(GTK_TREE_MODEL(iNotifyStore), &iter, 0, &str, -1);
			iNotifyStrings.push_back(str);
			g_free(str);
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(iNotifyStore), &iter));

	writer_t writer;
	writer.AppendUint32(KFileIdentifier);
	writer.AppendUint32(KFileFormatStarlightConfig);
	writer.AppendUint32(KStarlightConfigVersion_1_2);
	writer.AppendString(iUsername);
	writer.AppendString(iPassword);
	writer.AppendString(iAddress.AsString());
	writer.AppendString(iChannels);

	for(uint32 i = 0; i < 13; ++i)
		writer.AppendString(GetColor(KColorButtons[i]));

	writer.AppendString(iInitialArena);

	for(uint32 i = 0; i < 5; ++i)
		writer.AppendString(iLastUsers[i]);

	writer.AppendUint8(iMinimizeToTray);
	writer.AppendUint8(iLogChat);
	writer.AppendUint8(iShowTimestamps);
	writer.AppendUint8(iShowKillMessages);
	writer.AppendUint8(iShowEnterLeaveMessages);

	writer.AppendUint32(iNotifyStrings.size());
	for(StringSequence::const_iterator i = iNotifyStrings.begin(); i != iNotifyStrings.end(); ++i)
		writer.AppendString(*i);

	File fp;
	fp.Open(KFilename, "wb");
	fp.Write(writer.Asbstring());
}

void ConfigDialog::Restore()
{
	GtkEntry * username = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.username"));
	GtkEntry * password = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.password"));
	GtkEntry * address  = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox3.hbox2.host"));
	GtkEntry * channels = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.channels"));
	GtkEntry * initialArena = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.initialarena"));
	GtkToggleButton * logChat = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.logchat"));
	GtkToggleButton * minToTray = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.minimizetotray"));

	ResetToDefaults();

	try
	{
		File fp;
		fp.Open(KFilename, "rb");

		bstring data = fp.Read(File::Length(KFilename));
		reader_t reader(data);

		if(reader.ReadUint32() != KFileIdentifier)
			throw not_found_error("unknown file format.");

		if(reader.ReadUint32() != KFileFormatStarlightConfig)
			throw not_found_error("not a Starlight config file.");

		uint32 version = reader.ReadUint32();
		if(version != KStarlightConfigVersion_1_0 && version != KStarlightConfigVersion_1_1 && version != KStarlightConfigVersion_1_2)
			throw not_found_error("not a Starlight 1.0, 1.1, or 1.2 config file.");

		iUsername = reader.ReadString();
		iPassword = reader.ReadString();
		iAddress = InetAddress(reader.ReadString());
		iChannels = reader.ReadString();

		gtk_entry_set_text(username, iUsername.c_str());
		gtk_entry_set_text(password, iPassword.c_str());
		gtk_entry_set_text(address,  iAddress.AsString().c_str());
		gtk_entry_set_text(channels, iChannels.c_str());

		for(uint32 i = 0; i < 12; ++i)
			SetColor(KColorButtons[i], reader.ReadString());

		//
		// Done parsing v1.0 config files.
		//
		if(version == KStarlightConfigVersion_1_0)
			return;

		SetColor(KColorButtons[12], reader.ReadString());

		iInitialArena = reader.ReadString();
		gtk_entry_set_text(initialArena, iInitialArena.c_str());

		for(uint32 i = 0; i < 5; ++i)
			iLastUsers[i] = reader.ReadString();

		iMinimizeToTray = reader.ReadUint8() != 0;
		gtk_toggle_button_set_active(minToTray, iMinimizeToTray ? TRUE : FALSE);

		//
		// Done parsing v1.1 config files.
		//
		if(version == KStarlightConfigVersion_1_1)
			return;

		iLogChat = reader.ReadUint8() != 0;
		iShowTimestamps = reader.ReadUint8() != 0;
		iShowKillMessages = reader.ReadUint8() != 0;
		iShowEnterLeaveMessages = reader.ReadUint8() != 0;

		gtk_toggle_button_set_active(logChat, iLogChat ? TRUE : FALSE);

		for(uint32 count = reader.ReadUint32(); count; --count)
		{
			std::string str = reader.ReadString();
			iNotifyStrings.push_back(str);

			GtkTreeIter iter;
			gtk_list_store_append(iNotifyStore, &iter);
			gtk_list_store_set(iNotifyStore, &iter, 0, str.c_str(), -1);
		}
	}
	catch(const std::runtime_error &)
	{
		ResetToDefaults();
	}
}

void ConfigDialog::ResetToDefaults()
{
	GtkEntry * username = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.username"));
	GtkEntry * password = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table1.password"));
	GtkEntry * address  = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox3.hbox2.host"));
	GtkEntry * channels = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.channels"));
	GtkEntry * initialArena = GTK_ENTRY(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.initialarena"));
	GtkToggleButton * logChat = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.logchat"));
	GtkToggleButton * minToTray = GTK_TOGGLE_BUTTON(gtk_widget_get_by_name(GTK_WIDGET(iDialog), "dialog-vbox1.notebook1.vbox4.table3.minimizetotray"));

	iUsername.clear();
	iPassword.clear();
	iAddress = InetAddress();
	iChannels.clear();
	iInitialArena.clear();
	iMinimizeToTray = true;
	iLogChat = true;
	iShowTimestamps = true;
	iShowKillMessages = true;
	iShowEnterLeaveMessages = false;
	iNotifyStrings.clear();

	gtk_entry_set_text(username, "");
	gtk_entry_set_text(password, "");
	gtk_entry_set_text(address, "");
	gtk_entry_set_text(channels, "");

	SetColor(KColorButtons[0], "black");
	SetColor(KColorButtons[1], "green");
	SetColor(KColorButtons[2], "black");
	SetColor(KColorButtons[3], "gold");
	SetColor(KColorButtons[4], "green");
	SetColor(KColorButtons[5], "orange");
	SetColor(KColorButtons[6], "grey");
	SetColor(KColorButtons[7], "red");
	SetColor(KColorButtons[8], "black");
	SetColor(KColorButtons[9], "gold");
	SetColor(KColorButtons[10], "green");
	SetColor(KColorButtons[11], "red");
	SetColor(KColorButtons[12], "white");

	gtk_entry_set_text(initialArena, "");

	for(uint32 i = 0; i < 5; ++i)
		iLastUsers[i].clear();

	gtk_toggle_button_set_active(minToTray, TRUE);
	gtk_toggle_button_set_active(logChat, TRUE);
	gtk_list_store_clear(iNotifyStore);
}
