// cards_sync.cc
//
//  Copyright (c) 2002-2005 Johannes Winkelmann jw at tks6 dot net 
//  Copyright (c) 2014-2015 by NuTyX team (http://nutyx.org)
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

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <cstring>
#include <cstdlib>

#include "file_utils.h"
#include "file_download.h"
#include "config_parser.h"
#include "cards_sync.h"

using namespace std;

const string CardsSync::DEFAULT_REPOFILE = ".PKGREPO";

CardsSync::CardsSync (const CardsArgumentParser& argParser)
	: m_argParser(argParser)
{
	m_repoFile = DEFAULT_REPOFILE;
}
CardsSync::CardsSync ( const CardsArgumentParser& argParser,
		const string& url, const string& baseDirectory,
		const string& repoFile)
		: m_baseDirectory(baseDirectory),
		m_remoteUrl(url),
		m_argParser(argParser)
{
	if (repoFile != "") {
		m_repoFile = repoFile;
	} else {
		m_repoFile = DEFAULT_REPOFILE;
	}
}
void CardsSync::treatErrors(const string& s) const
{
	switch ( m_actualError )
	{
		case CANNOT_FIND_FILE:
		case CANNOT_PARSE_FILE:
		case CANNOT_WRITE_FILE:
		case CANNOT_SYNCHRONIZE:
		case CANNOT_RENAME_FILE:
		case CANNOT_DETERMINE_NAME_BUILDNR:
		case EMPTY_PACKAGE:
		case CANNOT_FORK:
		case WAIT_PID_FAILED:
		case DATABASE_LOCKED:
		case CANNOT_LOCK_DIRECTORY:
		case CANNOT_REMOVE_FILE:
		case CANNOT_RENAME_DIRECTORY:
		case OPTION_ONE_ARGUMENT:
		case INVALID_OPTION:
		case OPTION_MISSING:
		case TOO_MANY_OPTIONS:
		case LISTED_FILES_ALLREADY_INSTALLED:
		case PKGADD_CONFIG_LINE_TOO_LONG:
		case PKGADD_CONFIG_WRONG_NUMBER_ARGUMENTS:
		case PKGADD_CONFIG_UNKNOWN_ACTION:
		case PKGADD_CONFIG_UNKNOWN_EVENT:
		case CANNOT_COMPILE_REGULAR_EXPRESSION:
		case NOT_INSTALL_PACKAGE_NEITHER_PACKAGE_FILE:
		case PACKAGE_NOT_FOUND:
		case PACKAGE_ALLREADY_INSTALL:
		case PACKAGE_NOT_INSTALL:
		case PACKAGE_NOT_PREVIOUSLY_INSTALL:
		case CANNOT_GENERATE_LEVEL:
		case CANNOT_CREATE_FILE:
			break;
		case CANNOT_DOWNLOAD_FILE:
			throw runtime_error("could not download " + s);
			break;
		case CANNOT_OPEN_FILE:
			throw RunTimeErrorWithErrno("could not open " + s);
			break;
		case CANNOT_READ_FILE:
			throw runtime_error("could not read " + s);
			break;
		case CANNOT_READ_DIRECTORY:
			throw RunTimeErrorWithErrno("could not read directory " + s);
			break;
		case CANNOT_CREATE_DIRECTORY:
			throw RunTimeErrorWithErrno("could not create directory " + s);
			break;
		case ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE:
			throw runtime_error(s + " only root can install / sync / upgrade / remove packages");
			break;
	}
}
unsigned int CardsSync::getLocalPackages(const string& path)
{
	m_localPackagesList.clear();
	if ( findFile( m_localPackagesList, path) != 0 ) {
		m_actualError = CANNOT_READ_DIRECTORY;
		treatErrors(path);
	}
	return m_localPackagesList.size();
}
unsigned int CardsSync::getRemotePackages(const string& pkgrepoFile)
{
	m_remotePackagesList.clear();
	if ( parseFile(m_remotePackagesList,pkgrepoFile.c_str()) != 0) {
		m_actualError = CANNOT_READ_FILE;
		treatErrors(pkgrepoFile);
	}
	return m_remotePackagesList.size();
}
void CardsSync::run2()
{
	if (getuid()) {
		m_actualError = ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE;
		treatErrors("");
	}

	Config config;
	ConfigParser::parseConfig("/etc/cards.conf", config);
	for (vector<DirUrl>::iterator i = config.dirUrl.begin();i != config.dirUrl.end(); ++i) {
		DirUrl DU = *i ;
		if (DU.Url == "" ) {
			continue;
		}
		string categoryDir, url ;
		categoryDir = DU.Dir;
		url = DU.Url;
		string category = basename(const_cast<char*>(categoryDir.c_str()));
		string categoryPKGREPOFile = categoryDir + "/" + m_repoFile ;
		// cout << "Synchronizing " << categoryDir << " from " << url << endl;
		FileDownload PKGRepo(url + "/" + m_repoFile,
			categoryDir,
			m_repoFile, false);
		PKGRepo.downloadFile();
	}
}
void CardsSync::run()
{
	if (getuid()) {
		m_actualError = ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE;
		treatErrors("");
	}
	if ( ! (m_argParser.isSet(CardsArgumentParser::OPT_PORTS)) &&
			! (m_argParser.isSet(CardsArgumentParser::OPT_INSTALLED)) &&
			! (m_argParser.isSet(CardsArgumentParser::OPT_BINARIES)) ) {
	
		throw runtime_error("missing option -i, -b OR -p");
	}
	Config config;
	ConfigParser::parseConfig("/etc/cards.conf", config);
	InfoFile downloadFile;
	string localPackageDirectory,remotePackageDirectory ;

	for (vector<DirUrl>::iterator i = config.dirUrl.begin();i != config.dirUrl.end(); ++i) {
		DirUrl DU = *i ;
		if (DU.Url == "" ) {
			continue;
		}
		string categoryDir, url ;
		categoryDir = DU.Dir;
		url = DU.Url;
		string category = basename(const_cast<char*>(categoryDir.c_str()));
		string categoryPKGREPOFile = categoryDir + "/" + m_repoFile ;
		cout << "Synchronizing " << categoryDir << " from " << url << endl;
		categoryDir = categoryDir + "/";
		// Get the .PKGREPO file of the category
		FileDownload PKGRepo(url + "/" + m_repoFile,
			categoryDir,
			m_repoFile, false);
		PKGRepo.downloadFile();
		// We read the category .PKGREPO file
		getRemotePackages(categoryPKGREPOFile);
		// and need to remove it
		//  remove(categoryMD5sumFile.c_str());
		if ( m_remotePackagesList.size() == 0 ) {
			cout << "no ports founds ..." << endl;
			continue;
		}
		vector<string> remotePackage;
		// Check what we have so far
		getLocalPackages(categoryDir);
		for (set<string>::const_iterator li = m_localPackagesList.begin(); li != m_localPackagesList.end(); ++li) {
			bool found = false;
			for (set<string>::const_iterator ri = m_remotePackagesList.begin(); ri != m_remotePackagesList.end(); ++ri) {
				localPackageDirectory = *li;
				remotePackageDirectory = *ri;
				
				if ( remotePackageDirectory.size() < 34 ) {
					m_actualError = CANNOT_PARSE_FILE;
					treatErrors(remotePackageDirectory + "  missing info ...");
				}
				if ( remotePackageDirectory[32] != '#' ) {
					m_actualError = CANNOT_PARSE_FILE;
					treatErrors( remotePackageDirectory + " wrong format... ");
				}
				split( remotePackageDirectory, '#', remotePackage, 0,true);
				// Check if the local Package is still in the depot by comparing the directories
				if ( localPackageDirectory == remotePackage[2] ) {
					found = true ;
					break;	// found check the next local one
				}
			}
			if ( ! found ) {
				deleteFolder(categoryDir + localPackageDirectory);
			}
		}

		// We check again the local one because some are delete maybe
		getLocalPackages(categoryDir);		
		vector<InfoFile> downloadFilesList;
		// Checking for new ports
		for (set<string>::const_iterator ri = m_remotePackagesList.begin(); ri != m_remotePackagesList.end(); ++ri) {
			string remotePackagePKGREPODirectory = *ri;
			remotePackageDirectory = remotePackagePKGREPODirectory.substr(33) + "/";
			downloadFile.url = url + "/" + remotePackageDirectory + m_repoFile;
			downloadFile.dirname = categoryDir + remotePackageDirectory;
			downloadFile.filename = m_repoFile;
			downloadFile.md5sum = remotePackagePKGREPODirectory.substr(0,32);
			bool PKGREPOfound = false;
			
			for (set<string>::const_iterator li = m_localPackagesList.begin(); li != m_localPackagesList.end(); ++li) {
				localPackageDirectory = *li + "/";
				if ( remotePackageDirectory == localPackageDirectory ) {
					string downloadFileName = downloadFile.dirname + m_repoFile;
					if ( checkFileExist(downloadFile.dirname + m_repoFile)) {
						// is it still uptodate
						if ( checkMD5sum(downloadFileName.c_str(),downloadFile.md5sum.c_str() ) ) {
							PKGREPOfound = true;
							break;
						}
					}
				}
			}
			if ( ! PKGREPOfound ) {
				downloadFilesList.push_back(downloadFile);
			}
		}
		if ( downloadFilesList.size() > 0 ) {
			FileDownload FD(downloadFilesList,false);
		}		
		// We check again the local one because maybe they was no one
		getLocalPackages(categoryDir);	
		downloadFilesList.clear();
		if ( (m_argParser.isSet(CardsArgumentParser::OPT_PORTS)) || 
			(m_argParser.isSet(CardsArgumentParser::OPT_BINARIES)) ) {
			/*
				If pkgfile is pass as option, we want to download everything
				concerning the build of a package. Download everything except
				the binaries
			*/	
			for (set<string>::const_iterator ri = m_localPackagesList.begin(); ri != m_localPackagesList.end(); ++ri) {
				string localPackageDirectory = *ri;
				string PKGREPOFile = categoryDir+localPackageDirectory + "/" + m_repoFile;
				string packageName = *ri;
				string::size_type pos = localPackageDirectory.find('@');
				if ( pos != std::string::npos) {
					packageName = localPackageDirectory.substr(0,pos);
				}
				vector<string> PKGREPOFileContent;
				if ( parseFile(PKGREPOFileContent,PKGREPOFile.c_str()) != 0) {
					m_actualError = CANNOT_READ_FILE;
					treatErrors(PKGREPOFile);
				}
				for (vector<string>::const_iterator i = PKGREPOFileContent.begin(); i != PKGREPOFileContent.end(); ++i) {
					string input = *i;
					if (input.size() < 11)
						continue;
					if (input[10] == ':' ) {
						continue;
					}
					downloadFile.dirname = categoryDir + localPackageDirectory +"/";
					downloadFile.md5sum = input.substr(0,32);

					string fileName = input.substr(33);
					/*
						If they is no extra separate column it's no a binary so
						if not present or wrong PKGREPO, we get it.
					*/
					string::size_type pos = fileName.find(':');
					if ( pos == std::string::npos) {
						downloadFile.url = url + "/" + localPackageDirectory + "/" + fileName;
						downloadFile.filename = fileName;
						string downloadFileName = downloadFile.dirname + fileName;
						if (! checkFileExist(downloadFile.dirname + downloadFile.filename)) {
							downloadFilesList.push_back(downloadFile);
						} else if ( ! checkMD5sum(downloadFileName.c_str(),downloadFile.md5sum.c_str()) ) {
							downloadFilesList.push_back(downloadFile);
						}
					}
				}
			}
		}
		if ( downloadFilesList.size() > 0 ) {
			FileDownload FD(downloadFilesList,false);
		}
		downloadFilesList.clear();
		if (m_argParser.isSet(CardsArgumentParser::OPT_BINARIES)) {
			/*
				If all is pass we want to download the binaries as well
			*/
			for (set<string>::const_iterator ri = m_localPackagesList.begin(); ri != m_localPackagesList.end(); ++ri) {
				bool found = false;
				string localPackageDirectory = *ri;
				string PKGREPOFile = categoryDir+localPackageDirectory + "/" + m_repoFile;
				string packageName = *ri;
				string::size_type pos = 0;
				vector<string> PKGREPOFileContent;
				if ( parseFile(PKGREPOFileContent,PKGREPOFile.c_str()) != 0) {
					m_actualError = CANNOT_READ_FILE;
					treatErrors(PKGREPOFile);
				}
				// packageInfo[0] Package Build Date
				// packageInfo[1] Package Extension
				// packageInfo[2] Package Version
				// packageInfo[3] Package Release ( new )
				// packageInfo[4] Package Description ( new )
				// packageInfo[5] Package .... ( new )
				vector<string> packageInfo;
				for (vector<string>::const_iterator i = PKGREPOFileContent.begin();i != PKGREPOFileContent.end(); ++i) {
					string input = *i;
					if (input.size() < 11) {
						cout << "Wrong Format, field to small: " << input << endl;
						continue;
					}
					if (input[10] == ':' ) {
						split( input, ':', packageInfo, 0 , true);
						found = true;
#ifndef NDEBUG
						cerr << packageInfo[0]  << " " << packageInfo[1] ;
						if ( packageInfo.size() > 2 )
							cerr << " " <<   packageInfo[2];// Version
						if ( packageInfo.size() > 3 )
							cerr << " " << packageInfo[3];  // Release
						if ( packageInfo.size() > 4 )
							cerr << " " <<  packageInfo[4]; // Description
						cerr << endl;
#endif
						continue;
					}
					if (found) {	// This port have binaries available
						downloadFile.dirname = categoryDir + localPackageDirectory +"/";
						downloadFile.md5sum = input.substr(0,32);
						string fileName = input.substr(33);
						string::size_type pos = fileName.find(':');
						if ( pos != std::string::npos) {
							string fileName = input.substr(33,pos) +
								packageInfo[0] +
								input.substr(34+pos) +
								packageInfo[1];
							downloadFile.url = url + "/" + localPackageDirectory + "/" + fileName;
							downloadFile.filename = fileName;
							string downloadFileName = downloadFile.dirname + fileName;
							if (! checkFileExist(downloadFile.dirname + downloadFile.filename)) {
								downloadFilesList.push_back(downloadFile);
							} else if ( ! checkMD5sum(downloadFileName.c_str(),downloadFile.md5sum.c_str()) ) {
									downloadFilesList.push_back(downloadFile);
							}
						}
					}
				}
			}
			if ( downloadFilesList.size() > 0 ) {
				FileDownload FD(downloadFilesList,true);
			}
		}
	} 
}
void CardsSync::deleteFolder(const string& folderName)
{
	set<string> filesToDelete;
	if ( findFile(filesToDelete, folderName) != 0 ){
			m_actualError = CANNOT_READ_FILE;
			treatErrors(folderName);
	}
	for (set<string>::const_iterator f = filesToDelete.begin(); f != filesToDelete.end(); f++) {
		string fileName = folderName + "/" + *f;
		cout << "Deleting " << fileName << endl; 
		removeFile("/",fileName);
	}
		cout << "Deleting " << folderName << endl;
		removeFile("/",folderName);
}
// vim:set ts=2 :
