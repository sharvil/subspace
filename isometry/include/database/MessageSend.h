#pragma once

#include "base.h"
#include "DbLib.h"
#include "Exceptions.h"

namespace Database
{
	class MessageSend : public DbAction
	{
		public:
			explicit MessageSend(uint32 from, const std::string & to, const std::string & message)
				: iFrom(from),
				  iTo(to),
				  iMessage(message),
				  iReplaced(false)
			{ }

			void operator()(Transaction & t)
			{
				std::string query = Format("select id from isometry.player where lower(name)='%s'", esc(String::ToLowerCopy(iTo)).c_str());
				pqxx::result res = t.exec(query);

				if(res.empty())
					throw player_not_found_error(iTo);

				uint32 to;
				res[0][0].to(to);

				//
				// Note: it might be better to do this the other way around - we're more likely to get new messages rather than
				// someone replacing their old message.
				//
				query = Format("update isometry.message set time=now(), message='%s' where source=%d and dest=%d", esc(iMessage).c_str(), iFrom, to);
				res = t.exec(query);
				iReplaced = true;

				if(res.affected_rows() == 0)
				{
					query = Format("insert into isometry.message (source, dest, message) values (%d, %d, '%s')", iFrom, to, esc(iMessage).c_str());
					res = t.exec(query);
					iReplaced = false;
				}
			}

			bool Replaced() const
			{
				return iReplaced;
			}

		private:
			uint32 iFrom;
			const std::string & iTo;
			const std::string & iMessage;

			bool iReplaced;
	};
}
