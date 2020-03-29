#pragma once

#include "common.h"

class ConfigDialog;

class IConfigListener
{
	public:
		virtual void ConfigurationChanged(ConfigDialog & config) = 0;

		virtual ~IConfigListener() {}
};
