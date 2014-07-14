// cards_argument_parser.h
// 
//  Copyright (c) 2013-2014 by NuTyX team (http://nutyx.org)
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.
//

#ifndef CARDS_ARGUMENT_PARSER_H
#define CARDS_ARGUMENT_PARSER_H

#include "argument_parser.h"

class CardsArgumentParser : public ArgParser
{
	public:
		CardsArgumentParser();
		virtual ~CardsArgumentParser() {};

		static ArgParser::APCmd CMD_CONFIG;
		static ArgParser::APCmd CMD_BASE;
		static ArgParser::APCmd CMD_SYNC;
		static ArgParser::APCmd CMD_INFO;
		static ArgParser::APCmd CMD_LIST;
		static ArgParser::APCmd CMD_INSTALL;
		static ArgParser::APCmd CMD_REMOVE;
		static ArgParser::APCmd CMD_DIFF;
		static ArgParser::APCmd CMD_LEVEL;
		static ArgParser::APCmd CMD_DEPENDS;
		static ArgParser::APCmd CMD_DEPTREE;
		static ArgParser::APCmd CMD_SEARCH;
		static ArgParser::APCmd CMD_ISINST;

		static ArgParser::APOpt OPT_BASE_HELP;

		static ArgParser::APOpt OPT_LIST_INSTALL;
		static ArgParser::APOpt OPT_LIST_BINARIES;
		static ArgParser::APOpt OPT_LIST_PORTS;

		static ArgParser::APOpt OPT_INFO_INSTALL;
		static ArgParser::APOpt OPT_INFO_BINARY;
		static ArgParser::APOpt OPT_INFO_PORT;

		static ArgParser::APOpt OPT_DIFF_BINARIES;
		static ArgParser::APOpt OPT_DIFF_PORTS;

		static ArgParser::APOpt OPT_SEARCH_FILE;
		static ArgParser::APOpt OPT_SEARCH_DESCRIPTION;
		static ArgParser::APOpt OPT_INSTALL_DRY;
		static ArgParser::APOpt OPT_INSTALL_DEVEL;
		static ArgParser::APOpt OPT_BASE_REMOVE;
		static ArgParser::APOpt OPT_REMOVE_ALL;
		static ArgParser::APOpt OPT_PACKAGEFILES;
		static ArgParser::APOpt OPT_SYNCALL;
		static ArgParser::APOpt OPT_SHOW_ALL_DEPENDENCIES;
		static ArgParser::APOpt OPT_LEVEL_SILENCE;

		std::string getAppIdentification() const
		{
			return std::string("cards ")  + VERSION + "\n";
		}
};

#endif
// vim:set ts=2 :