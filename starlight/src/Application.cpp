#include "common.h"
#include "Application.h"
#include "TimeLib.h"

#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>

static const std::string KInterfaceFile("interface.glade");
static const std::string KDefaultIconFile("resources/blue128.png");
static const std::string KPrivateMessageIconFile("resources/red128.png");
static const std::string KNotifyMessageIconFile("resources/yellow128.png");
const std::string Application::KThemeFilename("data/theme.rc");
const std::string Application::KThemeDirectory("data/themes/");

Application::Application()
	: iArena(0),
	  iChat(0),
	  iPlayers(0),
	  iConfig(0),
	  iNetwork(*this),
	  iIsFocused(true),
	  iLastArenaListTime(0)
{

	gtk_rc_parse(KThemeFilename.c_str());

	GladeXML * xml = glade_xml_new(KInterfaceFile.c_str(), 0, 0);
	gtk_window_set_default_icon_from_file(KDefaultIconFile.c_str(), 0);

	iWindow = glade_xml_get_widget(xml, "ApplicationWindow");
	iPreferencesDialog = glade_xml_get_widget(xml, "PreferencesDialog");
	iAboutDialog = glade_xml_get_widget(xml, "AboutDialog");
	iNewUserDialog = glade_xml_get_widget(xml, "NewUserDialog");
	iFileSaveDialog = glade_xml_get_widget(xml, "FileSaveDialog");
	iRegistrationFormDialog = glade_xml_get_widget(xml, "RegistrationFormDialog");

	GtkWidget * playerWidget = gtk_widget_find_by_name(iWindow, "treeview1");
	GtkWidget * chatWidget = gtk_widget_find_by_name(iWindow, "scrolledwindow1");
	GtkWidget * arenaWidget = gtk_widget_find_by_name(iWindow, "treeview2");

	iInput = GTK_ENTRY(gtk_widget_find_by_name(iWindow, "input"));

	iConfig = new ConfigDialog(*this, iPreferencesDialog, iNetwork);
	iConfig->AddListener(*this);

	iArena = new ArenaView(arenaWidget, iConfig);
	iChat = new ChatView(chatWidget, iConfig);
	iPlayers = new PlayerView(playerWidget, iConfig);

	GtkMenu * fileMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_find_by_name(iWindow, "FileMenu"))));
	GtkMenu * editMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_find_by_name(iWindow, "EditMenu"))));
	GtkMenu * viewMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_find_by_name(iWindow, "ViewMenu"))));
	GtkMenu * helpMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_find_by_name(iWindow, "HelpMenu"))));

	g_signal_connect_swapped(iWindow, "focus-in-event", G_CALLBACK(SOnFocusChange), this);
	g_signal_connect_swapped(iWindow, "focus-out-event", G_CALLBACK(SOnFocusChange), this);
	g_signal_connect_swapped(iWindow, "window-state-event", G_CALLBACK(SOnWindowStateChanged), this);

	g_signal_connect_swapped(iWindow, "delete-event", G_CALLBACK(SOnQuit), this);
	g_signal_connect(iAboutDialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), 0);
	g_signal_connect(iPreferencesDialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), 0);
	g_signal_connect(iNewUserDialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), 0);
	g_signal_connect(iRegistrationFormDialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), 0);

	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(fileMenu), "gtk-connect"), "activate", G_CALLBACK(SOnConnect), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(fileMenu), "gtk-disconnect"), "activate", G_CALLBACK(SOnDisconnect), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(fileMenu), "gtk-quit"), "activate", G_CALLBACK(SOnQuit), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(iWindow, "vbox1.hpaned1.vbox2.hbox1.sendButton"), "clicked", G_CALLBACK(SOnSend), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(editMenu), "preferences"), "activate", G_CALLBACK(SOnPreferences), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showtimestamps"), "activate", G_CALLBACK(SOnToggleShowTimestamps), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showenter"), "activate", G_CALLBACK(SOnToggleShowEnter), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showkills"), "activate", G_CALLBACK(SOnToggleShowKills), this);
	g_signal_connect_swapped(gtk_widget_get_by_name(GTK_WIDGET(helpMenu), "about"), "activate", G_CALLBACK(SOnAbout), this);
	g_signal_connect_swapped(iAboutDialog, "response", G_CALLBACK(gtk_widget_hide), iAboutDialog);
	g_signal_connect_swapped(iNewUserDialog, "response", G_CALLBACK(SOnNewUserResponse), this);
	g_signal_connect_swapped(iRegistrationFormDialog, "response", G_CALLBACK(SOnRegistrationFormResponse), this);
	g_signal_connect_swapped(iInput, "key-press-event", G_CALLBACK(SOnKeyPress), this);
	g_signal_connect_swapped(playerWidget, "key-press-event", G_CALLBACK(SOnKeyPress), this);
	g_signal_connect_swapped(playerWidget, "row-activated", G_CALLBACK(SOnPlayerSelected), this);
	g_signal_connect_swapped(arenaWidget, "row-activated", G_CALLBACK(SOnArenaSelected), this);

	gtk_widget_show_all(iWindow);
	gtk_window_maximize(GTK_WINDOW(iWindow));

	GtkPaned * hpane = GTK_PANED(gtk_widget_find_by_name(iWindow, "hpaned1"));
	gint maxSize;
	g_object_get(hpane, "max-position", &maxSize, NULL);
	gtk_paned_set_position(hpane, maxSize * 20 / 100);

	iStatusIcon = gtk_status_icon_new_from_file(KDefaultIconFile.c_str());
	gtk_status_icon_set_visible(iStatusIcon, FALSE);
	g_signal_connect_swapped(iStatusIcon, "activate", G_CALLBACK(SOnStatusIconClick), this);

	if(iConfig->GetUsername().empty() || iConfig->GetPassword().empty() || iConfig->GetAddress() == InetAddress())
		iConfig->Show();
	else
		ConfigurationChanged(*iConfig);
}

