#if 0

#include "base.h"
#include "Database.h"

#include <cstdarg>

const std::string Database::KDatabaseName("data/isometry.dat");

Database::Database()
	  : iDatabase(0)
{
	if(sqlite3_open(KDatabaseName.c_str(), &iDatabase) != SQLITE_OK)
		throw resource_error("[Database::Database]: unable to load " + KDatabaseName + ": " + std::string(sqlite3_errmsg(iDatabase)));
}

Database::~Database()
{
	if(iDatabase != 0)
		sqlite3_close(iDatabase);
}

Database & Database::Instance()
{
	static Database self;
	return self;
}

Database::ResultSet Database::Execute(const std::string & format, ...)
{
	cstring * table;
	int32 rows;
	int32 columns;
	int32 error;

	va_list ap;
	va_start(ap, format);
	cstring query = sqlite3_vmprintf(format.c_str(), ap);
	va_end(ap);

	error = sqlite3_get_table(iDatabase, query, &table, &rows, &columns, 0);
	sqlite3_free(query);

	switch(error)
	{
		case SQLITE_OK:
			break;

		case SQLITE_CONSTRAINT:
			throw constraint_error("database query", sqlite3_errmsg(iDatabase));

		default:
			throw system_error("[Database::Execute]: sqlite query error: " + std::string(sqlite3_errmsg(iDatabase)));
	}

	ResultSet ret(table, rows, columns, sqlite3_changes(iDatabase));

	sqlite3_free_table(table);

	return ret;
}

Database::ResultSet::ResultSet(cstring * values, int32 rows, int32 columns, int32 changes)
	: iRows(rows),
	  iColumns(columns),
	  iChanges(changes)
{
	for(uint32 i = 0; i < (rows + 1) * columns; ++i)
	{
		if(values[i])
			iValues.push_back(values[i]);
		else
			iValues.push_back(std::string());
	}
}

bool Database::ResultSet::changed() const
{
	return iChanges != 0;
}

bool Database::ResultSet::empty() const
{
	return iColumns == 0 || iRows == 0;
}

bool Database::ResultSet::IsEmpty() const
{
	return empty();
}

std::string Database::ResultSet::GetColumnName(uint32 column) const
{
	if(column >= GetColumnCount())
		throw invalid_argument("[Database::ResultSet::GetColumnName]: column index out of bounds.");

	return iValues[column];
}

uint32 Database::ResultSet::GetRowCount() const
{
	return iRows;
}

uint32 Database::ResultSet::GetColumnCount() const
{
	return iColumns;
}

uint32 Database::ResultSet::GetChangeCount() const
{
	return iChanges;
}

bool Database::ResultSet::operator () (std::string & value, uint32 row, uint32 column)
{
	if(row >= GetRowCount())
		throw invalid_argument("[Database::ResultSet::operator ()]: row index out of bounds.");

	if(column >= GetColumnCount())
		throw invalid_argument("[Database::ResultSet::operator ()]: column index out of bounds.");

	value = iValues[(row + 1) * GetColumnCount() + column];
	return true;
}

bool Database::ResultSet::operator () (bstring & value, uint32 row, uint32 column)
{
	if(row >= GetRowCount())
		throw invalid_argument("[Database::ResultSet::operator ()]: row index out of bounds.");

	if(column >= GetColumnCount())
		throw invalid_argument("[Database::ResultSet::operator ()]: column index out of bounds.");

	value = Database::BlobUnescape(iValues[(row + 1) * GetColumnCount() + column]);
	return true;
}

std::string Database::BlobEscape(const bstring & str)
{
	std::string ret;
	std::string::value_type tmp[3] = { 0 };

	for(bstring::size_type i = 0; i < str.size(); ++i)
	{
		sprintf(tmp, "%02X", str[i]);
		ret.append(tmp);
	}

	return ret;
}

bstring Database::BlobUnescape(const std::string & str)
{
	if(str.empty())
		return bstring();

	std::string::size_type length = (str.length() - 3) / 2;
	bstring ret(length);

	for(std::string::size_type i = 0; i < length; ++i)
		sscanf(str.c_str() + 2 * i + 2, "%02X", &ret[i]);
	return ret;
}
#endif
