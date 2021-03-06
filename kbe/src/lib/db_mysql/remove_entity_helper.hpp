/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __REMOVE_ENTITY_HELPER_H__
#define __REMOVE_ENTITY_HELPER_H__

// common include	
// #define NDEBUG
#include <sstream>
#include "common.hpp"
#include "sqlstatement.hpp"
#include "cstdkbe/cstdkbe.hpp"
#include "cstdkbe/memorystream.hpp"
#include "helper/debug_helper.hpp"
#include "dbmgr_lib/db_interface.hpp"
#include "dbmgr_lib/entity_table.hpp"
#include "db_interface_mysql.hpp"

namespace KBEngine{ 

class RemoveEntityHelper
{
public:
	RemoveEntityHelper()
	{
	}

	virtual ~RemoveEntityHelper()
	{
	}

	static bool removeDB(DBInterface* dbi, DB_OP_TABLE_ITEM_DATA_BOX& opTableItemDataBox)
	{
		bool ret = _removeDB(dbi, opTableItemDataBox);

		if(!ret)
			return false;

		std::string sqlstr = "delete from "ENTITY_TABLE_PERFIX"_";
		sqlstr += opTableItemDataBox.tableName;
		sqlstr += " where "TABLE_ID_CONST_STR"=";

		char sqlstr1[MAX_BUF];
		kbe_snprintf(sqlstr1, MAX_BUF, "%"PRDBID, opTableItemDataBox.dbid);
		sqlstr += sqlstr1;
		
		ret = dbi->query(sqlstr.c_str(), sqlstr.size(), false);
		KBE_ASSERT(ret);

		return ret;
	}

	static bool _removeDB(DBInterface* dbi, DB_OP_TABLE_ITEM_DATA_BOX& opTableItemDataBox)
	{
		bool ret = true;

		KBEUnordered_map< std::string, std::vector<DBID> > childTableDBIDs;

		DB_OP_TABLE_DATAS::iterator iter1 = opTableItemDataBox.optable.begin();
		for(; iter1 != opTableItemDataBox.optable.end(); iter1++)
		{
			DB_OP_TABLE_ITEM_DATA_BOX& wbox = *iter1->second.get();

			KBEUnordered_map<std::string, std::vector<DBID> >::iterator iter = 
				childTableDBIDs.find(opTableItemDataBox.tableName);

			if(iter == childTableDBIDs.end())
			{
				std::vector<DBID> v;
				childTableDBIDs.insert(std::pair< std::string, std::vector<DBID> >(wbox.tableName, v));
			}
		}
		
		if(childTableDBIDs.size() > 1)
		{
			std::string sqlstr_getids;
			KBEUnordered_map< std::string, std::vector<DBID> >::iterator tabiter = childTableDBIDs.begin();
			for(; tabiter != childTableDBIDs.end();)
			{
				char sqlstr[MAX_BUF * 10];
				kbe_snprintf(sqlstr, MAX_BUF * 10, "select count(id) from "ENTITY_TABLE_PERFIX"_%s where "TABLE_PARENTID_CONST_STR"=%"PRDBID" union all ", 
					tabiter->first.c_str(),
					opTableItemDataBox.dbid);
				
				sqlstr_getids += sqlstr;

				kbe_snprintf(sqlstr, MAX_BUF * 10, "select id from "ENTITY_TABLE_PERFIX"_%s where "TABLE_PARENTID_CONST_STR"=%"PRDBID, 
					tabiter->first.c_str(),
					opTableItemDataBox.dbid);

				sqlstr_getids += sqlstr;
				if(++tabiter != childTableDBIDs.end())
					sqlstr_getids += " union all ";
			}
			
			if(dbi->query(sqlstr_getids.c_str(), sqlstr_getids.size(), false))
			{
				MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
				if(pResult)
				{
					MYSQL_ROW arow;
					int32 count = 0;
					tabiter = childTableDBIDs.begin();
					bool first = true;

					while((arow = mysql_fetch_row(pResult)) != NULL)
					{
						if(count == 0)
						{
							StringConv::str2value(count, arow[0]);
							if(!first || count <= 0)
								tabiter++;
							continue;
						}

						DBID old_dbid;
						StringConv::str2value(old_dbid, arow[0]);
						tabiter->second.push_back(old_dbid);
						count--;
						first = false;
					}

					mysql_free_result(pResult);
				}
			}
		}
		else if(childTableDBIDs.size() == 1)
		{
			KBEUnordered_map< std::string, std::vector<DBID> >::iterator tabiter = childTableDBIDs.begin();
				char sqlstr[MAX_BUF * 10];
				kbe_snprintf(sqlstr, MAX_BUF * 10, "select id from "ENTITY_TABLE_PERFIX"_%s where "TABLE_PARENTID_CONST_STR"=%"PRDBID, 
					tabiter->first.c_str(),
					opTableItemDataBox.dbid);

				if(dbi->query(sqlstr, strlen(sqlstr), false))
				{
					MYSQL_RES * pResult = mysql_store_result(static_cast<DBInterfaceMysql*>(dbi)->mysql());
					if(pResult)
					{
						MYSQL_ROW arow;
						while((arow = mysql_fetch_row(pResult)) != NULL)
						{
							DBID old_dbid;
							StringConv::str2value(old_dbid, arow[0]);
							tabiter->second.push_back(old_dbid);
						}

						mysql_free_result(pResult);
					}
				}
		}
	
		// 删除废弃的数据项
		KBEUnordered_map< std::string, std::vector<DBID> >::iterator tabiter = childTableDBIDs.begin();
		for(; tabiter != childTableDBIDs.end(); tabiter++)
		{
			if(tabiter->second.size() == 0)
				continue;

			// 先删除数据库中的记录
			std::string sqlstr = "delete from "ENTITY_TABLE_PERFIX"_";
			sqlstr += tabiter->first;
			sqlstr += " where "TABLE_ID_CONST_STR" in (";

			std::vector<DBID>::iterator iter = tabiter->second.begin();
			for(; iter != tabiter->second.end(); iter++)
			{
				DBID dbid = (*iter);

				char sqlstr1[MAX_BUF];
				kbe_snprintf(sqlstr1, MAX_BUF, "%"PRDBID, dbid);
				sqlstr += sqlstr1;
				sqlstr += ",";
			}
			
			sqlstr.erase(sqlstr.size() - 1);
			sqlstr += ")";
			bool ret = dbi->query(sqlstr.c_str(), sqlstr.size(), false);
			KBE_ASSERT(ret);

			DB_OP_TABLE_DATAS::iterator iter1 = opTableItemDataBox.optable.begin();
			for(; iter1 != opTableItemDataBox.optable.end(); iter1++)
			{
				DB_OP_TABLE_ITEM_DATA_BOX& wbox = *iter1->second.get();
				if(wbox.tableName == tabiter->first)
				{
					std::vector<DBID>::iterator iter = tabiter->second.begin();
					for(; iter != tabiter->second.end(); iter++)
					{
						DBID dbid = (*iter);
						
						wbox.parentTableDBID = opTableItemDataBox.dbid;
						wbox.dbid = dbid;
						wbox.isEmpty = true;

						_removeDB(dbi, wbox);
					}
				}
			}
		}

		return ret;
	}

protected:
};

}
#endif // __REMOVE_ENTITY_HELPER_H__