Application::~Application()
{
	delete iArena;
	delete iChat;
	delete iPlayers;
	delete iConfig;
}

bool Application::IsFocused() const
{
	return iIsFocused;
}

void Application::SetState(const EState & state)
{
	switch(state)
	{
		case KStateNotified:
			if(IsFocused())
				break;
			gtk_status_icon_set_from_file(iStatusIcon, KNotifyMessageIconFile.c_str());
			gtk_window_set_default_icon_from_file(KNotifyMessageIconFile.c_str(), 0);
			gtk_window_set_urgency_hint(GTK_WINDOW(iWindow), true);
			break;

		case KStateUrgent:
			if(IsFocused())
				break;
			gtk_status_icon_set_from_file(iStatusIcon, KPrivateMessageIconFile.c_str());
			gtk_window_set_default_icon_from_file(KPrivateMessageIconFile.c_str(), 0);
			gtk_window_set_urgency_hint(GTK_WINDOW(iWindow), true);
			break;

		default:
			gtk_status_icon_set_from_file(iStatusIcon, KDefaultIconFile.c_str());
			gtk_window_set_default_icon_from_file(KDefaultIconFile.c_str(), 0);
			gtk_window_set_urgency_hint(GTK_WINDOW(iWindow), false);
			break;
	}
}

