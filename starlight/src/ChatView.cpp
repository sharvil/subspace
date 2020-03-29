#include "common.h"
#include "ChatView.h"
#include "ConfigDialog.h"
#include "TimeLib.h"

const std::string ChatView::KLogFile("data/logs/session.log");
const std::string ChatView::KTagName[12] =
	{
		"system", "arena", "public", "frequency", "private", "warning",
		"timestamp", "channel", "publicmacro", "remotefrequency", "remoteprivate", "error"
	};

ChatView::ChatView(GtkWidget * scrolledWindow, ConfigDialog * config)
	: iScrolledWindow(GTK_SCROLLED_WINDOW(scrolledWindow)),
	  iTextView(GTK_TEXT_VIEW(gtk_widget_get_by_name(scrolledWindow, "textview1"))),
	  iConfig(config),
	  iAutoScroll(true),
	  iWroteLogTimestamp(false)
{
	PangoFontDescription * desc = pango_font_description_from_string("Monospace 10");
	gtk_widget_modify_font(GTK_WIDGET(iTextView), desc);
	pango_font_description_free(desc);

	GtkTextBuffer * buffer = gtk_text_view_get_buffer(iTextView);

	gtk_text_buffer_create_tag(buffer, "timestamp", "foreground", "black", 0);
	gtk_text_buffer_create_tag(buffer, "system",    "foreground", "black", "weight", "bold", 0);

	gtk_text_buffer_create_tag(buffer, "arena",           "foreground", "green", "weight", "bold", 0);
	gtk_text_buffer_create_tag(buffer, "publicmacro",     "foreground", "black", 0);
	gtk_text_buffer_create_tag(buffer, "public",          "foreground", "black", 0);
	gtk_text_buffer_create_tag(buffer, "frequency",       "foreground", "gold", 0);
	gtk_text_buffer_create_tag(buffer, "remotefrequency", "foreground", "gold", 0);
	gtk_text_buffer_create_tag(buffer, "private",         "foreground", "green", 0);
	gtk_text_buffer_create_tag(buffer, "warning",         "foreground", "orange", "weight", "bold", 0);
	gtk_text_buffer_create_tag(buffer, "remoteprivate",   "foreground", "green", 0);
	gtk_text_buffer_create_tag(buffer, "error",           "foreground", "red", "weight", "bold", 0);
	gtk_text_buffer_create_tag(buffer, "channel",         "foreground", "red", 0);

	GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment(iScrolledWindow);
	g_signal_connect_swapped(adjustment, "changed", G_CALLBACK(SOnScrollChanged), this);
	g_signal_connect_swapped(adjustment, "value-changed", G_CALLBACK(SOnScrollValueChanged), this);

	iConfig->AddListener(*this);
	ConfigurationChanged(*iConfig);
}

ChatView::~ChatView()
{
	iConfig->RemoveListener(*this);
}

void ChatView::AddMessage(const std::string & message)
{
	InsertTimestamp();
	Insert(message, "system");
	WriteLog(KChatArena, message);
}

void ChatView::AddMessage(const EChatType & type, const std::string & name, const std::string & message)
{
	std::string line;

	const std::string KColorMap[] =
	{
		"arena", "publicmacro", "public", "frequency", "remotefrequency",
		"private", "warning", "remoteprivate", "error", "channel"
	};

	std::string color = KColorMap[type];

	InsertTimestamp();
	if(type == KChatArena || type == KChatRemotePrivate || name.empty())
		line = message;
	else if(type == KChatChannel)
	{
		//
		// Empty name = incoming chat message
		// Valid name = outgoing chat message
		//
		// This code has to display both types of messages. Incoming messages
		// are of the form:
		//
		// #:name> msg
		//
		// whereas outgoing messages are of the form:
		//
		// #;message
		//

		cstring endptr;
		uint32 channelNo = strtoul(message.c_str(), &endptr, 10);
		while(endptr && std::isspace(*endptr))
			++endptr;

		if(*endptr == ';')
			line = Format("%u:%s> %s", channelNo, name.c_str(), endptr + 1);
		else if(!name.empty())
			line = Format("1:%s> %s", name.c_str(), message.c_str());
		else
			line = message;
	}
	else
		line = Format("%12.12s> %s",  name.c_str(), message.c_str());

	Insert(line, color);
	WriteLog(type, line);
}

void ChatView::InsertTimestamp()
{
	//
	// Don't add the newline if this is the first text insert.
	//
	if(gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(iTextView)) > 0)
		Insert("\n", "timestamp");

	if(!iConfig->GetShowTimestamps())
		return;

	Time t;
	Insert(Format("[%02d:%02d:%02d] ", t.GetLocalHour(), t.GetLocalMinute(), t.GetLocalSecond()), "timestamp");
}

void ChatView::Insert(const std::string & text, const std::string & tag)
{
	GtkTextBuffer * buffer = gtk_text_view_get_buffer(iTextView);

	if(tag.empty())
		gtk_text_buffer_insert_at_cursor(buffer, text.c_str(), text.length());
	else
	{
		GtkTextIter end;
		gtk_text_buffer_get_end_iter(buffer, &end);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &end, text.c_str(), text.length(), tag.c_str(), NULL);
	}
}

void ChatView::OnScrollChanged()
{
	GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment(iScrolledWindow);

	gdouble upper, page_size;
	g_object_get(adjustment, "upper", &upper, "page-size", &page_size, NULL);

	if(iAutoScroll)
		gtk_adjustment_set_value(adjustment, upper - page_size);
}

void ChatView::OnScrollValueChanged()
{
	GtkAdjustment * adjustment = gtk_scrolled_window_get_vadjustment(iScrolledWindow);

	gdouble upper, page_size;
	g_object_get(adjustment, "upper", &upper, "page-size", &page_size, NULL);

	iAutoScroll = (gtk_adjustment_get_value(adjustment) == upper - page_size);
}

void ChatView::ConfigurationChanged(ConfigDialog & config)
{
	GtkTextTagTable * table = gtk_text_buffer_get_tag_table(gtk_text_view_get_buffer(iTextView));

	for(uint32 i = 0; i < 12; ++i)
	{
		GtkTextTag * tag = gtk_text_tag_table_lookup(table, KTagName[i].c_str());
		if(tag == 0)
			continue;

		g_object_set(G_OBJECT(tag), "foreground", config.GetColor(KTagName[i]).c_str(), NULL);
	}

//	GdkColor background;
//	if(gdk_color_parse(config.GetColor("background").c_str(), &background))
//		gtk_widget_modify_base(GTK_WIDGET(iTextView), GTK_STATE_NORMAL, &background);

	if(!iWroteLogTimestamp)
	{
		Time t;
		WriteLog(KChatArena, Format("Log file opened at %s.", t.ToLocalTzString().c_str()));
	}

	iWroteLogTimestamp = config.GetLogChat();
}

void ChatView::WriteLog(const EChatType & type, const std::string & message) const
{
	if(!iConfig->GetLogChat())
		return;

	std::string::value_type typeChr;
	switch(type)
	{
		case KChatFrequency:
			typeChr = 'T';
			break;

		case KChatRemoteFrequency:
			typeChr = 'E';
			break;

		case KChatChannel:
			typeChr = 'C';
			break;

		case KChatPrivate:
		case KChatRemotePrivate:
			typeChr = 'P';
			break;

		case KChatWarning:
		case KChatError:
			typeChr = '*';
			break;

		default:
			typeChr = ' ';
			break;
	}

	std::string logLine = Format("%c %s\n", typeChr, message.c_str());

	File fp(KLogFile, "at");
	fp.Write(logLine.data(), logLine.length());
}
