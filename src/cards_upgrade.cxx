//
// cards_upgrade.cxx
//
//  Copyright (c) 2015-2017 by NuTyX team (http://nutyx.org)
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

#include "cards_upgrade.h"

Cards_upgrade::Cards_upgrade(const CardsArgumentParser& argParser,
	const char *configFileName)
	: Pkginst("cards upgrade",configFileName), m_argParser(argParser)
{
	using namespace std;
	if (m_argParser.isSet(CardsArgumentParser::OPT_ROOT))
		m_root=m_argParser.getOptionValue(CardsArgumentParser::OPT_ROOT);

	if (m_root.empty())
		m_root="/";
	else
		m_root=m_root+"/";

	parsePkgRepoCollectionFile();
	buildSimpleDatabase();
	std::set<std::string> tmpList;
	if (!m_config.group.empty()) {
		for (auto i: m_listOfInstPackages) {
			for ( auto j :  m_config.group ) {
					std::string packageName  = getBasePackageName(i.first) + "." + j;
					if ( i.first == packageName )
						continue;
					if (checkBinaryExist(packageName)) {
						tmpList.insert(packageName);
					}
			}
		}
	}
	if ( tmpList.size() > 0) {
		for ( auto i : tmpList) {
			pair<string,time_t> packageNameBuildDate;
			packageNameBuildDate.first = i;
			packageNameBuildDate.second = getBinaryBuildTime(i);
			if (checkPackageNameBuildDateSame(packageNameBuildDate))
				continue;
			m_ListOfPackages.insert(packageNameBuildDate);
		}
	}
	for (auto i : m_listOfInstPackages) {
		if (!checkBinaryExist(i.first)) {
			m_ListOfPackagesToDelete.insert(i.first);
			continue;
		}
		pair<string,time_t> packageNameBuildDate;
		packageNameBuildDate.first = i.first ;
		packageNameBuildDate.second = getBinaryBuildTime(i.first);
		if ( checkPackageNameBuildDateSame(packageNameBuildDate)) {
			continue;
		}
		m_ListOfPackages.insert(packageNameBuildDate);
	}
	if ( m_argParser.command() == CardsArgumentParser::CMD_UPGRADE) {
		if ( m_argParser.isSet(CardsArgumentParser::OPT_CHECK))
			Isuptodate();
		if ( m_argParser.isSet(CardsArgumentParser::OPT_SIZE))
			size();
		if ( (! m_argParser.isSet(CardsArgumentParser::OPT_SIZE)) &&
				(! m_argParser.isSet(CardsArgumentParser::OPT_CHECK)) ) {
			if ( m_ListOfPackages.size() == 0  && ( m_ListOfPackagesToDelete.size() == 0 ) ) {
				std::cout << _("Your system is up to date") << endl;
			} else {
				if (getuid()) {
					m_actualError = ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE;
					treatErrors("");
				}
				upgrade();
			}
		}
	}
	if ( m_argParser.command() == CardsArgumentParser::CMD_DIFF) {
		if ( ( m_ListOfPackages.size() == 0 ) && ( m_ListOfPackagesToDelete.size() == 0 ) ) {
			std::cout << _("Your system is up to date") << endl;
		} else {
			dry();
		}
	}
}
void Cards_upgrade::size()
{
	std::cout << m_ListOfPackages.size() + m_ListOfPackagesToDelete.size() << std::endl;
}
void Cards_upgrade::Isuptodate()
{
	if ( ( m_ListOfPackages.size() == 0 ) && ( m_ListOfPackagesToDelete.size() == 0 ) )
		std::cout << "no" << std::endl;
	else
		std::cout << "yes" << std::endl;
}
void Cards_upgrade::dry()
{
	if (m_ListOfPackages.size() > 1 )
				std::cout << _("Packages") << ": ";
	if (m_ListOfPackages.size() == 1 )
				std::cout << _("Package")<< " : ";
	for (auto i : m_ListOfPackages )
		std::cout << "'" << i.first  << "' ";
	if (m_ListOfPackages.size() > 0 )
				std::cout << _("will be replaced when you upgrade your NuTyX") << std::endl;

	if (m_ListOfPackagesToDelete.size() > 1 )
				std::cout << _("Packages") << ": ";
	if (m_ListOfPackagesToDelete.size() == 1 )
				std::cout << _("Package") << " : ";
	for (auto i: m_ListOfPackagesToDelete)
			std::cout << "'" << i << "' ";
	if (m_ListOfPackagesToDelete.size() > 0 )
				std::cout << _("will be removed when you upgrade your NuTyX") << std::endl;

}
void Cards_upgrade::upgrade()
{
	for (auto i : m_ListOfPackages) generateDependencies(i);

	if (m_argParser.isSet(CardsArgumentParser::OPT_DRY))
		dry();
	else {
		for (auto i : m_dependenciesList) {
			m_packageArchiveName=getPackageFileName(i.first);
			m_force=true;
			if (checkPackageNameExist(i.first)) {
				m_upgrade=true;
			} else {
				m_upgrade=false;
			}
		run();
		}
		for (auto i : m_ListOfPackagesToDelete) {
			Db_lock lock(m_root,true);
			getListOfPackageNames(m_root);
			buildCompleteDatabase(false);
			removePackageFilesRefsFromDB(i);
			removePackageFiles(i);
		}
	}
}
// vim:set ts=2 :
