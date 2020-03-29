#pragma once

#include "common.h"
#include "SubSpaceProtocol.h"
#include "IConfigListener.h"

#include <gtk/gtk.h>

class ConfigDialog;

class ChatView : public IConfigListener
{
	public:
		explicit ChatView(GtkWidget * scrolledWindow, ConfigDialog * config);
		~ChatView();

		void AddMessage(const std::string & message);
		void AddMessage(const EChatType & type, const std::string & from, const std::string & message);

	private:
		static const std::string KLogFile;
		static const std::string KTagName[12];

		static void SOnScrollChanged(ChatView * self) { self->OnScrollChanged(); }
		static void SOnScrollValueChanged(ChatView * self) { self->OnScrollValueChanged(); }

		void OnScrollChanged();
		void OnScrollValueChanged();

		void InsertTimestamp();
		void Insert(const std::string & text, const std::string & tagName);
		void WriteLog(const EChatType & type, const std::string & message) const;

		void ConfigurationChanged(ConfigDialog & config);

		GtkScrolledWindow * iScrolledWindow;
		GtkTextView * iTextView;
		ConfigDialog * iConfig;
		bool iAutoScroll;
		bool iWroteLogTimestamp;
};
