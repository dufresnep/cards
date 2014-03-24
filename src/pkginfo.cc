//  pkginfo.cc
// 
//  Copyright (c) 2000-2005 Per Liden
//  Copyright (c) 2006-2013 by CRUX team (http://crux.nu)
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

#include "pkginfo.h"
#include <iterator>
#include <vector>
#include <iomanip>
#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

int Pkginfo::getNumberOfPackages()
{
	int nPkg = getListOfPackages("");
	getInstalledPackages(true);
	return nPkg;
}
bool Pkginfo::isInstalled(const char * packageName)
{
	string s_packageName = packageName;
	int nPkg = getListOfPackages("");
	if (nPkg > 0) {
		for (set<string>::iterator i = m_packagesList.begin();i != m_packagesList.end();++i) {
			string installedPackageName = *i;
			if ( installedPackageName == s_packageName) {
#ifndef NDEBUG
        cerr << s_packageName << endl;
#endif
				return true;
			}
		}
		return false;
	}
	return false;
}
void Pkginfo::run(int argc, char** argv)
{
	//
	// Check command line options
	//
  int o_runtimedependencies_mode = 0;
	int o_footprint_mode = 0;
	int o_installed_mode = 0;
	int o_list_mode = 0;
	int o_owner_mode = 0;
	int o_details_mode = 0;
	int o_librairies_mode = 0;
	int o_runtime_mode = 0;

	string o_root;
	string o_arg;

	for (int i = 1; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-r" || option == "--root") {
			assertArgument(argv, argc, i);
			o_root = argv[i + 1];
			i++;
		} else if (option == "-i" || option == "--installed") {
			o_installed_mode += 1;
		} else if (option == "-d" || option == "--details") {
			assertArgument(argv, argc, i);
			o_details_mode +=1;
			o_arg = argv[i + 1];
			i++;
		} else if (option == "-l" || option == "--list") {
			assertArgument(argv, argc, i);
			o_list_mode += 1;
			o_arg = argv[i + 1];
			i++;
		} else if (option == "-R" || option == "--runtimedep") {
			assertArgument(argv, argc, i);
			o_runtime_mode += 1;
			o_arg = argv[i + 1];
			i++;
		} else if (option == "-o" || option == "--owner") {
			assertArgument(argv, argc, i);
			o_owner_mode += 1;
			o_arg = argv[i + 1];
			i++;
		} else if (option == "-f" || option == "--footprint") {
			assertArgument(argv, argc, i);
			o_footprint_mode += 1;
			o_arg = argv[i + 1];
			i++;
		} else if (option == "-L" || option == "--librairies") {
			assertArgument(argv, argc, i);
			o_librairies_mode +=1;
			o_arg =  argv[i + 1];
			i++;
		} else if (option == "--runtimedepfiles") {
			assertArgument(argv, argc, i);
			o_runtimedependencies_mode +=1;
			o_arg = argv[i + 1];
			i++;
		} else {
			m_actualError = INVALID_OPTION;
			treatErrors(option);
		}
	}

	if (o_runtimedependencies_mode + o_footprint_mode + o_details_mode + 
	o_installed_mode + o_list_mode + o_owner_mode + 
	o_footprint_mode + o_librairies_mode + o_runtime_mode == 0)
	{
		m_actualError = OPTION_MISSING;
		treatErrors(o_arg);
	}
	if (o_runtimedependencies_mode + o_footprint_mode + o_installed_mode + o_list_mode + o_owner_mode > 1)
	{
		m_actualError = TOO_MANY_OPTIONS;
		treatErrors(o_arg);
	}
	if (o_footprint_mode) {
		//
		// Make footprint
		//
		getFootprintPackage(o_arg);
	} else {
		//
		// Modes that require the database to be opened
		//
		{
			Db_lock lock(o_root, false);
			getListOfPackages(o_root);
		}
		if (o_installed_mode) {
			//
			// List installed packages
			//
			for (set<string>::const_iterator i = m_packagesList.begin(); i != m_packagesList.end(); ++i) {
				cout << *i << " ";
			}
			cout << endl;
		} else if (o_list_mode) {
			//
			// List package or file contents
			//
			getInstalledPackages(false);
			if (checkPackageNameExist(o_arg)) {
				copy(m_listOfInstPackages[o_arg].files.begin(), m_listOfInstPackages[o_arg].files.end(), ostream_iterator<string>(cout, "\n"));
			} else if (checkFileExist(o_arg)) {
				pair<string, pkginfo_t> package = openArchivePackage(o_arg);
				copy(package.second.files.begin(), package.second.files.end(), ostream_iterator<string>(cout, "\n"));
			
			} else {
				m_actualError = NOT_INSTALL_PACKAGE_NEITHER_PACKAGE_FILE;
				treatErrors(o_arg);
			}
		} else if (o_runtimedependencies_mode) {
			//
			// Get runtimedependencies of the file found in the directory path
			//
			getInstalledPackages(true); // get the list of installed package silently
			regex_t r;
			int Result;
			regcomp(&r, ".", REG_EXTENDED | REG_NOSUB);
			set<string>filenameList;
			Result = findRecursiveFile (filenameList, const_cast<char*>(o_arg.c_str()), &r, WS_DEFAULT);
			// get the list of library for all the possible files 
			set<string> librairiesList;
			for (set<string>::const_iterator i = filenameList.begin();i != filenameList.end();++i)
				Result = getRuntimeLibrairiesList(librairiesList,*i);
			// get the own package  for all the elf files dependencies libraries
#ifndef NDEBUG
			for (set<string>::const_iterator i = librairiesList.begin();i != librairiesList.end();++i)
				cerr << *i<<endl;
#endif
			if ( (librairiesList.size() > 0 ) && (Result > -1) )
			{
				set<string> runtimeList;
				for (set<string>::const_iterator i = librairiesList.begin();i != librairiesList.end();++i)
				{
					for (packages_t::const_iterator j = m_listOfInstPackages.begin(); j != m_listOfInstPackages.end();++j)
					{
						bool found = false;
						for (set<string>::const_iterator k = j->second.files.begin(); k != j->second.files.end(); ++k)
						{
							if ( k->find('/' + *i) != string::npos)
							{
								string dependencie = j->first + j->second.build;
								runtimeList.insert(dependencie);
								break;
								found = true;
							}
						}
						if ( found == true)
						{
							found = false;
							break;
						}
					}
				}
				if (runtimeList.size()>0) {
#ifndef NDEBUG
					cerr << "Number of librairies founds: " << runtimeList.size() << endl;
#endif
					unsigned int s = 1;
					for (set<string>::const_iterator i = runtimeList.begin();i!=runtimeList.end();++i) {
						cout << *i << endl;
						s++;
					}
					cout << endl;
				}
			}	
		} else if (o_librairies_mode + o_runtime_mode > 0) {
			getInstalledPackages(true); // get the list of installed package silently
			set<string> librairiesList;
			int Result = -1;
			if (checkPackageNameExist(o_arg))
			{
				for (set<string>::const_iterator i = m_listOfInstPackages[o_arg].files.begin(); i != m_listOfInstPackages[o_arg].files.end(); ++i)
				{
					string filename('/' + *i);
					Result = getRuntimeLibrairiesList(librairiesList,filename);
				}
				if ( (librairiesList.size() > 0 ) && (Result > -1) )
				{
					if (o_runtime_mode)
					{
						set<string> runtimeList;
						for (set<string>::const_iterator i = librairiesList.begin();i != librairiesList.end();++i)
						{
							for (packages_t::const_iterator j = m_listOfInstPackages.begin(); j != m_listOfInstPackages.end();++j)
							{
								bool found = false;
								for (set<string>::const_iterator k = j->second.files.begin(); k != j->second.files.end(); ++k)
								{
									if ( k->find('/' + *i) != string::npos)
									{
										runtimeList.insert(j->first);
										break;
										found = true;
									}
								}
								if (found == true)
								{
									found = false;
									break;
								}
							}
						}
						if (runtimeList.size()>0)
						{
							unsigned int s = 1;
							for (set<string>::const_iterator i = runtimeList.begin();i!=runtimeList.end();++i)
							{
								cout << *i;
								s++;
								if (s <= runtimeList.size())
									cout << ",";
							}
							cout << endl;
						}
					} else {
							for (set<string>::const_iterator i = librairiesList.begin();i != librairiesList.end();++i)
								cout << *i << endl;
					}
				}
			}	
		} else if (o_details_mode) {
			getInstalledPackages(false);
			if (checkPackageNameExist(o_arg)) {
				time_t ct = strtoul(m_listOfInstPackages[o_arg].build.c_str(),NULL,0);
				char * c_time_s = ctime(&ct);
				cout << "Name        : " << o_arg << endl
						 << "Version     : " << m_listOfInstPackages[o_arg].version << endl
						 << "Build date  : " << c_time_s
				     << "Size        : " << m_listOfInstPackages[o_arg].size << endl
				     << "Depends on  : " << m_listOfInstPackages[o_arg].run << endl
						 << "Arch        : " << m_listOfInstPackages[o_arg].arch << endl;
				}
			} else if (o_owner_mode) {
			//
			// List owner(s) of file or directory
			//
			getInstalledPackages(false);
			regex_t preg;
			if (regcomp(&preg, o_arg.c_str(), REG_EXTENDED | REG_NOSUB))
			{
				m_actualError = CANNOT_COMPILE_REGULAR_EXPRESSION;
				treatErrors(o_arg);
			}
			vector<pair<string, string> > result;
			result.push_back(pair<string, string>("Package", "File"));
			unsigned int width = result.begin()->first.length(); // Width of "Package"
#ifndef NDEBUG
			cerr << o_arg << endl;
#endif
			for (packages_t::const_iterator i = m_listOfInstPackages.begin(); i != m_listOfInstPackages.end(); ++i) {
				for (set<string>::const_iterator j = i->second.files.begin(); j != i->second.files.end(); ++j) {
					const string file('/' + *j);
					if (!regexec(&preg, file.c_str(), 0, 0, 0)) {
						result.push_back(pair<string, string>(i->first, *j));
						if (i->first.length() > width)
							width = i->first.length();
					}
				}
			}
			
			regfree(&preg);
			
			if (result.size() > 1) {
				for (vector<pair<string, string> >::const_iterator i = result.begin(); i != result.end(); ++i) {
					cout << left << setw(width + 2) << i->first << i->second << endl;
				}
			} else {
				cout << m_utilName << ": no owner(s) found" << endl;
			}
		}
	}
}
void Pkginfo::printHelp() const
{
	cout << "usage: " << m_utilName << " [options]" << endl
	     << "options:" << endl
	     << "  -i, --installed             list installed packages" << endl
       << "  -d, --details               list details about the <package>" << endl
       << "  -L, --librairies            list all the runtime librairies for the <package>" << endl
	     << "  -l, --list <package|file>   list files in <package> or <file>" << endl
	     << "  -o, --owner <pattern>       list owner(s) of file(s) matching <pattern>" << endl
	     << "  -f, --footprint <file>      print footprint for <file>" << endl
       << "  -R, --runtimedep <package>  return on a single line all the runtime dependencies" << endl
       << "  --runtimedepfiles <path>    return on a single line all the runtime dependencies for the files found in the <path>" << endl
	     << "  -r, --root <path>           specify alternative installation root" << endl
	     << "  -v, --version               print version and exit" << endl
	     << "  -h, --help                  print help and exit" << endl;
}
// vim:set ts=2 :