void Application::NetworkDied() const
{
	GtkWidget * dlg = gtk_message_dialog_new_with_markup(GTK_WINDOW(iWindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", "Your internet connection has unexpectedly terminated. Please try to connect to a zone once an internet connection is available.");
	gtk_window_set_title(GTK_WINDOW(dlg), "Network Error");
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

void Application::OnUiStatusIconClick()
{
	gtk_window_deiconify(GTK_WINDOW(iWindow));
	gtk_widget_show(iWindow);
	gtk_status_icon_set_visible(iStatusIcon, FALSE);
}

void Application::OnUiWindowStateChanged(GdkEventWindowState * event)
{
//
// Work around known bug in GTK+ for Windows where maximize->minimize->restore doesn't bring
// window back maximized.
//
#ifdef PLATFORM_WIN32
	static bool maximizeHack = false;
	if(event->changed_mask & GDK_WINDOW_STATE_ICONIFIED)
		if(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED && event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
			maximizeHack = true;
		else
		{
			if(!(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) && maximizeHack)
				gtk_window_maximize(GTK_WINDOW(iWindow));
			maximizeHack = false;
		}
#endif

	if(iConfig->GetMinimizeToTray() && event->changed_mask & GDK_WINDOW_STATE_ICONIFIED && event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
	{
		gtk_widget_hide(iWindow);
		gtk_status_icon_set_visible(iStatusIcon, TRUE);
	}
}

void Application::OnUiQuit()
{
	OnUiDisconnect();
	gtk_main_quit();
}

void Application::OnUiConnect()
{
	iUsername = iConfig->GetUsername();
	iPassword = iConfig->GetPassword();
	iNewUser = false;

	if(!IsConnected())
	{
		if(iUsername.empty() || iPassword.empty() || iConfig->GetAddress() == InetAddress())
			iConfig->Show();
		else
			iNetwork.Connect(iConfig->GetAddress(), *this);
	}
}

void Application::OnUiDisconnect()
{
	iLastArenaListTime = 0;
	iNetwork.Disconnect(*this);
}

void Application::OnUiSend()
{
	std::string text(gtk_entry_get_text(iInput));
	gtk_entry_set_text(iInput, "");

	String::Trim(text);
	if(text.empty())
		return;

	if(PreParseCommand(text))
		return;

	if(!IsInArena())
	{
		iChat->AddMessage("Cannot send message: you are not logged into an arena.");
		return;
	}

	ExpandMacros(text);
	if(ParseCommand(text))
		return;

	plid_t target = KInvalidPlid;
	EChatType type = KChatPublic;
	if(text.find("//") == 0)
	{
		type = KChatFrequency;
		text.erase(0, 2);
	}
	else if(text.find("'") == 0)
	{
		type = KChatFrequency;
		text.erase(0, 1);
	}
	else if(text.find("/") == 0)
	{
		type = KChatPrivate;
		target = iPlayers->GetSelectedId();
		text.erase(0, 1);

		if(target == KInvalidPlid)
		{
			iChat->AddMessage("No user selected for private message.");
			text.clear();
		}
	}
	else if(text.find(";") == 0)
	{
		//
		// From Continuum logs. Chats to channel 1 go on the wire without any
		// semicolons or prefixes. Chats to channels other than 1 go on the wire
		// as #;msg
		//
		// In all cases, the target is not KInvalidPlid but 0 (don't know why).
		//
		type = KChatChannel;
		target = 0;

		uint32 channel;
		if(sscanf(text.c_str(), "; %u ;", &channel) == 1 && channel == 1)
			text = text.substr(3);
		else
			text = text.substr(1);
	}
	else if(text.find(":") == 0 && text.rfind(":") != 0)
	{
		int8 name[512];
		uint32 skipLen;
		sscanf(text.c_str(), ":%[^:]:%n", name, &skipLen);

		target = iPlayers->GetId(name);
		if(target == KInvalidPlid)
			type = KChatRemotePrivate;
		else
		{
			type = KChatPrivate;
			text = text.substr(skipLen);
		}
	}

	if(String::Trim(text).empty())
		return;

	SendChatMessage(target, type, text);
	iChat->AddMessage(type, iPlayers->GetName(iUid), text);
}

void Application::OnUiPreferences()
{
	iConfig->Show();
}

void Application::OnUiToggleShowTimestamps()
{
	GtkMenu * viewMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_get_by_name(iWindow, "vbox1.menubar1.ViewMenu"))));
	GtkCheckMenuItem * showTimestamps = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showtimestamps"));
	iConfig->SetShowTimestamps(gtk_check_menu_item_get_active(showTimestamps) != 0);
}
void Application::OnUiToggleShowEnter()
{
	GtkMenu * viewMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_get_by_name(iWindow, "vbox1.menubar1.ViewMenu"))));
	GtkCheckMenuItem * showEnter = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showenter"));
	iConfig->SetShowEnterMessages(gtk_check_menu_item_get_active(showEnter) != 0);
}
void Application::OnUiToggleShowKills()
{
	GtkMenu * viewMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_get_by_name(iWindow, "vbox1.menubar1.ViewMenu"))));
	GtkCheckMenuItem * showKills = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showkills"));
	iConfig->SetShowKillMessages(gtk_check_menu_item_get_active(showKills) != 0);
}

