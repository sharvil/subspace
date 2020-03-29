#pragma once

#include "base.h"

#define DECLARE_EXCEPTION(base, name, prefix, suffix)                  \
	struct name : public base                                          \
	{                                                                  \
		name(const std::string & msg) : base(prefix + msg + suffix) {} \
		virtual ~name() throw() {}                                     \
	};                                                                 \

DECLARE_EXCEPTION(not_found_error, zone_not_found_error, "zone'", "'");
DECLARE_EXCEPTION(not_found_error, chat_not_found_error, "chat '", "'");
DECLARE_EXCEPTION(not_found_error, squad_not_found_error, "squad '", "'");
DECLARE_EXCEPTION(not_found_error, player_not_found_error, "player '", "'");
DECLARE_EXCEPTION(not_found_error, ban_not_found_error, "ban #", "");
DECLARE_EXCEPTION(not_found_error, op_not_found_error, "op '", "'");
DECLARE_EXCEPTION(not_found_error, op_group_not_found_error, "op group '", "'");

DECLARE_EXCEPTION(permission_error, credential_error, "", "");
DECLARE_EXCEPTION(permission_error, access_denied_error, "", "");
DECLARE_EXCEPTION(permission_error, operation_not_permitted_error, "", "");

struct banned_error : public permission_error
{
	banned_error(const std::string & bannedUntil, uint32 banId) : permission_error(std::string()), iBannedUntil(bannedUntil), iBanId(banId) {}
	virtual ~banned_error() throw() {}

	std::string iBannedUntil;
	uint32 iBanId;
};


#undef DECLARE_EXCEPTION
