/*
Copyright (C) 2016 Draios inc.

This file is part of falco.

falco is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

falco is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with falco.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "falco_outputs.h"

#include "config_falco.h"


#include "formats.h"
#include "logger.h"

using namespace std;

falco_outputs::falco_outputs()
{

}

falco_outputs::~falco_outputs()
{

}

void falco_outputs::init(bool json_output)
{
	// The engine must have been given an inspector by now.
	if(! m_inspector)
	{
		throw falco_exception("No inspector provided");
	}

	falco_common::init(m_lua_main_filename.c_str(), FALCO_SOURCE_LUA_DIR);

	// Note that falco_formats is added to both the lua state used
	// by the falco engine as well as the separate lua state used
	// by falco outputs.
	falco_formats::init(m_inspector, m_ls, json_output);

	falco_logger::init(m_ls);
}

void falco_outputs::add_output(output_config oc)
{
	uint8_t nargs = 1;
	lua_getglobal(m_ls, m_lua_add_output.c_str());

	if(!lua_isfunction(m_ls, -1))
	{
		throw falco_exception("No function " + m_lua_add_output + " found. ");
	}
	lua_pushstring(m_ls, oc.name.c_str());

	// If we have options, build up a lua table containing them
	if (oc.options.size())
	{
		nargs = 2;
		lua_createtable(m_ls, 0, oc.options.size());

		for (auto it = oc.options.cbegin(); it != oc.options.cend(); ++it)
		{
			lua_pushstring(m_ls, (*it).second.c_str());
			lua_setfield(m_ls, -2, (*it).first.c_str());
		}
	}

	if(lua_pcall(m_ls, nargs, 0, 0) != 0)
	{
		const char* lerr = lua_tostring(m_ls, -1);
		throw falco_exception(string(lerr));
	}

}

void falco_outputs::handle_event(sinsp_evt *ev, string &level, string &priority, string &format)
{
	lua_getglobal(m_ls, m_lua_output_event.c_str());

	if(lua_isfunction(m_ls, -1))
	{
		lua_pushlightuserdata(m_ls, ev);
		lua_pushstring(m_ls, level.c_str());
		lua_pushstring(m_ls, priority.c_str());
		lua_pushstring(m_ls, format.c_str());

		if(lua_pcall(m_ls, 4, 0, 0) != 0)
		{
			const char* lerr = lua_tostring(m_ls, -1);
			string err = "Error invoking function output: " + string(lerr);
			throw falco_exception(err);
		}
	}
	else
	{
		throw falco_exception("No function " + m_lua_output_event + " found in lua compiler module");
	}

}