void Application::OnUiAbout()
{
	gtk_widget_show_all(GTK_WIDGET(iAboutDialog));
}

void Application::OnUiNewUserResponse(int32 response)
{
	gtk_widget_hide(GTK_WIDGET(iNewUserDialog));

	if(response == 0)
	{
		iNewUser = true;
		iNetwork.Connect(iConfig->GetAddress(), *this);
	}
}

void Application::OnUiRegistrationFormResponse(int32 response)
{
	gtk_widget_hide(iRegistrationFormDialog);

	GtkEntry * realName = GTK_ENTRY(gtk_widget_get_by_name(iRegistrationFormDialog, "dialog-vbox5.content.realName"));
	GtkEntry * emailAddress = GTK_ENTRY(gtk_widget_get_by_name(iRegistrationFormDialog, "dialog-vbox5.content.email"));

	if(response == 0)
	{
		std::string name = gtk_entry_get_text(realName);
		std::string email = gtk_entry_get_text(emailAddress);

		SendRegistrationForm(name, email);
		EnterArena(iConfig->GetInitialArena());
	}
	else
		iNetwork.Disconnect(*this);

	gtk_entry_set_text(realName, "");
	gtk_entry_set_text(emailAddress, "");
}

void Application::OnUiFocusChange(GdkEventFocus * event)
{
	iIsFocused = event->in != 0;
	if(IsFocused())
		SetState(KStateNormal);
}

bool Application::OnUiKeyPress(GdkEventKey * event)
{
	//
	// Tab in entry field should not cause focus change.
	//
	if(event->keyval == GDK_Tab)
		return true;

	if(event->keyval == GDK_Page_Up)
	{
		iPlayers->SelectPrevious();
		return true;
	}

	if(event->keyval == GDK_Page_Down)
	{
		iPlayers->SelectNext();
		return true;
	}

	if(event->keyval == GDK_colon)
	{
		std::string text = gtk_entry_get_text(iInput);
		String::LTrim(text);
		if(text.empty() || text[0] != ':')
			return false;

		static uint32 rot = 0;
		if(text.length() == 1)
			rot = 0;
		else if(text.find(':', 1) != text.length() - 1)
			return false;
		else
			++rot;
		gtk_entry_set_text(iInput, std::string(":" + iConfig->GetLastUser(rot) + ":").c_str());
		gtk_editable_set_position(GTK_EDITABLE(iInput), -1);
		return true;
	}

	return false;
}

void Application::OnUiArenaSelected(const std::string & arenaName)
{
	iChat->AddMessage("Moving to arena: " + arenaName);
	ParseCommand("?go " + arenaName);
	gtk_widget_grab_focus(GTK_WIDGET(iInput));
}

void Application::OnUiPlayerSelected(const std::string & username)
{
	gtk_entry_set_text(iInput, std::string(":" + username + ":").c_str());
	gtk_widget_grab_focus(GTK_WIDGET(iInput));
	gtk_editable_set_position(GTK_EDITABLE(iInput), -1);
}

bool Application::PreParseCommand(const std::string & command)
{
	if(command.find("?colors") == 0)
	{
		for(EChatType i = KChatArena; i <= KChatChannel; i = static_cast <EChatType> (i + 1))
			iChat->AddMessage(i, "", "The quick brown fox jumped over the lazy dog.");
		return true;
	}

	return false;
}

void Application::ExpandMacros(std::string & chatText)
{
	for(std::string::size_type pos = chatText.find("%selfname", 0); pos != std::string::npos; pos = chatText.find("%selfname", pos + 1))
	{
		if(pos > 0 && chatText[pos - 1] == '%')
			continue;

		chatText.replace(pos, 9, iPlayers->GetName(iUid));
	}

	for(std::string::size_type pos = chatText.find("%squad", 0); pos != std::string::npos; pos = chatText.find("%squad", pos + 1))
	{
		if(pos > 0 && chatText[pos - 1] == '%')
			continue;

		chatText.replace(pos, 6, iPlayers->GetSquadName(iUid));
	}

	for(std::string::size_type pos = chatText.find("%tickname", 0); pos != std::string::npos; pos = chatText.find("%tickname", pos + 1))
	{
		if(pos > 0 && chatText[pos - 1] == '%')
			continue;

		chatText.replace(pos, 9, iPlayers->GetName(iPlayers->GetSelectedId()));
	}

	for(std::string::size_type pos = chatText.find("%%", 0); pos != std::string::npos; pos = chatText.find("%%", pos + 1))
		chatText.replace(pos, 2, "%");
}

bool Application::ParseCommand(const std::string & command)
{
	if(command.find("?go") == 0 && (command.length() == 3 || command[3] == ' '))
	{
		EnterArena(String::TrimCopy(command.substr(3)));
		GetArenaList();

		iPlayers->Clear();
		iArena->Clear();
		return true;
	}

	if(command.find("?ignore ") == 0)
	{
		char buf1[KMaximumChatSize], buf2[KMaximumChatSize];
		if(sscanf(command.c_str(), "%*s %s %[^\n]", buf1, buf2) != 2)
		{
			iChat->AddMessage("Usage: ?ignore (add|remove) name");
			return true;
		}

		std::string subcommand = buf1;
		std::string name = buf2;

		if(subcommand == "add")
		{
			iIgnores.AddName(name);
			iChat->AddMessage(Format("Ignoring: %s", name.c_str()));
		}
		else if(subcommand == "remove")
		{
			iIgnores.RemoveName(name);
			iChat->AddMessage(Format("Unignoring: %s", name.c_str()));
		}
		else
			iChat->AddMessage("Invalid ?ignore command.");

		return true;
	}

	return false;
}

void Application::OnPoll()
{
	uint32 now = Time::GetMilliCount();

	if(now - iLastArenaListTime > 15000)
	{
		iLastArenaListTime = now;
		GetArenaList();
	}
}

void Application::OnLoginResponse(const ELoginResponse & response, bool requestRegistration)
{
	const std::string KStringMap[] =
	{
		"Login complete.",
		"Login failed: unknown username.",
		"Login failed: invalid password.",
		"Login failed: zone currently full.",
		"Login failed: you have been locked out.",
		"Login failed: you do not have permission to enter this zone.",
		"Login complete.",
		"Login failed: you have too many points to enter this zone.",
		"Login failed: your connection is too slow.",
		"Login failed: you do not have permission to enter this zone.",
		"Login failed: the server is not accepting any more connections.",
		"Login failed: your username is invalid.",
		"Login failed: your username contains obscene text.",
		"Login complete.",
		"Login failed: the server is busy.",
		"Login failed: this zone is restricted to experienced players.",
		"Login failed: you are using an unsupported demo version.",
		"Login failed: too many demo clients are connected to zone.",
		"Login failed: you are using an unsupported demo version."
	};

	if(response == KLoginNeedModerator)
		iChat->AddMessage("Login failed: you need to have moderator access to login to this zone.");
	else if(response == KLoginUnknownUser)
		gtk_widget_show_all(iNewUserDialog);
	else if(response >= KLoginUnknown)
		iChat->AddMessage("Login failed: an unknown error has occured.");
	else
		iChat->AddMessage(KStringMap[response]);

	if(requestRegistration)
		gtk_widget_show(iRegistrationFormDialog);
	else if(IsLoggedIn())
		EnterArena(iConfig->GetInitialArena());
}

void Application::OnLoginMessage(const std::string & message)
{
	GtkWidget * dlg = gtk_message_dialog_new_with_markup(GTK_WINDOW(iWindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", message.c_str());
	gtk_window_set_title(GTK_WINDOW(dlg), "User Information");
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}

void Application::OnArenaLogin()
{
	if(!iConfig->GetChannels().empty())
	{
		StringSequence strings = String::Split(iConfig->GetChannels(), ',');
		for(StringSequence::iterator i = strings.begin(); i != strings.end(); ++i)
			String::Trim(*i);

		std::string channels = "?chat=" + String::Join(strings, ',');
		SendChatMessage(KInvalidPlid, KChatPublic, channels);
	}
}

void Application::OnChatMessage(plid_t from, const EChatType & type, const std::string & message)
{
	std::string name = iPlayers->GetName(from);

	if(type == KChatRemotePrivate)
	{
		std::string::size_type pos = message.find(")>");
		if(pos != std::string::npos)
			name = message.substr(1, pos - 1);
	}
	else if(type == KChatChannel)
		name.clear();

	if(iIgnores.IsIgnored(name))
		return;

	if(type == KChatPrivate || type == KChatRemotePrivate)
	{
		iConfig->SetLastUser(name);
		if(!IsFocused())
			SetState(KStateUrgent);
	}

	if(!IsFocused())
	{
		std::string lowercaseMessage = String::ToLowerCopy(message);
		if(lowercaseMessage.find(String::ToLowerCopy(iPlayers->GetName(iUid))) != std::string::npos)
			SetState(KStateNotified);
		else
		{
			const StringSequence & notifyStrings = iConfig->GetNotifyStrings();
			for(StringSequence::const_iterator i = notifyStrings.begin(); i != notifyStrings.end(); ++i)
				if(lowercaseMessage.find(String::ToLowerCopy(*i)) != std::string::npos)
				{
					SetState(KStateNotified);
					break;
				}
		}
	}

	iChat->AddMessage(type, name, message);
}

void Application::OnPlayerEntering(const Player & player)
{
	if(IsInArena() && iConfig->GetShowEnterMessages())
		iChat->AddMessage(Format("%s entered arena", player.iName.c_str()));

	iPlayers->AddPlayer(player);
}

void Application::OnPlayerLeaving(plid_t uid)
{
	if(iConfig->GetShowEnterMessages())
		iChat->AddMessage(Format("%s left arena", iPlayers->GetName(uid).c_str()));

	iPlayers->RemovePlayer(uid);
}

void Application::OnPlayerDied(plid_t player, plid_t killer)
{
	if(!iConfig->GetShowKillMessages())
		return;

	std::string playerName = iPlayers->GetName(player);
	std::string killerName = iPlayers->GetName(killer);

	iChat->AddMessage(Format("%s killed by: %s", playerName.c_str(), killerName.c_str()));
}

void Application::OnLogout()
{
	iChat->AddMessage("You have been disconnected from the server.");
	iPlayers->Clear();
	iArena->Clear();
}

void Application::OnArenaList(const ArenaList & arenas)
{
	iArena->Clear();
	iArena->Populate(arenas);
}

void Application::OnShipTeamChange(plid_t player, uint8 ship, uint16 team)
{
	if(player == iUid && ship != 8)
		ChangeShip(8);
}

void Application::OnCommandResponse(const std::string & response)
{
	iChat->AddMessage(response);
}

void Application::ConfigurationChanged(ConfigDialog & config)
{
	GdkColor color;
//	if(gdk_color_parse(config.GetColor("background").c_str(), &color))
//		gtk_widget_modify_base(GTK_WIDGET(iInput), GTK_STATE_NORMAL, &color);

	if(gdk_color_parse(config.GetColor("public").c_str(), &color))
		gtk_widget_modify_text(GTK_WIDGET(iInput), GTK_STATE_NORMAL, &color);

	GtkMenu * viewMenu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(gtk_widget_get_by_name(iWindow, "vbox1.menubar1.ViewMenu"))));
	GtkCheckMenuItem * showTimestamps = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showtimestamps"));
	GtkCheckMenuItem * showEnter = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showenter"));
	GtkCheckMenuItem * showKills = GTK_CHECK_MENU_ITEM(gtk_widget_get_by_name(GTK_WIDGET(viewMenu), "showkills"));

	gtk_check_menu_item_set_active(showTimestamps, config.GetShowTimestamps());
	gtk_check_menu_item_set_active(showEnter, config.GetShowEnterMessages());
	gtk_check_menu_item_set_active(showKills, config.GetShowKillMessages());
}
